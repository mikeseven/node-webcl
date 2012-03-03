/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var util    = require("util"),
    fs     = require("fs"),
    Image  = require("node-image").Image,
    Buffer = require('buffer').Buffer,
    textureBuffer;

var cl = require('../webcl'),
    clu=require('../lib/clUtils.js'),
    log=console.log;

//First check if the webcl extension is installed at all 
if (cl == undefined) {
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

var outBuf=ImageFilter(image);

// PNG uses 32-bit images, JPG can only work on 24-bit images
if(!Image.save('out.png',outBuf, image.width, image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
  log("Error saving image");
image.unload();

function ImageFilter(image) {
  var out=new Uint8Array(image.size);

  //Pick platform
  var platformList=cl.getPlatforms();
  platform=platformList[0];

  //Query the set of devices on this platform
  devices = platform.getDevices(cl.DEVICE_TYPE_GPU);

  // create GPU context for this platform
  context=cl.createContext({
    deviceType: cl.DEVICE_TYPE_GPU,
    platform: platform});

  kernelSourceCode = fs.readFileSync(__dirname+'/swapRB.cl','ascii');
  
  //Create and program from source
  program=context.createProgram(kernelSourceCode);
  
  //Build program
  program.build(devices);

  // create device buffers
  try {
    cmPinnedBufIn = context.createBuffer(cl.MEM_READ_ONLY | cl.MEM_ALLOC_HOST_PTR, image.size);
    cmPinnedBufOut = context.createBuffer(cl.MEM_WRITE_ONLY | cl.MEM_ALLOC_HOST_PTR, image.size);
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
    console.log(program.getBuildInfo(devices[0],cl.PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  kernel.setArg(0, cmPinnedBufIn, cl.type.MEM);
  kernel.setArg(1, cmPinnedBufOut, cl.type.MEM);
  kernel.setArg(2, image.width, cl.type.INT | cl.type.UNSIGNED);
  kernel.setArg(3, image.height, cl.type.INT | cl.type.UNSIGNED);

  //Create command queue
  queue=context.createCommandQueue(devices[0], 0);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS=[ kernel.getWorkGroupInfo(devices[0], cl.KERNEL_WORK_GROUP_SIZE) ];
  var globalWS = [ Math.ceil (image.size / localWS[0]) * localWS[0] ];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      null,
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

  queue.finish (); //Finish all the operations
  
  return out;
}


