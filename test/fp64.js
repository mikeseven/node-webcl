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

// options
BUFFER_SIZE=4096;
useGPU=true;

VectorAdd();

function select_FP64_device(devices, useGPU) {
  var i,device;
  useGPU = (typeof useGPU === 'undefined') ? true : useGPU; // select GPU by default
  var isAMD=false;
  for(i=0;i<devices.length;i++) {
    var dev=devices[i];
    if(dev.enableExtension('KHR_fp64')) {
      if(!useGPU || dev.getInfo(webcl.DEVICE_TYPE)==webcl.DEVICE_TYPE_GPU) {
        device=dev;
        if(device.getInfo(webcl.DEVICE_VENDOR).indexOf("Advanced Micro Devices")>=0)
          isAMD=true;
        break;        
      }
    }
  }  
  if(i==devices.length) {
    log("Error: can't find a device that supports 64-bit floats");
    return null;
  }
  log('Found fp64 device: '+device.getInfo(webcl.DEVICE_VENDOR)+device.getInfo(webcl.DEVICE_NAME));
  return [device, isAMD];
}

function VectorAdd() {
  dumpResults=(BUFFER_SIZE<=128);

  var A=new Float64Array(BUFFER_SIZE);
  var B=new Float64Array(BUFFER_SIZE);

  for (var i = 0; i < BUFFER_SIZE; i++) {
    A[i] = i;
    B[i] = i * 2;
  }

  var platformList=webcl.getPlatforms();
  platform=platformList[0];
  log('Using platform: '+platform.getInfo(webcl.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  var devices = platform.getDevices(webcl.DEVICE_TYPE_ALL);

  // find a device that supports fp64
  // Note: Intel integrated GPU HD 4000 doesn't support fp64
  var dev=select_FP64_device(devices, useGPU);
  if(!dev)
    process.exit(-1);

  device=dev[0];
  isAMD=dev[1];

  // create GPU context for this platform
  var context=webcl.createContext(device);

  // Create command queue
  queue=context.createCommandQueue();
  
  kernelSourceCode = [
"__kernel void vadd(__global double *a, __global double *b, __global double *c, uint iNumElements) ",
"{                                                                           ",
"    size_t i =  get_global_id(0);                                           ",
"    if(i >= iNumElements) return;                                           ",
"    c[i] = a[i] + b[i];                                                     ",
"}                                                                           "
].join("\n");

  if(isAMD) 
    kernelSourceCode = "#pragma OPENCL EXTENSION cl_amd_fp64 : enable\n" + kernelSourceCode;
  else
    kernelSourceCode = "#pragma OPENCL EXTENSION cl_khr_fp64 : enable\n" + kernelSourceCode;

  //Create and program from source
  program=context.createProgram(kernelSourceCode);

  //Build program
  try {
    program.build(device);
  }
  catch(ex) {
    log(ex);
    log(program.getBuildInfo(device,webcl.PROGRAM_BUILD_LOG));
  }

  size=BUFFER_SIZE*Float64Array.BYTES_PER_ELEMENT; // size in bytes
  
  // Create buffer for A and B and copy host contents
  aBuffer = context.createBuffer(webcl.MEM_READ_ONLY, size);
  bBuffer = context.createBuffer(webcl.MEM_READ_ONLY, size);

  // Create buffer for C to read results
  cBuffer = context.createBuffer(webcl.MEM_WRITE_ONLY, size);

  // Create kernel object
  try {
    kernel= program.createKernel("vadd");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,webcl.PROGRAM_BUILD_LOG));
  }

  // Set kernel args
  kernel.setArg(0, aBuffer);
  kernel.setArg(1, bBuffer);
  kernel.setArg(2, cBuffer);
  kernel.setArg(3, new Uint32Array([BUFFER_SIZE]));

  // Execute the OpenCL kernel on the list
  var localWS = [8]; // process one list at a time
  var globalWS = [clu.roundUp(localWS, BUFFER_SIZE)]; // process entire list

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);
  
  // Do the work
  queue.enqueueWriteBuffer (aBuffer, false, 0, A.length*Float64Array.BYTES_PER_ELEMENT, A);
  queue.enqueueWriteBuffer (bBuffer, false, 0, B.length*Float64Array.BYTES_PER_ELEMENT, B);

  // Execute (enqueue) kernel
  log("execute kernel");
  queue.enqueueNDRangeKernel(kernel, 1, 
      null,
      globalWS,
      localWS);

  // get results and block while getting them
  var C=new Float64Array(BUFFER_SIZE);
  queue.enqueueReadBuffer (cBuffer, true, 0, C.length*Float64Array.BYTES_PER_ELEMENT, C);

  // print results
  printResults(A,B,C);
}

function printResults(A,B,C) {
  // Print input vectors and result vector
  var i, ok=true;
  if(dumpResults) {
    var output = "\nA = "; 
    for (i = 0; i < BUFFER_SIZE; i++) {
      output += A[i] + ", ";
    }
    output += "\nB = ";
    for (i = 0; i < BUFFER_SIZE; i++) {
      output += B[i] + ", ";
    }
    output += "\nC = ";
    for (i = 0; i < BUFFER_SIZE; i++) {
      output += C[i] + ", ";
      if(C[i] != A[i]+B[i]) {
        ok=false;
        break;
      }
    }
    log(output);
  }
  else {
    for (i = 0; i < BUFFER_SIZE; i++) {
      if(C[i] != A[i]+B[i]) {
        ok=false;
        break;
      }
    }
  }
  log(ok ? "PASS" : "FAIL: incorrect vector addition");
}
