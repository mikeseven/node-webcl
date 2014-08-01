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
  require('../webcl');
  log = console.log;
  exit = process.exit;
}
else
  webcl = window.webcl;

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

  log('data: '+data+' size: '+data.length)
  var check = true;
  //var str="";
  for(i=0; i<4096; i++) {
    //str+=data[i]+' ';
    if(data[i] != 5.0) {
      check = false;
      break;
    }  
  }
  //log(str);
  if(check)
    log("The data has been initialized successfully.");
  else
    log("The data has not been initialized successfully.");
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
  var platformList=webcl.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getInfo(webcl.PLATFORM_NAME));

  //Query the set of devices on this platform
  var devices = platform.getDevices(webcl.DEVICE_TYPE_GPU);
  device=devices[0];
  log('using device: '+device.getInfo(webcl.DEVICE_NAME));

  // create GPU context for this platform
  var context=webcl.createContext(device ,'Error occured in context', function(err,data){
    log(data+" : "+err);
    exit(1);
  });

  /* Build the program and create a kernel */
  var source = [
                "__kernel void callback(__global float *buffer) {",
                "  for(int i=0;i<4096;i++)",
                "   buffer[i]=5;",
                "}"
                ].join("\n");

  // Create and program from source
  log('create program');
  try {
    program=context.createProgram(source);
  } catch(ex) {
    log("Couldn't create the program. "+ex);
    exit(1);
  }

  /* Build program */
  log('build program');
  try {
    program.build(device);
  } catch(ex) {
    /* Find size of log and print to std output */
    log('build program error');
    var info=program.getBuildInfo(device, webcl.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  log('create kernel');
  try {
    kernel = program.createKernel("callback");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  log('create output buffer');
  try {
    data_buffer = context.createBuffer(webcl.MEM_WRITE_ONLY, 4096*4);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }

  /* Create kernel argument */
  log('kernel set arg');
  try {
    kernel.setArg(0, data_buffer);
  } catch(ex) {
    log("Couldn't set a kernel argument. "+ex);
    exit(1);   
  };

  /* Create a command queue */
  log('create q');
  try {
    queue = context.createCommandQueue(device,0);
  } catch(ex) {
    log("Couldn't create a command queue. "+ex);
    exit(1);   
  };

  /* Enqueue kernel */
  log('enqueue task');
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
  log('set event callbacks');
  kernel_event.setCallback(webcl.COMPLETE, kernel_complete, "The kernel finished successfully.");
  read_event.setCallback(webcl.COMPLETE, read_complete, data);

  // test 1: queue should finish with event completed
  // log('q finish');
  // queue.finish(); // wait for everything to finish

  // test 2: wait for all events to complete
  log('Wait for events to complete');
  webcl.waitForEvents([kernel_event, read_event]);
  
  // test 3: spin on all event completions
  // log('  spinning on event completion');
  // var event_list=[kernel_event, read_event];
  // for(var i=0;i<event_list.length;i++) {
  //   var ev = event_list[i];
  //   while(1) {
  //     var ret=ev.getInfo(webcl.EVENT_COMMAND_EXECUTION_STATUS);
  //     if(ret == webcl.COMPLETE)
  //       break;
  //   }
  // }
  log("main app thread END"); 
})();
