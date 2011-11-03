/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var WebCL=require("../../../lib/webcl"),
sys=require('util'),
clu=require('../../../lib/clUtils.js');

var log=console.log;

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

//Pick platform
var platformList=WebCL.getPlatforms();
log("Number of platforms: \t"+platformList.length); 
platform=platformList[0];

//Pick device
devices = platform.getDevices(WebCL.CL_DEVICE_TYPE_ALL);
var numDevices=devices.length;
log("Number of devices in platform 0: \t"+numDevices); 
device=devices[0];

//Create context
context=WebCL.createContext(devices, [WebCL.CL_CONTEXT_PLATFORM, platform]);

kernelSourceCode = [
"// simple.cl",
"//",
"//    This is a simple example demonstrating buffers and sub-buffer usage",
"",
"__kernel void square(__global int* buffer)",
"{",
"  size_t id = get_global_id(0);",
"  buffer[id] = buffer[id] * buffer[id];",
"}"
].join("\n");

//Create program from source
program=context.createProgram(kernelSourceCode);

//Build program
try {
  program.build(devices,"-I.");
}
catch(err) {
  console.log(program.getBuildInfo(device,WebCL.CL_PROGRAM_BUILD_LOG));
}

var NUM_BUFFER_ELEMENTS = 20;
var size=NUM_BUFFER_ELEMENTS*numDevices, sizeBytes=size * Int32Array.BYTES_PER_ELEMENT;
var inputOutput=[];
for(var i=0;i<size;i++)
  inputOutput[i]=i;
log("InputOutput: "+inputOutput);

var buffers=[];

// create a single buffer to cover all the input data
var buffer=context.createBuffer(WebCL.CL_MEM_READ_WRITE, sizeBytes);
buffers.push(buffer);

// now for all devices other than the first create a sub-buffer
for(var i=0;i<numDevices;i++) {
  buffers.push(buffer.createSubBuffer(WebCL.CL_MEM_READ_WRITE,WebCL.CL_BUFFER_CREATE_TYPE_REGION,{
    origin:NUM_BUFFER_ELEMENTS * i * Int32Array.BYTES_PER_ELEMENT,
    size: NUM_BUFFER_ELEMENTS * Int32Array.BYTES_PER_ELEMENT
  }));
}

// Create command queues
var queues=[];
var kernels=[];

for(var i=0;i<numDevices;i++) {
  var queue=context.createCommandQueue(devices[i], 0);
  queues.push(queue);
  
  //Create kernel object
  try {
    kernel= program.createKernel("square");
    kernel.setArg(0,buffers[i],WebCL.type.MEM);
    kernels.push(kernel);
  }
  catch(err) {
    console.log(program.getBuildInfo(devices[i],WebCL.CL_PROGRAM_BUILD_LOG));
    return;
  }
}

var map=queues[0].enqueueMapBuffer(buffers[0], WebCL.CL_TRUE, WebCL.CL_MAP_WRITE, 0, sizeBytes);
var buf=map.getBuffer();
var ibuf=new Int32Array(buf,0);
for(var i=0;i<size;i++)
  ibuf[i]=inputOutput[i];

log("mapped buffer:")
for(var i=0;i<size;i++) {
  log(ibuf[i]);
}
for(var i=0;i<sizeBytes;i++) {
  log(buf[i]);
}

queues[0].enqueueUnmapMemObject(buffers[0], map);

// call kernel for each device
var events=[];
for(var i=0;i<queues.length;i++) {
  var event=queues[i].enqueueNDRangeKernel(kernels[i],
      [],
      [NUM_BUFFER_ELEMENTS],
      []);
  events.push(event);
}
log("events: "+events);

// Technically don't need this as we are doing a blocking read with in-order queue.
// TODO ERROR??? WebCL.waitForEvents(events);

var map=queues[0].enqueueMapBuffer(buffers[0], WebCL.CL_TRUE, WebCL.CL_MAP_READ, 0, sizeBytes);
var ibuf=new Int32Array(map.getBuffer(),0);
for(var i=0;i<size;i++) {
  inputOutput[i]=ibuf[i];
}
queues[0].enqueueUnmapMemObject(buffers[0], map);
queues[0].finish();

log("InputOutput: "+inputOutput);
