var nodejs = (typeof window === 'undefined');
var assert=require('assert');

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

  var check = webcl.TRUE;
  //var str="";
  for(i=0; i<BUFFER_SIZE; i++) {
    //str+=data[i]+' ';
    if(data[i] != i+i*2) {
      check = webcl.FALSE;
      break;
    }  
  }
  //log(str);
  if(check)
    log("The data has been computed successfully.");
  else
    log("The data has NOT been computed successfully.");
}

function VectorAdd() {
  BUFFER_SIZE=10;
  var A=new Uint32Array(BUFFER_SIZE);
  var B=new Uint32Array(BUFFER_SIZE);

  for (var i = 0; i < BUFFER_SIZE; i++) {
    A[i] = i;
    B[i] = i * 2;
  }

  // create GPU context for this platform
  context=webcl.createContext(webcl.DEVICE_TYPE_DEFAULT);

  // Create command queue
  try {
    queue=context.createCommandQueue();
  }
  catch(ex) {
    log(ex);
    exit(-1);
  }
  
  device = queue.getInfo(webcl.QUEUE_DEVICE);
  log('using device: '+device.getInfo(webcl.DEVICE_NAME));

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

  // get a bunch of references
  var ctx = kernel.getInfo(webcl.KERNEL_CONTEXT);
  assert(ctx === context);

  var prog=kernel.getInfo(webcl.KERNEL_PROGRAM);
  assert(prog === program);

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

  // get more references
  var qctx = queue.getInfo(webcl.QUEUE_CONTEXT);
  assert(qctx === context);
  var qdev=queue.getInfo(webcl.QUEUE_DEVICE);
  assert(qdev === device);

  // Execute (enqueue) kernel
  log("using enqueueNDRangeKernel");
  kernel_event=new webcl.WebCLEvent();
  queue.enqueueNDRangeKernel(kernel, 1, 
      null,
      globalWS,
      localWS,
      null,
      kernel_event);

  kernel_event.setCallback(webcl.COMPLETE, kernel_complete, "The kernel finished successfully.");

  // get results and block while getting them
  var C=new Uint32Array(BUFFER_SIZE);
  read_event=new webcl.WebCLEvent();
  queue.enqueueReadBuffer (cBuffer, true, 0, C.length*Uint32Array.BYTES_PER_ELEMENT, C, null, read_event);
  read_event.setCallback(webcl.COMPLETE, read_complete, C);

  // more references
  var ectx=read_event.getInfo(webcl.EVENT_CONTEXT);
  assert(ectx === context);
  var eq=read_event.getInfo(webcl.EVENT_COMMAND_QUEUE);
  assert(eq === queue);

  // print results
  printResults(A,B,C);

  // cleanup
  // test release each CL object
  // queue.release();
  kernel.release();
  program.release();
  // aBuffer.release();
  // bBuffer.release();
  // cBuffer.release();
  // context.release();

  // test release all CL objects
  webcl.releaseAll();

  // if no manual cleanup specified, webcl.releaseAll() is called at exit of program
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
