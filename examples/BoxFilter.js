// Copyright (c) 2011-2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL = require('../webcl');
  clu = require('../lib/clUtils');
  util = require('util');
  fs = require('fs');
  Image = require('node-image').Image;
  log = console.log;
}

//First check if the webcl extension is installed at all 
if (WebCL == undefined) {
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
var platformList=WebCL.getPlatforms();
var platform=platformList[0];

//Query the set of GPU devices on this platform
var devices = platform.getDevices(WebCL.DEVICE_TYPE_ALL);
log("  # of Devices Available = "+devices.length); 
var uiTargetDevice = clu.clamp(uiTargetDevice, 0, (devices.length - 1));
var device=devices[uiTargetDevice];
log("  Using Device "+ uiTargetDevice+": "+device.getInfo(WebCL.DEVICE_NAME)); 

var hasImageSupport=device.getInfo(WebCL.DEVICE_IMAGE_SUPPORT);
if(hasImageSupport != WebCL.TRUE) {
  log("No image support");
  return;
}

var numComputeUnits=device.getInfo(WebCL.DEVICE_MAX_COMPUTE_UNITS);
log('  # of Compute Units = '+numComputeUnits);

log('  createContext...');
context=WebCL.createContext(device);

// Create a command-queue 
queue=context.createCommandQueue(device, 0);

// Allocate OpenCL object for the source data
var InputFormat= {
  channelOrder : WebCL.RGBA,
  channelType : WebCL.UNSIGNED_INT8,
  width : image.width, 
  height : image.height,
  rowPitch : image.pitch
};

//2D Image (Texture) on device
cmDevBufIn = context.createImage(WebCL.MEM_READ_ONLY | WebCL.MEM_USE_HOST_PTR, InputFormat, image.buffer);

RowSampler = context.createSampler(false, WebCL.ADDRESS_CLAMP, WebCL.FILTER_NEAREST);

// Allocate the OpenCL intermediate and result buffer memory objects on the device GMEM
cmDevBufTemp = context.createBuffer(WebCL.MEM_READ_WRITE, szBuffBytes);
cmDevBufOut = context.createBuffer(WebCL.MEM_WRITE_ONLY, szBuffBytes);

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
queue.enqueueReadBuffer(cmDevBufOut, WebCL.TRUE, 0, szBuffBytes, uiOutput);

// PNG uses 32-bit images, JPG can only work on 24-bit images
if(!Image.save('out_'+iRadius+'.png',uiOutput, image.width,image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
  log("Error saving image");

// cleanup
log(util.inspect(process.memoryUsage()));

function ResetKernelArgs(width, height, r, fScale)
{
  // (Image/texture version)
  var aints=new Int32Array(3);
  aints[0]=width;
  aints[1]=height;
  aints[2]=r;
  var afloats=new Float32Array(1);
  afloats[0]=fScale;

  ckBoxRowsTex.setArg(0, cmDevBufIn);
  ckBoxRowsTex.setArg(1, cmDevBufTemp);
  ckBoxRowsTex.setArg(2, RowSampler); 
  ckBoxRowsTex.setArg(3, aints);
  ckBoxRowsTex.setArg(4, aints.subarray(1));
  ckBoxRowsTex.setArg(5, aints.subarray(2));
  ckBoxRowsTex.setArg(6, afloats);

  // Set the Argument values for the column kernel
  ckBoxColumns.setArg(0, cmDevBufTemp);
  ckBoxColumns.setArg(1, cmDevBufOut);
  ckBoxColumns.setArg(2, aints);
  ckBoxColumns.setArg(3, aints.subarray(1));
  ckBoxColumns.setArg(4, aints.subarray(2));
  ckBoxColumns.setArg(5, afloats);
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
  queue.enqueueWriteImage(cmDevBufIn, WebCL.TRUE, szTexOrigin, szTexRegion, 0, 0, image.buffer);

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
