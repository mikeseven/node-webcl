/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl    = require('../webcl');
  clu   = require('../lib/clUtils');
  util  = require("util"),
  fs    = require("fs");
  Image = require("node-image").Image;
  log   = console.log;
}

//First check if the webcl extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  process.exit(-1);
}

var file = __dirname+'/mike_scooter.jpg';

log('Loading image '+file);
var img=Image.load(file);
var image=img.convertTo32Bits();
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
  var platform=platformList[0];

  // create GPU context for this platform
  var context=cl.createContext({
    deviceType: cl.DEVICE_TYPE_GPU,
    platform: platform});

  // find the device for this context
  var devices = context.getInfo(cl.CONTEXT_DEVICES);
  if(!devices) {
      alert("Error: Failed to retrieve compute devices for context!");
      return -1;
  }
  
  var device_found=false;
  var device;
  for(var i=0,l=devices.length;i<l;++i ) 
  {
    var device_type=devices[i].getInfo(cl.DEVICE_TYPE);
    if(device_type == cl.DEVICE_TYPE_GPU) 
    {
        device = devices[i];
        device_found = true;
        break;
    } 
  }
  
  if(!device_found)
  {
      alert("Error: Failed to locate compute device!");
      return -1;
  }

  // Report the device vendor and device name
  // 
  var vendor_name = device.getInfo(cl.DEVICE_VENDOR);
  var device_name = device.getInfo(cl.DEVICE_NAME);

  log("Connecting to: "+vendor_name+" "+device_name);

  kernelSourceCode = fs.readFileSync(__dirname+'/swapRB.cl','ascii');
  
  //Create and program from source
  program=context.createProgram(kernelSourceCode);
  
  //Build program
  program.build(device);

  // create device buffers
  try {
    cmPinnedBufIn = context.createBuffer(cl.MEM_READ_ONLY | cl.MEM_ALLOC_HOST_PTR, image.size);
    cmPinnedBufOut = context.createBuffer(cl.MEM_WRITE_ONLY | cl.MEM_ALLOC_HOST_PTR, image.size);
  }
  catch(err) {
    console.log('error creating buffers');
    process.exit(-1);
  }

  //Create kernel object
  try {
    kernel= program.createKernel("swapRB");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,cl.PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  kernel.setArg(0, cmPinnedBufIn);
  kernel.setArg(1, cmPinnedBufOut);
  kernel.setArg(2, image.width, cl.type.UINT);
  kernel.setArg(3, image.height, cl.type.UINT);

  //Create command queue
  queue=context.createCommandQueue(device, 0);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS=[ kernel.getWorkGroupInfo(device, cl.KERNEL_WORK_GROUP_SIZE) ];
  var globalWS = [ localWS[0] * clu.DivUp(image.size, localWS[0]) ];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Write our data set into the input array in device memory asynchronously
  queue.enqueueWriteBuffer(cmPinnedBufIn, false, {
    origin: 0, 
    size: image.size, 
    buffer: image.buffer});

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      null,
      globalWS,
      localWS);
  
  queue.enqueueReadBuffer(cmPinnedBufOut, false, {
    origin: 0, 
    size: out.length, 
    buffer: out});

  queue.finish(); //Finish all the operations

  return out;
}


