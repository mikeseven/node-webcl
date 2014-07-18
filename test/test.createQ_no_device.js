var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL = require('../webcl');
  clu = require('../lib/clUtils');
  log=console.log;
}
else
  WebCL = window.webcl;

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
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

  // create GPU context for this platform
  context=WebCL.createContext(WebCL.DEVICE_TYPE_DEFAULT);

  // Create command queue
  try {
    queue=context.createCommandQueue();
  }
  catch(ex) {
    log(ex);
    exit(-1);
  }
  
  device = queue.getInfo(WebCL.QUEUE_DEVICE);
  log('using device: '+device.getInfo(WebCL.DEVICE_NAME));

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
  program.build(device);

  size=BUFFER_SIZE*Uint32Array.BYTES_PER_ELEMENT; // size in bytes
  
  // Create buffer for A and B and copy host contents
  aBuffer = context.createBuffer(WebCL.MEM_READ_ONLY, size);
  bBuffer = context.createBuffer(WebCL.MEM_READ_ONLY, size);

  // Create buffer for C to read results
  cBuffer = context.createBuffer(WebCL.MEM_WRITE_ONLY, size);

  // Create kernel object
  try {
    kernel= program.createKernel("vadd");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,WebCL.PROGRAM_BUILD_LOG));
  }
  
  // Set kernel args
  kernel.setArg(0, aBuffer);
  kernel.setArg(1, bBuffer);
  kernel.setArg(2, cBuffer);
  kernel.setArg(3, new Uint32Array([BUFFER_SIZE]));

  // Execute the OpenCL kernel on the list
  var localWS = [5]; // process one list at a time
  var globalWS = [clu.roundUp(localWS, BUFFER_SIZE)]; // process entire list

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);
  
  // Do the work
  queue.enqueueWriteBuffer (aBuffer, false, 0, A.length*Uint32Array.BYTES_PER_ELEMENT, A);
  queue.enqueueWriteBuffer (bBuffer, false, 0, B.length*Uint32Array.BYTES_PER_ELEMENT, B);

  // Execute (enqueue) kernel
  log("using enqueueNDRangeKernel");
  queue.enqueueNDRangeKernel(kernel, 1, 
      null,
      globalWS,
      localWS);

  // get results and block while getting them
  var C=new Uint32Array(BUFFER_SIZE);
  queue.enqueueReadBuffer (cBuffer, true, 0, C.length*Uint32Array.BYTES_PER_ELEMENT, C);

  // print results
  printResults(A,B,C);

  // cleanup
  // test release each CL object
  // queue.release();
  // kernel.release();
  // program.release();
  // aBuffer.release();
  // bBuffer.release();
  // cBuffer.release();
  // context.release();

  // test release all CL objects
  // WebCL.releaseAll();

  // if no manual cleanup specified, WebCL.releaseAll() is called at exit of program
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
