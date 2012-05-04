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
  clu = require('../lib/clUtils');
  log=console.log;
}

var cl = new WebCL();

//First check if the WebCL extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  process.exit(-1);
}

VectorAdd();

function VectorAdd() {
  BUFFER_SIZE=10;
  var A=new Uint32Array(BUFFER_SIZE);
  var B=new Uint32Array(BUFFER_SIZE);
  var C=new Uint32Array(BUFFER_SIZE);

  for (var i = 0; i < BUFFER_SIZE; i++) {
    A[i] = i;
    B[i] = i * 2;
    C[i] = 0;
  }

  //Pick platform
  var platformList=cl.getPlatforms();
  platform=platformList[0];

  //Query the set of devices on this platform
  devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
  log('using device: '+devices[0].getInfo(cl.DEVICE_NAME));

  // create GPU context for this platform
  context=cl.createContext({
	  deviceType: cl.DEVICE_TYPE_GPU, 
	  platform: platform
  });

  kernelSourceCode = [
"__kernel void vadd(__global int *a, __global int *b, __global int *c, uint iNumElements) ",
"{                                                                           ",
"    size_t i =  get_global_id(0);                                           ",
"    if(i > iNumElements) return;                                            ",
"    c[i] = a[i] + b[i];                                                     ",
"}                                                                           "
].join("\n");

  //Create and program from source
  program=context.createProgram(kernelSourceCode);

  //Build program
  program.build(devices,"");

  size=BUFFER_SIZE*Uint32Array.BYTES_PER_ELEMENT; // size in bytes
  
  //Create kernel object
  try {
    kernel= program.createKernel("vadd");
  }
  catch(err) {
    console.log(program.getBuildInfo(devices[0],cl.PROGRAM_BUILD_LOG));
  }
  
  //Create command queue
  queue=context.createCommandQueue(devices[0], 0);

  //Create buffer for A and copy host contents
  aBuffer = context.createBuffer(cl.MEM_READ_ONLY, size);
  map=queue.enqueueMapBuffer(aBuffer, cl.TRUE, cl.MAP_WRITE, 0, BUFFER_SIZE * Uint32Array.BYTES_PER_ELEMENT);
  
  // WARNING: this feature for typed arrays is only in nodejs 0.7.x
  var buf=new Uint32Array(map);
  for(var i=0;i<BUFFER_SIZE;i++) {
    buf.set(i, A[i]);
  }
  queue.enqueueUnmapMemObject(aBuffer, map);

  //Create buffer for B and copy host contents
  bBuffer = context.createBuffer(cl.MEM_READ_ONLY, size);
  map=queue.enqueueMapBuffer(bBuffer, cl.TRUE, cl.MAP_WRITE, 0, BUFFER_SIZE * Uint32Array.BYTES_PER_ELEMENT);
  buf=new Uint32Array(map);
  for(var i=0;i<BUFFER_SIZE;i++) {
    buf[i]=B[i];
  }
  queue.enqueueUnmapMemObject(bBuffer, map);

  //Create buffer for that uses the host ptr C
  cBuffer = context.createBuffer(cl.MEM_READ_WRITE, size);

  //Set kernel args
  kernel.setArg(0, aBuffer);
  kernel.setArg(1, bBuffer);
  kernel.setArg(2, cBuffer);
  kernel.setArg(3, BUFFER_SIZE, cl.type.UINT);

  // Execute the OpenCL kernel on the list
  var localWS = [5]; // process one list at a time
  var globalWS = [clu.roundUp(localWS, BUFFER_SIZE)]; // process entire list

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      null,
      globalWS,
      localWS);
  
  //printResults(A,B,C);

  log("using enqueueMapBuffer");
  // Map cBuffer to host pointer. This enforces a sync with 
  // the host backing space, remember we choose GPU device.
  map=queue.enqueueMapBuffer(
      cBuffer,
      cl.TRUE, // block 
      cl.MAP_READ,
      0,
      BUFFER_SIZE * Uint32Array.BYTES_PER_ELEMENT);

  buf=new Uint32Array(map);
  for(var i=0;i<BUFFER_SIZE;i++) {
    C[i]=buf[i];
  }

  queue.enqueueUnmapMemObject(cBuffer, map);
  
  queue.finish (); //Finish all the operations

  printResults(A,B,C);
}

function printResults(A,B,C) {
  //Print input vectors and result vector
  var output = "\nA = "; 
  for (var i = 0; i < BUFFER_SIZE; i++) {
    output += A[i] + ", ";
  }
  output += "\nB = ";
  for (var i = 0; i < BUFFER_SIZE; i++) {
    output += B[i] + ", ";
  }
  output += "\nC = ";
  for (var i = 0; i < BUFFER_SIZE; i++) {
    output += C[i] + ", ";
  }

  log(output);
}
