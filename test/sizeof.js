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

(function main() {
  /* CL objects */
  var /* WebCLPlatform */     platform;
  var /* WebCLDevice */       device;
  var /* WebCLContext */      context;
  var /* WebCLProgram */      program;
  var /* WebCLKernel */       kernel;
  var /* WebCLCommandQueue */ queue;
  var /* WebCLBuffer */       data_buffer, ret_buffer;
  var /* ArrayBuffer */       mapped_memory;
  
  var NUM_ELEMS = 256; 
  
  /* Initialize data */
  var data=new Uint32Array(NUM_ELEMS);
  var ret=new Uint32Array(1);

  /* Create a device and context */
  log('creating context');
  
  // //Pick platform
  // var platformList=WebCL.getPlatforms();
  // platform=platformList[0];
  // log('using platform: '+platform.getInfo(WebCL.PLATFORM_NAME));
  
  // //Query the set of devices on this platform
  // var devices = platform.getDevices(WebCL.DEVICE_TYPE_DEFAULT);
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
      deviceType: WebCL.DEVICE_TYPE_GPU, 
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
    "typedef struct {",
    "  float3 origin;", // 16
    "  float r;",       // 4
    "  float2 dis;",    // 8
    "} Sphere;",        // 28 but aligned to 32
    "         ",
    "typedef struct {",
    "  float3 origin;",     // 16
    "  float3 dir;",        // 16
    "  float3 nor;",        // 16
    "  float4 col;",        // 16
    "  float fovfactor;",   // 4
    "  float t;",           // 4
    "  float3 rgb;",        // 16
    "  Sphere sph;",        // 32
    "} __attribute__((aligned(16))) RayAligned;", // 120 aligned to 128
    "         ",
    "typedef struct {",
    "  float3 origin;",
    "  float3 dir;",
    "  float3 nor;",
    "  float4 col;",
    "  float fovfactor;",
    "  float t;",
    "  float3 rgb;",
    "  Sphere sph;",
    "} Ray;", // should be 128
    "         ",
    "__kernel void ksizeof(__global uint *c, uint num, __global uint *ret) {",
    "  *ret=0;",
    "         ",
    "  c[(*ret)++]=sizeof(bool);",
    "  c[(*ret)++]=sizeof(char);",
    "  c[(*ret)++]=sizeof(char3);",
    "  c[(*ret)++]=sizeof(char3);",
    "  c[(*ret)++]=sizeof(char4);",
    "  c[(*ret)++]=sizeof(short);",
    "  c[(*ret)++]=sizeof(short2);",
    "  c[(*ret)++]=sizeof(short3);",
    "  c[(*ret)++]=sizeof(short4);",
    "  c[(*ret)++]=sizeof(int);",
    "  c[(*ret)++]=sizeof(int2);",
    "  c[(*ret)++]=sizeof(int3);",
    "  c[(*ret)++]=sizeof(int4);",
    "  c[(*ret)++]=sizeof(long);",
    "  c[(*ret)++]=sizeof(long2);",
    "  c[(*ret)++]=sizeof(long3);",
    "  c[(*ret)++]=sizeof(long4);",
    "  c[(*ret)++]=sizeof(float);",
    "  c[(*ret)++]=sizeof(float2);",
    "  c[(*ret)++]=sizeof(float3);",
    "  c[(*ret)++]=sizeof(float4);",
    "  c[(*ret)++]=sizeof(Sphere);",
    "  c[(*ret)++]=sizeof(Ray);",
    "  c[(*ret)++]=sizeof(RayAligned);",
    "}"
  ].join("\n");

  var itemName=['bool',
                'char','char2','char3','char4',
                'short','short2','short3','short4',
                'int','int2','int3','int4',
                'long','long2','long3','long4',
                'float','float2','float3','float4',
                'Sphere',
                'Ray','RayAligned'
                ];
  
  var itemGoldValue=[1,
                1,4,4,4,
                2,4,8,8,
                4,8,16,16,
                8,16,32,32,
                4,8,16,16,
                32,
                128,128
                ];
  
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
    kernel = program.createKernel("ksizeof");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(WebCL.MEM_WRITE_ONLY, NUM_ELEMS*Uint32Array.BYTES_PER_ELEMENT);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }
  
  try {
    ret_buffer = context.createBuffer(WebCL.MEM_WRITE_ONLY, 1*Uint32Array.BYTES_PER_ELEMENT);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }

  /* Create kernel argument */
  try {
    kernel.setArg(0, data_buffer);
    kernel.setArg(1, NUM_ELEMS, WebCL.type.UINT); /* Tell kernel number of elements */
    kernel.setArg(2, ret_buffer); /* Pass pointer to returned number of values */

  } catch(ex) {
    log("Couldn't set a kernel argument. "+ex);
    exit(1);   
  }

  /* Create a command queue */
  try {
    queue = context.createCommandQueue(device);
  } catch(ex) {
    log("Couldn't create a command queue for profiling. "+ex);
  }

  /* Enqueue kernel */
  try {
    queue.enqueueTask(kernel);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read number of elements returned in buffer */
  try {
    queue.enqueueReadBuffer(ret_buffer, true, 0, ret.byteLength, ret);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  log("Returned "+ret[0]+" values");
  
  /* Read the buffer */
  try {
    queue.enqueueReadBuffer(data_buffer, true, 0, data.byteLength, data);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  for(var i=0;i<ret[0];++i) {
    log(itemName[i]+" must be "+itemGoldValue[i]+" found: "+data[i]+(itemGoldValue[i]==data[i] ? ' OK': ' FAIL'));
  }
    
  queue.finish();
  log('queue finished');
})()
