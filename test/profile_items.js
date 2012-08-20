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

function read_complete(status, data) {
  log('in read_complete, status: '+status);
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
  var /* WebCLEvent */        prof_event;
  var /* WebCLBuffer */       data_buffer;
  var /* ArrayBuffer */       mapped_memory;
  
  var NUM_INTS = 4096;
  var NUM_ITEMS = 512;
  var NUM_ITERATIONS = 2000;
  
  /* Initialize data */
  var data=new Uint32Array(NUM_INTS);
  for(i=0; i<NUM_INTS; i++) {
     data[i] = i;
  }

  /* Set number of data points and work-items */
  var num_ints = NUM_INTS;
  var num_items = NUM_ITEMS;

  /* Create a device and context */
  log('creating context');
  
  //Pick platform
  var platformList=WebCL.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getInfo(WebCL.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  var devices = platform.getDevices(WebCL.DEVICE_TYPE_DEFAULT);
  device=devices[0];
  log('using device: '+device.getInfo(WebCL.DEVICE_NAME));

  // create GPU context for this platform
  var context=WebCL.createContext({
    devices: device, 
    platform: platform
  });

  /* Build the program and create a kernel */
  var source = [
    "__kernel void profile_items(__global int4 *x, int num_ints) {",
    "  int num_vectors = num_ints/(4 * get_global_size(0));",
    "  x += get_global_id(0) * num_vectors;",
    "  for(int i=0; i<num_vectors; i++) {",
    "     x[i] += 1;",
    "     x[i] *= 2;",
    "     x[i] /= 3;",
    "  }",
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
    kernel = program.createKernel("profile_items");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(WebCL.MEM_READ_WRITE | WebCL.MEM_COPY_HOST_PTR, data.byteLength, data);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }

  /* Create kernel argument */
  try {
    kernel.setArg(0, data_buffer);
    kernel.setArg(1, num_ints, WebCL.type.INT);
  } catch(ex) {
    log("Couldn't set a kernel argument. "+ex);
    exit(1);   
  };

  /* Create a command queue */
  try {
    queue = context.createCommandQueue(device, WebCL.QUEUE_PROFILING_ENABLE);
  } catch(ex) {
    log("Couldn't create a command queue for profiling. "+ex);
  };

  var total_time = 0, time_start, time_end;
  prof_event=new WebCL.WebCLEvent();
  
  for(var i=0;i<NUM_ITERATIONS;i++) {
    /* Enqueue kernel */
    try {
      queue.enqueueNDRangeKernel(kernel, null, [num_items], null, null, prof_event);
    } catch(ex) {
      log("Couldn't enqueue the kernel. "+ex);
      exit(1);   
    }

    /* Finish processing the queue and get profiling information */
    queue.finish();
    
    time_start = prof_event.getProfilingInfo(WebCL.PROFILING_COMMAND_START);
    time_end = prof_event.getProfilingInfo(WebCL.PROFILING_COMMAND_END);
    //log("time: start="+time_start+" end="+time_end);
    total_time += time_end - time_start;
  }

  var avg_ns=Math.round(total_time/NUM_ITERATIONS);
  log("Average time: "+avg_ns+" ns = " +(avg_ns/1000000)+" ms");
  
  log('queue finished');
}

main();
