var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl = require('../webcl');
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
  var /* WebCLEvent */        user_event;
  var /* WebCLBuffer */       data_buffer;

  /* Initialize data */
  var data=new Float32Array(4);
  for(i=0; i<4; i++)
     data[i] = i * 1.0;

  /* Create a device and context */
  log('creating context');
  
  //Pick platform
  var platformList=cl.getPlatforms();
  platform=platformList[0];
  log('using platform: '+platform.getInfo(cl.PLATFORM_NAME));
  
  //Query the set of devices on this platform
  var devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
  device=devices[0];
  log('using device: '+device.getInfo(cl.DEVICE_NAME));

  // create GPU context for this platform
  var context=cl.createContext({
    devices: device, 
    platform: platform
  });

  /* Build the program and create a kernel */
  var source = [
    "__kernel void user_event(__global float4 *v) {",
    "  *v *= -1.0f;",
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
    var info=program.getBuildInfo(devices[0], cl.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  try {
    kernel = program.createKernel("user_event");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(cl.MEM_READ_WRITE | cl.MEM_COPY_HOST_PTR, 4*Float32Array.BYTES_PER_ELEMENT,data);
  } catch(ex) {
    log("Couldn't create a buffer. "+ex);
    exit(1);   
  }

  /* Create kernel argument */
  try {
    kernel.setArg(0, data_buffer);
  } catch(ex) {
    log("Couldn't set a kernel argument. "+ex);
    exit(1);   
  };

  /* Create a command queue */
  try {
    queue = context.createCommandQueue(device, cl.QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
  } catch(ex) {
    log("Couldn't create an out of order command queue, using in-order queue. "+ex);
    queue = context.createCommandQueue(device);
  };

  /* Configure events */
  try {
    user_event = context.createUserEvent();
  } catch(ex) {
     log("Couldn't enqueue the kernel");
     exit(1);   
  }

  /* Enqueue kernel */
  try {
    kernel_event=new cl.WebCLEvent();
    queue.enqueueTask(kernel , [user_event], kernel_event);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read the buffer */
  try {
    read_event=new cl.WebCLEvent();
    queue.enqueueReadBuffer(data_buffer, false, { 
      buffer: data,
      size: data.byteLength
    }, [ kernel_event ], read_event);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  /* Set event handling routines */
  try {
    read_event.setCallback(cl.COMPLETE, read_complete, data);
  } catch(ex) {
    log("Couldn't set callback for read event. "+ex);
    exit(1);   
  }
  
  log("Old data: "+data[0]+', '+data[1]+', '+data[2]+', '+data[3]);

  /* Set user event to success */
  user_event.setUserEventStatus(cl.SUCCESS);
  
  queue.finish();
  log('queue finished');
}

main();