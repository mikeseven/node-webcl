/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var util    = require("util"),
    fs     = require("fs"),
    Image  = require("node-image").Image,
    Jpeg  = require("jpeg").Jpeg,
    Buffer = require('buffer').Buffer,
    textureBuffer;

var WebCL=require("../lib/webcl"),
sys=require('util'),
clu=require('../lib/clUtils.js');

var log=console.log;

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

var file = __dirname+'/mike_scooter.jpg';

log('Loading image '+file);
var img=Image.load(file);
image=img.convertTo32Bits();
//img.unload();
log('Image '+file+': \n'+util.inspect(image));

image.size = image.height*image.pitch;

ImageFilter(image);
image.unload();

function ImageFilter(image) {
  var out=new Uint8Array(image.size);
  for(var i=0;i<image.size;i++)
    out[i]=0;

  //Pick platform
  var platformList=WebCL.getPlatformIDs();
  platform=platformList[0];

  //Pick first platform
  context=WebCL.createContext(WebCL.CL_DEVICE_TYPE_GPU, [WebCL.CL_CONTEXT_PLATFORM, platform]);

  //Query the set of devices attached to the context
  devices = context.getInfo(WebCL.CL_CONTEXT_DEVICES);
  
  kernelSourceCode = fs.readFileSync(__dirname+'/swapRB.cl','ascii');
  
  //Create and program from source
  program=context.createProgram(kernelSourceCode);
  
  //Build program
  program.build(devices);

  // create device buffers
  try {
    cmPinnedBufIn = context.createBuffer(WebCL.CL_MEM_READ_ONLY | WebCL.CL_MEM_ALLOC_HOST_PTR,image.size);
    cmPinnedBufOut = context.createBuffer(WebCL.CL_MEM_WRITE_ONLY | WebCL.CL_MEM_ALLOC_HOST_PTR, image.size);
    //coefs_ver = context.createBuffer(WebCL.CL_MEM_READ_ONLY,  COEFS_SIZE);
    //coefs_hor = context.createBuffer(WebCL.CL_MEM_READ_ONLY,  COEFS_SIZE);
  }
  catch(err) {
    console.log('error creating buffers');
    return;
  }

  //Create kernel object
  try {
    kernel= program.createKernel("swapRB");
  }
  catch(err) {
    console.log(program.getBuildInfo(devices[0],WebCL.CL_PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  kernel.setArg(0, cmPinnedBufIn, WebCL.type.MEM);
  kernel.setArg(1, cmPinnedBufOut, WebCL.type.MEM);
  kernel.setArg(2, image.width, WebCL.type.INT | WebCL.type.UNSIGNED);
  kernel.setArg(3, image.height, WebCL.type.INT | WebCL.type.UNSIGNED);

  //Create command queue
  queue=context.createCommandQueue(devices[0], 0);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS=[ kernel.getWorkGroupInfo(devices[0], WebCL.CL_KERNEL_WORK_GROUP_SIZE) ];
  var globalWS = [ Math.ceil (image.size / localWS[0]) * localWS[0] ];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      [],
      globalWS,
      localWS);
  

  // Write our data set into the input array in device memory asynchronously
  queue.enqueueWriteBuffer(cmPinnedBufIn, false, {
    offset: 0, 
    size: image.size, 
    buffer: image.buffer},
    []);

  queue.enqueueReadBuffer(cmPinnedBufOut, false, {
    offset: 0, 
    size: out.length, 
    buffer: out},
    [] );

  /*output=queue.enqueueMapBuffer(
      outputBuffer,
      WebCL.CL_TRUE, // block 
      WebCL.CL_MAP_READ,
      0,
      image.size);
  
  out=output.getBuffer();
  
  queue.enqueueUnmapMemObject(
      outputBuffer,
      output);*/
  
  queue.finish (); //Finish all the operations
  
  // PNG uses 32-bit images, JPG can only work on 24-bit images
  if(!Image.save('out.png',out, image.width,image.height, image.pitch, image.bpp, 0xFF0000, 0xFF00, 0xFF))
    log("Error saving image");
}


