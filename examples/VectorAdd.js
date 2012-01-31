/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var cl=require("../webcl"),
    clu=require("../lib/clUtils.js"),
    log=console.log;
var Int32Array = process.binding('typed_array').Int32Array;
var Uint32Array = process.binding('typed_array').Uint32Array;

//First check if the WebCL extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

VectorAdd();

function VectorAdd() {
  BUFFER_SIZE=10;
  var A=new Uint32Array(BUFFER_SIZE);
  var B=new Uint32Array(BUFFER_SIZE);

  for (var i = 0; i < BUFFER_SIZE; i++) {
    A[i] = i;
    B[i] = i * 2;
  }

  //Pick platform
  var platformList=cl.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getPlatformInfo(cl.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
  log('using device: '+devices[0].getDeviceInfo(cl.DEVICE_NAME));

  // create GPU context for this platform
  context=cl.createContext(cl.DEVICE_TYPE_GPU, [cl.CONTEXT_PLATFORM, platform]);

  kernelSourceCode = [
"__kernel void vadd(__global int *a, __global int *b, __global int *c, uint iNumElements) ",
"{                                                                           ",
"    size_t i =  get_global_id(0);                                           ",
"    if(i >= iNumElements) return;                                           ",
"    c[i] = a[i] + b[i];                                                     ",
"}                                                                           "
].join("\n");

  //Create and program from source
  program=context.createProgram(kernelSourceCode);

  //Build program
  program.build(devices);

  size=BUFFER_SIZE*Uint32Array.BYTES_PER_ELEMENT; // size in bytes
  
  // Create buffer for A and B and copy host contents
  aBuffer = context.createBuffer(cl.MEM_READ_ONLY, size);
  bBuffer = context.createBuffer(cl.MEM_READ_ONLY, size);

  // Create buffer for C to read results
  cBuffer = context.createBuffer(cl.MEM_WRITE_ONLY, size);

  // Create kernel object
  try {
    kernel= program.createKernel("vadd");
  }
  catch(err) {
    console.log(program.getBuildInfo(devices[0],cl.PROGRAM_BUILD_LOG));
  }
  
  // Set kernel args
  kernel.setArg(0, aBuffer, cl.type.MEM);
  kernel.setArg(1, bBuffer, cl.type.MEM);
  kernel.setArg(2, cBuffer, cl.type.MEM);
  kernel.setArg(3, BUFFER_SIZE, cl.type.INT | cl.type.UNSIGNED);

  // Create command queue
  queue=context.createCommandQueue(devices[0], 0);

  // Execute the OpenCL kernel on the list
  var localWS = [5]; // process one list at a time
  var globalWS = [clu.roundUp(localWS, BUFFER_SIZE)]; // process entire list

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);
  
  // Do the work
  queue.enqueueWriteBuffer (aBuffer, false, {
    buffer: A,
    origin: [0],
    size: [A.length*Uint32Array.BYTES_PER_ELEMENT]});

  queue.enqueueWriteBuffer (bBuffer, false, {
    buffer: B,
    origin: [0],
    size: [B.length*Uint32Array.BYTES_PER_ELEMENT]});
  
  // Execute (enqueue) kernel
  log("using enqueueNDRangeKernel");
  queue.enqueueNDRangeKernel(kernel,
      null,
      globalWS,
      localWS);
        
  // get results and block while getting them
  var C=new Uint32Array(BUFFER_SIZE);
  queue.enqueueReadBuffer (cBuffer, true, {
    buffer: C,
    origin: [0], 
    size: [C.length*Uint32Array.BYTES_PER_ELEMENT]});
  
  // print results
  printResults(A,B,C);
}

function printResults(A,B,C) {
  // Print input vectors and result vector
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
