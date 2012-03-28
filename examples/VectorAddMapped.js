/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl = require('../webcl');
  clu = require('../lib/clUtils');
  log=console.log;
}

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
