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
  log = console.log;
  exit = process.exit;
}

function read_complete(event, data) {
  log('in read_complete, status: '+event.status);
  log("New data: "+data[0]+', '+data[1]+', '+data[2]+', '+data[3]);
}

function main() {
  /* CL objects */
  var /* WebCLPlatform */     platform;
  var /* WebCLDevice */       device;
  var /* WebCLContext */      context;
  var /* WebCLProgram */      program;
  var /* WebCLKernel */       kernel;
  var /* WebCLCommandQueue */ queue;
  var /* WebCLEvent */        user_event;
  var /* WebCLBuffer */       data_buffer;

  /* Initialize data */
  var data=new Float32Array(4);
  for(i=0; i<4; i++)
     data[i] = i * 2.0;

  /* Create a device and context */
  log('creating context');
  
  // //Pick platform
  // var platformList=WebCL.getPlatforms();
  // platform=platformList[0];
  // log('using platform: '+platform.getInfo(WebCL.PLATFORM_NAME));
 
  
  // //Query the set of devices on this platform
  // var devices = platform.getDevices(WebCL.DEVICE_TYPE_GPU);
  // device=devices[0];
  // log('using device: '+device.getInfo(WebCL.DEVICE_NAME));

  // // create GPU context for this platform
  // var context=WebCL.createContext({
  //   devices: device, 
  //   platform: platform
  // });

  var context=null;
  try {
    context=WebCL.createContext({
      deviceType: WebCL.DEVICE_TYPE_ALL, 
      // platform: platform
    });
  }
  catch(ex) {
    throw new Exception("Can't create CL context");
  }

  var devices=context.getInfo(WebCL.CONTEXT_DEVICES);
  log("Found "+devices.length+" devices");
  var device=devices[0];

  /* Build the program and create a kernel */
  var source = [
    "__kernel void user_event(__global float4 *v) {",
    "  *v *= -1.0f;",
    "}"
  ].join("\n");

  // Create and program from source
  try {
    program=context.createProgram(source);
  } catch(ex) {
    log("Couldn't create the program. "+ex);
    exit(1);
  }

  /* Build program */
  try {
    program.build(devices);
  } catch(ex) {
    /* Find size of log and print to std output */
    var info=program.getBuildInfo(devices[0], WebCL.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  try {
    kernel = program.createKernel("user_event");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(WebCL.MEM_READ_WRITE | WebCL.MEM_COPY_HOST_PTR, 4*Float32Array.BYTES_PER_ELEMENT,data);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }

  /* Create kernel argument */
  try {
    kernel.setArg(0, data_buffer);
  } catch(ex) {
    log("Couldn't set a kernel argument. "+ex);
    exit(1);   
  };

  /* Create a command queue */
  try {
    queue = context.createCommandQueue(device, WebCL.QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
  } catch(ex) {
    log("Couldn't create an out of order command queue, using in-order queue. "+ex);
    queue = context.createCommandQueue(device);
  };

  /* Configure events */
  try {
    user_event = context.createUserEvent();
  } catch(ex) {
     log("Couldn't enqueue the kernel");
     exit(1);   
  }

  /* Enqueue kernel */
  try {
    kernel_event=new WebCL.WebCLEvent();
    queue.enqueueTask(kernel , [user_event], kernel_event);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read the buffer */
  try {
    read_event=new WebCL.WebCLEvent();
    queue.enqueueReadBuffer(data_buffer, false, 0, data.byteLength, data, [ kernel_event ], read_event);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  /* Set event handling routines */
  try {
    read_event.setCallback(WebCL.COMPLETE, read_complete, data);
  } catch(ex) {
    log("Couldn't set callback for read event. "+ex);
    exit(1);   
  }
  
  log("Old data: "+data[0]+', '+data[1]+', '+data[2]+', '+data[3]);

  /* Set user event to success */
  user_event.setUserEventStatus(WebCL.SUCCESS);
  
  queue.finish();
  log('queue finished');
}

main();
