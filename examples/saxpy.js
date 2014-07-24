var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL = require('../webcl');
  log=console.log;
}
else
  WebCL = window.webcl;

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  process.exit(-1);
}

saxpy();

function get_event_exec_time(event)
{
    var start_time = event.getProfilingInfo (WebCL.PROFILING_COMMAND_START);
    var end_time=event.getProfilingInfo (WebCL.PROFILING_COMMAND_END);

    return (end_time - start_time) * 1e-6;
}

function saxpy() {
  var VECTOR_SIZE = 512*1024;
  log("SAXPY with vector size: "+VECTOR_SIZE+" elements");

  var alpha = 2.0;
  var A=new Float32Array(VECTOR_SIZE), B=new Float32Array(VECTOR_SIZE), C=new Float32Array(VECTOR_SIZE);
  for(var i = 0; i < VECTOR_SIZE; i++)
  {
      A[i] = i;
      B[i] = (VECTOR_SIZE - i);
      C[i] = 0;
  }

  // create GPU context for this platform
  context=WebCL.createContext(WebCL.DEVICE_TYPE_GPU);

  // Create command queue
  try {
    queue=context.createCommandQueue(WebCL.QUEUE_PROFILING_ENABLE);
  }
  catch(ex) {
    log(ex);
    exit(-1);
  }
  
  device = queue.getInfo(WebCL.QUEUE_DEVICE);
  log('using device: '+device.getInfo(WebCL.DEVICE_NAME));

  var saxpy_kernel = [
    "__kernel                             ",
    "void saxpy_kernel(float alpha,       ",
    "                  __global float *A, ",
    "                  __global float *B, ",
    "                  __global float *C) ",
    "{                                    ",
    "    int idx = get_global_id(0);      ",
    "    C[idx] = alpha* A[idx] + B[idx]; ",
    "}                                    ",
  ].join("\n");

  //Create and program from source
  program=context.createProgram(saxpy_kernel);

  //Build program
  program.build(device);

  size=VECTOR_SIZE*Float32Array.BYTES_PER_ELEMENT; // size in bytes
  
  // Create buffer for A and B and copy host contents
  var aBuffer = context.createBuffer(WebCL.MEM_READ_ONLY, size);
  var bBuffer = context.createBuffer(WebCL.MEM_READ_ONLY, size);

  // Create buffer for C to read results
  var cBuffer = context.createBuffer(WebCL.MEM_WRITE_ONLY, size);

  // Create kernel object
  try {
    kernel= program.createKernel("saxpy_kernel");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,WebCL.PROGRAM_BUILD_LOG));
  }
  
  // Set kernel args
  kernel.setArg(0, new Float32Array(alpha));
  kernel.setArg(1, aBuffer);
  kernel.setArg(2, bBuffer);
  kernel.setArg(3, cBuffer);

  // Do the work
  var write_events=[new WebCL.WebCLEvent(), new WebCL.WebCLEvent()];
  queue.enqueueWriteBuffer (aBuffer, false, 0, size, A, null, write_events[0]);
  queue.enqueueWriteBuffer (bBuffer, false, 0, size, B, null, write_events[1]);

  // Execute (enqueue) kernel
  var localWS = [64]; // process one list at a time
  var globalWS = [VECTOR_SIZE]; // process entire list
  var kernel_event=new WebCL.WebCLEvent();

  queue.enqueueNDRangeKernel(kernel, 1, null, globalWS, localWS, write_events, kernel_event);

  // get results and block while getting them
  var read_event=new WebCL.WebCLEvent();
  queue.enqueueReadBuffer (cBuffer, true, 0, size, C, [kernel_event], read_event);

  queue.finish();

  // get all event statistics
  log("Time to transfer matrix A: "+get_event_exec_time(write_events[0])+" ms");
  log("Time to transfer matrix B: "+get_event_exec_time(write_events[1])+" ms");
  log("Time to execute SAXPY kernel: "+get_event_exec_time(kernel_event)+" ms");
  log("Time to read matrix C: "+get_event_exec_time(read_event)+" ms");

  // cleanup
  var err = kernel.release();
  err |= program.release();
  err |= aBuffer.release();
  err |= bBuffer.release();
  err |= cBuffer.release();
  err |= queue.release();
  err |= context.release();
  if(err!=WebCL.SUCCESS)
    log("WebCL release failed");
}

