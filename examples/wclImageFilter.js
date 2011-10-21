var sys    = require("util"),
    fs     = require("fs"),
    Buffer = require('buffer').Buffer,
    file   = __dirname + "/NeHe.bmp",
    textureBuffer,
    image = { width: 256, height: 256, size: 256*256*3, texture: -1};

textureBuffer = fs.readFileSync(file);
image.buffer = textureBuffer.slice(54, textureBuffer.length);
console.log("file size: "+textureBuffer.length+" image buffer size: "+image.buffer.length);

// reverse all of the colors. (bgr -> rgb)
for (var i=0;i<image.buffer.length;i+=3) {
  var temp = image.buffer[i];
  image.buffer[i] = image.buffer[i+2];
  image.buffer[i+2] = temp;
}

/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var WebCL=require("../lib/webcl"),
sys=require('util'),
clu=require('../lib/clUtils.js');

var log=console.log;

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

ImageFilter();

function ImageFilter() {
  //Pick platform
  var platformList=WebCL.getPlatformIDs();
  platform=platformList[0];

  //Pick first platform
  context=new WebCL.WebCLContext(WebCL.CL_DEVICE_TYPE_GPU, [WebCL.CL_CONTEXT_PLATFORM, platform]);

  //Query the set of devices attched to the context
  devices = context.getInfo(WebCL.CL_CONTEXT_DEVICES);
  
  kernelSourceCode = fs.readFileSync(__dirname+'/swapRB.cl');
  
  //Create and program from source
  program=new WebCL.WebCLProgram(context, kernelSourceCode);

  //Build program
  program.build(devices,"");

  //Create kernel object
  try {
    kernel= program.createKernel("swapRB");
  }
  catch(err) {
    console.log(program.getBuildInfo(devices[0],WebCL.CL_PROGRAM_BUILD_LOG));
  }

  // Create the input and output arrays in device memory for our calculation
  var vert = new Float32Array(9);//[ [1.0, 0.0, -1.0], [2.0, 0.0, -2.0], [1.0, 0.0, -1.0]];
  vert[0]=1; vert[1]=0; vert[2]=-1;
  vert[3]=2; vert[4]=0; vert[5]=-2;
  vert[6]=1; vert[7]=0; vert[8]=-1;
  var horz = new Float32Array(9);//[ [1.0, 2.0, 1.0], [0.0, 0.0, 0.0], [-1.0, -2.0, -1.0]];
  horz[0]=1; horz[1]=2; horz[2]=1;
  horz[3]=0; horz[4]=0; horz[5]=0;
  horz[6]=-1; horz[7]=-2; horz[8]=-1;
  var COEFS_SIZE=4*(3*3);

  input = context.createBuffer(WebCL.CL_MEM_READ_ONLY,image.size);
  output = context.createBuffer(WebCL.CL_MEM_WRITE_ONLY, image.size);
  coefs_ver = context.createBuffer(WebCL.CL_MEM_READ_ONLY,  COEFS_SIZE);
  coefs_hor = context.createBuffer(WebCL.CL_MEM_READ_ONLY,  COEFS_SIZE);

  //Create command queue
  queue=context.createCommandQueue(devices[0], 0);

  // Write our data set into the input array in device memory 
  queue.enqueueWriteBuffer(input, true, 0, image.size, image.buffer, []);
  queue.enqueueWriteBuffer(coefs_ver, true, 0, COEFS_SIZE, vert, []);
  queue.enqueueWriteBuffer(coefs_hor, true, 0, COEFS_SIZE, horz, []);

  // Set the arguments to our compute kernel
  kernel.setArg(0, input, WebCL.types.MEM);
  kernel.setArg(1, output, WebCL.types.MEM);
  kernel.setArg(2, image.width, WebCL.types.UINT);
  kernel.setArg(3, image.height, WebCL.types.UINT);
  /*kernel.setArg(4, coefs_ver, WebCL.types.MEM);
  kernel.setArg(5, coefs_hor, WebCL.types.MEM);
  // Shared memory
  kernel.setArg(6, COEFS_SIZE, -1);
  kernel.setArg(7, COEFS_SIZE, -1);*/

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS=kernel.getWorkGroupInfo(devices[0], WebCL.CL_KERNEL_WORK_GROUP_SIZE);
  var globalWS = [Math.ceil (image.width*image.height / localWS) * localWS];
  //var globalWS = image.width * image.height;

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel,
      [],
      [globalWS],
      [localWS]);
  
  queue.finish (); //Finish all the operations

  // Read back the results from the device to verify the output
  log("image.buffer.length = "+image.buffer.length);
  var out=new Buffer(image.size);
  queue.enqueueReadBuffer(output, true, 0, image.size, out, [] );

  // save image
  fs.unlinkSync("out.ppm");
  var file=fs.openSync("out.ppm", "a");
  var header="P6\n"+image.width+" "+image.height+"\n255\n";
  fs.writeSync(file,header,0,header.length,null);
  /*for (var i=0;i<out.length;i+=3) {
    var temp = out[i];
    out[i] = out[i+2];
    out[i+2] = temp;
  }*/
  fs.writeSync(file,out,0,out.length,null);
  fs.closeSync(file);
}


