/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl = require('../webcl');
  clu = require('../lib/clUtils');
  util = require('util');
  fs = require('fs');
  Image = require('node-image').Image;
  log = console.log;
}

//First check if the webcl extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

process.on('exit',function() {
  log('Exiting app');
  log(util.inspect(process.memoryUsage()));
})


// Box processing params
var uiNumOutputPix = 64;                    // Default output pix per workgroup... may be modified depending HW/OpenCl caps
var iRadius = 10;                           // initial radius of 2D box filter mask
var fScale = 1/(2 * iRadius + 1);  // precalculated GV rescaling value

// OpenCL variables
var ckBoxRowsTex;             // OpenCL Kernel for row sum (using 2d Image/texture)
var ckBoxColumns;             // OpenCL for column sum and normalize
var cmDevBufIn;               // OpenCL device memory object (buffer or 2d Image) for input data
var cmDevBufTemp;             // OpenCL device memory temp buffer object  
var cmDevBufOut;              // OpenCL device memory output buffer object
var szBuffBytes;              // Size of main image buffers
var szGlobalWorkSize=[0,0];      // global # of work items
var szLocalWorkSize= [0,0];       // work group # of work items 
var szMaxWorkgroupSize = 512; // initial max # of work items

// load image
var file = __dirname+'/lenaRGB.jpg';
log('Loading image '+file);
var img=Image.load(file);
var image=img.convertTo32Bits();
var szBuffBytes = image.height*image.pitch;
//img.unload();
log('Image '+file+': \n'+util.inspect(image));

//Pick platform
var platformList=cl.getPlatforms();
var platform=platformList[0];

//Query the set of GPU devices on this platform
var devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
log("  # of Devices Available = "+devices.length); 
var uiTargetDevice = clu.clamp(uiTargetDevice, 0, (devices.length - 1));
var device=devices[uiTargetDevice];
log("  Using Device "+ uiTargetDevice+": "+device.getInfo(cl.DEVICE_NAME)); 

var hasImageSupport=device.getInfo(cl.DEVICE_IMAGE_SUPPORT);
if(hasImageSupport != cl.TRUE) {
  log("No image support");
  return;
}

var numComputeUnits=device.getInfo(cl.DEVICE_MAX_COMPUTE_UNITS);
log('  # of Compute Units = '+numComputeUnits);

log('  createContext...');
context=cl.createContext({
  devices: device, 
  platform: platform
});

// Create a command-queue 
queue=context.createCommandQueue(device, 0);

// Allocate OpenCL object for the source data
var InputFormat= {
    order : cl.RGBA,
    data_type : cl.UNSIGNED_INT8
};

//2D Image (Texture) on device
cmDevBufIn = context.createImage2D(cl.MEM_READ_ONLY | cl.MEM_USE_HOST_PTR, InputFormat, 
    image.width, image.height, image.pitch /*image.width*Uint32Array.BYTES_PER_ELEMENT*/, image.buffer);

RowSampler = context.createSampler(false, cl.ADDRESS_CLAMP, cl.FILTER_NEAREST);

// Allocate the OpenCL intermediate and result buffer memory objects on the device GMEM
cmDevBufTemp = context.createBuffer(cl.MEM_READ_WRITE, szBuffBytes);
cmDevBufOut = context.createBuffer(cl.MEM_WRITE_ONLY, szBuffBytes);

//Create the program 
sourceCL = fs.readFileSync(__dirname+'/BoxFilter.cl','ascii');
cpProgram = context.createProgram(sourceCL);

sBuildOpts = "-cl-fast-relaxed-math";
ciErrNum = cpProgram.build(device, sBuildOpts);

// Create kernels
ckBoxRowsTex = cpProgram.createKernel("BoxRowsTex");
ckBoxColumns = cpProgram.createKernel("BoxColumns");

// set the kernel args
ResetKernelArgs(image.width, image.height, iRadius, fScale);

// Warmup call to assure OpenCL driver is awake
BoxFilterGPU (image, cmDevBufOut, iRadius, fScale);
queue.finish();

// launch processing on the GPU
BoxFilterGPU (image, cmDevBufOut, iRadius, fScale);
queue.finish();

// Copy results back to host memory, block until complete
var uiOutput=new Uint8Array(szBuffBytes);
queue.enqueueReadBuffer(cmDevBufOut, cl.TRUE, {
  offset: 0, 
  size: szBuffBytes, 
  buffer: uiOutput
});

// PNG uses 32-bit images, JPG can only work on 24-bit images
if(!Image.save('out_'+iRadius+'.png',uiOutput, image.width,image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
  log("Error saving image");

// cleanup
log(util.inspect(process.memoryUsage()));

function ResetKernelArgs(width, height, r, fScale)
{
  // (Image/texture version)
  ckBoxRowsTex.setArg(0, cmDevBufIn);
  ckBoxRowsTex.setArg(1, cmDevBufTemp);
  ckBoxRowsTex.setArg(2, RowSampler); 
  ckBoxRowsTex.setArg(3, width, cl.type.UINT);
  ckBoxRowsTex.setArg(4, height, cl.type.UINT);
  ckBoxRowsTex.setArg(5, r, cl.type.INT);
  ckBoxRowsTex.setArg(6, fScale, cl.type.FLOAT);

  // Set the Argument values for the column kernel
  ckBoxColumns.setArg(0, cmDevBufTemp);
  ckBoxColumns.setArg(1, cmDevBufOut);
  ckBoxColumns.setArg(2, width, cl.type.UINT);
  ckBoxColumns.setArg(3, height, cl.type.UINT);
  ckBoxColumns.setArg(4, r, cl.type.INT);
  ckBoxColumns.setArg(5, fScale, cl.type.FLOAT);
}

//OpenCL computation function for GPU:  
//Copies input data to the device, runs kernel, copies output data back to host  
//*****************************************************************************
function BoxFilterGPU(image, cmOutputBuffer, r, fScale)
{
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer);

  // Copy input data from host to device 
  var szTexOrigin = [0, 0, 0];                // Offset of input texture origin relative to host image
  var szTexRegion = [image.width, image.height, 1];   // Size of texture region to operate on
  log('enqueue image: origin='+szTexOrigin+", region="+szTexRegion);
  queue.enqueueWriteImage(cmDevBufIn, cl.TRUE, szTexOrigin, szTexRegion, 0, 0, image.buffer);

  // Set global and local work sizes for row kernel
  szLocalWorkSize[0] = uiNumOutputPix;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0]= szLocalWorkSize[0] * clu.DivUp(image.height, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  log("row kernel work sizes: global="+szGlobalWorkSize+" local="+szLocalWorkSize);
  
  //Sync host
  queue.finish();

  //Launch row kernel
  queue.enqueueNDRangeKernel(ckBoxRowsTex, null, szGlobalWorkSize, szLocalWorkSize);

  //Set global and local work sizes for column kernel
  szLocalWorkSize[0] = 64;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0] = szLocalWorkSize[0] * clu.DivUp(image.width, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  log("column kernel work sizes: global="+szGlobalWorkSize+" local="+szLocalWorkSize);

  //Launch column kernel
  queue.enqueueNDRangeKernel(ckBoxColumns, null, szGlobalWorkSize, szLocalWorkSize);

  //sync host
  queue.finish();
}
