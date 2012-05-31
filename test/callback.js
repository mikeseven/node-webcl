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

var cl = new WebCL();
var completed_kernel=false, completed_read=false;

// kernel callback
function kernel_complete(event, data) {
  var status=event.status;
  log('in JS kernel_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);
  log(data);
  completed_kernel=true;
}

// read buffer callback
function read_complete(event, data) {
  var status=event.status;
  log('in JS read_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);

  var check = cl.TRUE;
  //var str="";
  for(i=0; i<4096; i++) {
    //str+=data[i]+' ';
    if(data[i] != 5.0) {
      check = cl.FALSE;
      break;
    }  
  }
  //log(str);
  if(check)
    log("The data has been initialized successfully.");
  else
    log("The data has not been initialized successfully.");
  completed_read=true;
}

(function main() {
  /* CL objects */
  var /* WebCLPlatform */     platform;
  var /* WebCLDevice */       device;
  var /* WebCLContext */      context;
  var /* WebCLProgram */      program;
  var /* WebCLKernel */       kernel;
  var /* WebCLCommandQueue */ queue;
  var /* WebCLEvent */        kernel_event, read_event;
  var /* WebCLBuffer */       data_buffer;

  /* Create a device and context */
  log('creating context');
  
  //Pick platform
  var platformList=cl.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getInfo(cl.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  var devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
  device=devices[0];
  log('using device: '+device.getInfo(cl.DEVICE_NAME));

  // create GPU context for this platform
  var context=cl.createContext({
    devices: device, 
    platform: platform
  } ,'Error occured in context', function(err,data){
    log(data+" : "+err);
    exit(1);
  });

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
    program.build(devices);
  } catch(ex) {
    /* Find size of log and print to std output */
    var info=program.getBuildInfo(devices[0], cl.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  try {
    kernel = program.createKernel("callback");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(cl.MEM_WRITE_ONLY, 4096*4);
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
    kernel_event=new cl.WebCLEvent();
    queue.enqueueTask(kernel , null, kernel_event);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read the buffer */
  var data=new Float32Array(4096);
  try {
    read_event=new cl.WebCLEvent();
    queue.enqueueReadBuffer(data_buffer, false, 0, 4096*4, data, null, read_event);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  /* Set event handling routines */
  try {
    kernel_event.setCallback(cl.COMPLETE, kernel_complete, "The kernel finished successfully.");
  } catch(ex) {
    log("Couldn't set callback for event. "+ex);
    exit(1);   
  }
  read_event.setCallback(cl.COMPLETE, read_complete, data);
  
  queue.finish(); // wait for everything to finish

  log("main app thread END"); 
})();

