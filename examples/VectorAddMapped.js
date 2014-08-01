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
  clu = require('../lib/clUtils');
  log=console.log;
}
else
  webcl = window.webcl;

//First check if the webcl extension is installed at all 
if (webcl == undefined) {
  alert("Unfortunately your system does not support webcl. " +
  "Make sure that you have the webcl extension installed.");
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
    C[i] = 10;
  }

  // create GPU context for this platform
  var context=null;
  try {
    context=webcl.createContext(webcl.DEVICE_TYPE_GPU);
  }
  catch(ex) {
    throw new Exception("Can't create CL context");
  }

  var devices=context.getInfo(webcl.CONTEXT_DEVICES);
  device=devices[0];

  log('using device: '+device.getInfo(webcl.DEVICE_VENDOR).trim()+
    ' '+device.getInfo(webcl.DEVICE_NAME));

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

  var size=BUFFER_SIZE*Uint32Array.BYTES_PER_ELEMENT; // size in bytes
  
  //Create kernel object
  try {
    kernel= program.createKernel("vadd");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,webcl.PROGRAM_BUILD_LOG));
  }
  
  //Create command queue
  queue=context.createCommandQueue(device, 0);

  //Create buffer for A and copy host contents
  aBuffer = context.createBuffer(webcl.MEM_READ_ONLY | webcl.MEM_USE_HOST_PTR, size, A);

  //Create buffer for B and copy host contents
  bBuffer = context.createBuffer(webcl.MEM_READ_ONLY | webcl.MEM_USE_HOST_PTR, size, B);

  //Create buffer for that uses the host ptr C
  cBuffer = context.createBuffer(webcl.MEM_WRITE_ONLY | webcl.MEM_USE_HOST_PTR, size, C);

  //Set kernel args
  kernel.setArg(0, aBuffer);
  kernel.setArg(1, bBuffer);
  kernel.setArg(2, cBuffer);
  kernel.setArg(3, new Uint32Array([BUFFER_SIZE]));

  // Execute the OpenCL kernel on the list
  // var localWS = [5]; // process one list at a time
  // var globalWS = [clu.roundUp(localWS, BUFFER_SIZE)]; // process entire list
  var localWS=null;
  var globalWS=[BUFFER_SIZE];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel, 1,
      null,
      globalWS,
      localWS);
  
  log("using enqueueMapBuffer");
  // Map cBuffer to host pointer. This enforces a sync with 
  // the host backing space, remember we choose GPU device.
  var map=queue.enqueueMapBuffer(
      cBuffer,
      webcl.TRUE, // block 
      webcl.MAP_READ,
      0,
      size);

  var output;
  output="after map C= ";
  for (var i = 0; i < BUFFER_SIZE; i++) {
    output += C[i] + ", ";
  }
  log(output);

  // we are now reading values as bytes, we need to cast it to the output type we want
  output = "output = ";
  for (var i = 0; i < size; i++) {
    output += map[i] + ", ";
  }
  log(output);

  queue.enqueueUnmapMemObject(cBuffer, map);
  
  output="after unmap C= ";
  for (var i = 0; i < BUFFER_SIZE; i++) {
    output += C[i] + ", ";
  }
  log(output);

  queue.finish(); //Finish all the operations

  printResults(A,B,C);

  // cleanup
  // queue.release();
  // kernel.release();
  // program.release();
  // aBuffer.release();
  // bBuffer.release();
  // cBuffer.release();
  // context.release();
  //webcl.releaseAll();
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
