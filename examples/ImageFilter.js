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
  WebCL    = require('../webcl');
  clu   = require('../lib/clUtils');
  util  = require("util"),
  fs    = require("fs");
  Image = require("node-image").Image;
  log   = console.log;
}

//First check if the webcl extension is installed at all 
if (WebCL == undefined) {
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

  // create GPU context for this platform
  var context=WebCL.createContext(WebCL.DEVICE_TYPE_GPU);

  // find the device for this context
  var devices = context.getInfo(WebCL.CONTEXT_DEVICES);
  if(!devices) {
      alert("Error: Failed to retrieve compute devices for context!");
      return -1;
  }
  
  var device_found=false;
  var device;
  for(var i=0,l=devices.length;i<l;++i ) 
  {
    var device_type=devices[i].getInfo(WebCL.DEVICE_TYPE);
    if(device_type == WebCL.DEVICE_TYPE_GPU) 
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
  var vendor_name = device.getInfo(WebCL.DEVICE_VENDOR);
  var device_name = device.getInfo(WebCL.DEVICE_NAME);

  log("Connecting to: "+vendor_name+" "+device_name);

  kernelSourceCode = fs.readFileSync(__dirname+'/swapRB.cl','ascii');
  
  //Create and program from source
  program=context.createProgram(kernelSourceCode);
  
  //Build program
  program.build(device);

  // create device buffers
  try {
    cmPinnedBufIn = context.createBuffer(WebCL.MEM_READ_ONLY | WebCL.MEM_ALLOC_HOST_PTR, image.size);
    cmPinnedBufOut = context.createBuffer(WebCL.MEM_WRITE_ONLY | WebCL.MEM_ALLOC_HOST_PTR, image.size);
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
    console.log(program.getBuildInfo(device,WebCL.PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  var aints=new Int32Array(2);
  aints[0]=image.width;
  aints[1]=image.height;
  kernel.setArg(0, cmPinnedBufIn);
  kernel.setArg(1, cmPinnedBufOut);
  kernel.setArg(2, aints);
  kernel.setArg(3, aints.subarray(1,1));

  //Create command queue
  queue=context.createCommandQueue(device, 0);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS=[ kernel.getWorkGroupInfo(device, WebCL.KERNEL_WORK_GROUP_SIZE) ];
  var globalWS = [ localWS[0] * clu.DivUp(image.size, localWS[0]) ];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Write our data set into the input array in device memory asynchronously
  queue.enqueueWriteBuffer(cmPinnedBufIn, false, 0, image.size, image.buffer);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      null,
      globalWS,
      localWS);

   queue.enqueueReadBuffer(cmPinnedBufOut, false, 0, out.length, out);

   queue.finish(); //Finish all the operations

  return out;
}


