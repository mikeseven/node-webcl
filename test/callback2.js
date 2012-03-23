var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl = require('../webcl');
  log = console.log;
  exit = process.exit;
}

/* CL objects */
var /* WebCLPlatform */     platform;
var /* WebCLDevice */       device;
var /* WebCLContext */      context;
var /* WebCLProgram */      program;
var /* WebCLKernel */       kernel;
var /* WebCLCommandQueue */ queue;
var /* WebCLEvent */        kernel_event, read_event;
var /* WebCLBuffer */       data_buffer;
var done=false;

// kernel callback
function kernel_complete(status, data) {
  log('in JS kernel_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);
  log(data);
}

// read buffer callback
function read_complete(status, data) {
  log('in JS read_complete, status: '+status);
  if(status<0) 
    log('Error: '+status);

  var check = cl.TRUE;
  //var str="";
  for(i=0; i<4096; i++) {
    //str+=data[i]+' ';
    if(data[i] != 5.0) {
      check = cl.FALSE;
      break;
    }  
  }
  //log(str);
  if(check)
    log("The data has been initialized successfully.");
  else
    log("The data has not been initialized successfully.");
}

function program_built(err, data) {
  try {
    kernel = program.createKernel("callback");
  } catch(ex) {
    log("Couldn't create a kernel. "+ex);
    exit(1);   
  }

  /* Create a write-only buffer to hold the output data */
  try {
    data_buffer = context.createBuffer(cl.MEM_WRITE_ONLY, 4096*4);
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
    queue = context.createCommandQueue(device, 0);
  } catch(ex) {
    log("Couldn't create a command queue. "+ex);
    exit(1);   
  };

  /* Enqueue kernel */
  try {
    kernel_event=new cl.WebCLEvent();
    queue.enqueueTask(kernel , null, kernel_event);
  } catch(ex) {
    log("Couldn't enqueue the kernel. "+ex);
    exit(1);   
  }

  /* Read the buffer */
  var data=new Float32Array(4096);
  try {
    read_event=new cl.WebCLEvent();
    queue.enqueueReadBuffer(data_buffer, false, { 
      buffer: data,
      origin: [0],
      size: [4096*4]
    }, null, read_event);
  } catch(ex) {
    log("Couldn't read the buffer. "+ex);
    exit(1);   
  }

  /* Set event handling routines */
  try {
    kernel_event.setCallback(cl.COMPLETE, kernel_complete, "The kernel finished successfully.");
  } catch(ex) {
    log("Couldn't set callback for event. "+ex);
    exit(1);   
  }
  read_event.setCallback(cl.COMPLETE, read_complete, data);
  
  queue.finish(); // wait for everything to finish
  done=true;
}
function main() {

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
  context=cl.createContext({
    devices: device, 
    platform: platform
  } ,'Error occured in context', function(err,data){
    log(data+" : "+err);
  });

  /* Build the program and create a kernel */
  var source = [
                "__kernel void callback(__global float *buffer) {",
                "  for(int i=0; i<4096; i++) ",
                "     buffer[i]=5;",
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
    program.build(devices, null, 'compil done', program_built);
  } catch(ex) {
    /* Find size of log and print to std output */
    var info=program.getBuildInfo(devices[0], cl.PROGRAM_BUILD_LOG);
    log(info);
    exit(1);
  }

  log("main app thread END");
  function sleep() {
    if(!done) {
      log('sleeping 0.5s');
      setTimeout(sleep, 500);
    }
  }
  sleep();
}

main();