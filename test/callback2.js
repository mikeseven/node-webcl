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
  webcl = require('../webcl');
  log = console.log;
  exit = process.exit;
}
else
  webcl = window.webcl;

/* CL objects */
var /* WebCLPlatform */     platform;
var /* WebCLDevice */       device;
var /* WebCLContext */      context;
var /* WebCLProgram */      program;
var /* WebCLKernel */       kernel;
var /* WebCLCommandQueue */ queue;
var /* WebCLEvent */        kernel_event, read_event;
var /* WebCLBuffer */       data_buffer;
var done=false;

// kernel callback
function kernel_complete(event, data) {
  var status=event.status;
  log('[JS CB] kernel_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);
  log(data);
}

// read buffer callback
function read_complete(event, data) {
  var status=event.status;
  log('[JS CB] read_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);

  var check = webcl.TRUE;
  //var str="";
  for(i=0; i<4096; i++) {
    //str+=data[i]+' ';
    if(data[i] != 5.0) {
      check = webcl.FALSE;
      break;
    }  
  }
  //log(str);
  if(check)
    log("The data has been initialized successfully.");
  else
    log("The data has not been initialized successfully.");
}

function program_built(err, data) {
  log('[JS CB] program built');
  try {
    kernel = program.createKernel("callback");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(webcl.MEM_WRITE_ONLY, 4096*4);
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
    queue = context.createCommandQueue(device, 0);
  } catch(ex) {
    log("Couldn't create a command queue. "+ex);
    exit(1);   
  };

  /* Enqueue kernel */
  try {
    kernel_event=new webcl.WebCLEvent();
    queue.enqueueTask(kernel , null, kernel_event);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read the buffer */
  var data=new Float32Array(4096);
  try {
    read_event=new webcl.WebCLEvent();
    queue.enqueueReadBuffer(data_buffer, false, 0, 4096*4, data, null, read_event);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  /* Set event handling routines */
  try {
    kernel_event.setCallback(webcl.COMPLETE, kernel_complete, "The kernel finished successfully.");
  } catch(ex) {
    log("Couldn't set callback for event. "+ex);
    exit(1);   
  }
  read_event.setCallback(webcl.COMPLETE, read_complete, data);
  
  queue.finish(); // wait for everything to finish
  done=true;
}
function main() {

  /* Create a device and context */
  log('creating context');
  
  //Pick platform
  var platformList=webcl.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getInfo(webcl.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  var devices = platform.getDevices(webcl.DEVICE_TYPE_GPU);

  // make sure we use a discrete GPU (Intel embedded GPU don't support event correctly)
  device=null;
  for(var i=0;i<devices.length;i++) {
    var vendor=devices[i].getInfo(webcl.DEVICE_VENDOR).trim().toUpperCase();
    if(vendor==='NVIDIA' || vendor==='AMD' || vendor.indexOf('ADVANCED MICRO DEVICES')>=0) {
      device=devices[i];
      break;
    }
  }
  if(!device || i==devices.length) {
    log("[ERROR] No suitable device found");
    exit(-1);
  }

  log('using device: '+device.getInfo(webcl.DEVICE_VENDOR).trim()+
    ' '+device.getInfo(webcl.DEVICE_NAME).trim());

  // create GPU context for this platform
  context=webcl.createContext(device);

  /* Build the program and create a kernel */
  var source = [
                "__kernel void callback(__global float *buffer) {",
                "  for(int i=0; i<4096; i++) ",
                "     buffer[i]=5;",
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
    program.build(device, null, program_built);
  } catch(ex) {
    /* Find size of log and print to std output */
    var info=program.getBuildInfo(device, webcl.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  log("main app thread END");

  // sleeping the main thread to let events propagate
  // function sleep() {
  //   if(!done) {
  //     log('sleeping 0.5s');
  //     setTimeout(sleep, 500);
  //   }
  // }
  // sleep();
}

main();
