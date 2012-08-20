// Copyright (c) 2011-2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Multiple bandwidth tests

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL = require('../webcl');
  clu = require('../lib/clUtils');
  log=console.log;
}

//defines, project
var MEMCOPY_ITERATIONS  = 100;
var DEFAULT_SIZE        = ( 10 * ( 1 << 20 ) );    // 10 M
var DEFAULT_INCREMENT   = (1 << 22);               //4 M
var CACHE_CLEAR_SIZE    = (1 << 24);               //16 M

//enums, project
var QUICK_MODE=0, RANGE_MODE=1; // test modes
var DEVICE_TO_HOST=0, HOST_TO_DEVICE=1, DEVICE_TO_DEVICE=2; // memory copy kind
var PAGEABLE=0, PINNED=1; // memory modes
var MAPPED=0, DIRECT=1; // access modes

//First check if the WebCL extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

// Create the OpenCL context
var ctx=null;
try {
  ctx=WebCL.createContext({
    deviceType: WebCL.DEVICE_TYPE_ALL, 
  });
}
catch(ex) {
  throw new Exception("Can't create CL context");
}

var devices=ctx.getInfo(WebCL.CONTEXT_DEVICES);
log("Found "+devices.length+" devices");

devices.forEach(function(d) {
  d.units=d.getInfo(WebCL.DEVICE_MAX_COMPUTE_UNITS);
  d.clock=d.getInfo(WebCL.DEVICE_MAX_CLOCK_FREQUENCY);
  //var timerRes=d.getInfo(WebCL.DEVICE_PROFILING_TIMER_RESOLUTION);
  d.type=d.getInfo(WebCL.DEVICE_TYPE);
  d.endian=(d.getInfo(WebCL.DEVICE_ENDIAN_LITTLE) ? "LITTLE" : "BIG");
  d.name=d.getInfo(WebCL.DEVICE_NAME);
  
  if(d.type==WebCL.DEVICE_TYPE_CPU) d.type="CPU";
  else if(d.type==WebCL.DEVICE_TYPE_GPU) d.type="GPU";
  else if(d.type==WebCL.DEVICE_TYPE_ACCELERATOR) d.type="ACCELERATOR";
  else d.type="DEFAULT";

  var flops=d.units * d.clock;
  log("  Device "+d.type+" "+d.name+": "+d.units+" CU @ "+d.clock+" MHz, " +
      d.endian+" endian");
});

// Run tests
var start=DEFAULT_SIZE; // 1 MB
var end=2*DEFAULT_SIZE;  // 20 MB
var increment = DEFAULT_INCREMENT;
var mode = RANGE_MODE;
var accMode = DIRECT;
var memMode = PAGEABLE;
var startDevice=0;
var endDevice=devices.length-1;

var cqCommandQueue=null;
testBandwidth(start, end, increment, mode, HOST_TO_DEVICE, accMode, memMode, startDevice, endDevice);
testBandwidth(start, end, increment, mode, DEVICE_TO_HOST, accMode, memMode, startDevice, endDevice);
testBandwidth(start, end, increment, mode, DEVICE_TO_DEVICE, accMode, memMode, startDevice, endDevice);


function
createQueue(device)
{
    // Release if there previous is already one
    //if(cqCommandQueue) {
    //  cqCommandQueue.release();
    //}
  
    cqCommandQueue = ctx.createCommandQueue(devices[device], WebCL.QUEUE_PROFILING_ENABLE);
}
  
///////////////////////////////////////////////////////////////////////////////
//  Run a bandwidth test
///////////////////////////////////////////////////////////////////////////////
function
testBandwidth(start, end, increment, 
              mode, kind, printmode, accMode, 
              memMode, startDevice, endDevice)
{
    switch(mode)
    {
    case QUICK_MODE:
        testBandwidthQuick( DEFAULT_SIZE, kind, printmode, accMode, memMode, startDevice, endDevice);
        break;
    case RANGE_MODE:
        testBandwidthRange(start, end, increment, kind, printmode, accMode, memMode, startDevice, endDevice);
        break;
    //case SHMOO_MODE: 
    //    testBandwidthShmoo(kind, printmode, accMode, memMode, startDevice, endDevice);
    //    break;
    default:  
        break;
    }

}

//////////////////////////////////////////////////////////////////////
//Run a quick mode bandwidth test
//////////////////////////////////////////////////////////////////////
function
testBandwidthQuick(size, kind, printmode, accMode, 
                    memMode, startDevice, endDevice)
{
 testBandwidthRange(size, size, DEFAULT_INCREMENT, kind, printmode, accMode, memMode, startDevice, endDevice);
}

///////////////////////////////////////////////////////////////////////
//Run a range mode bandwidth test
//////////////////////////////////////////////////////////////////////
function testBandwidthRange(start, end, increment, 
                            memCpyKind, accMode, memMode, 
                            startDevice, endDevice)
{
  //count the number of copies we're going to run
  var count = 1 + ((end - start) / increment);

  var memSizes=new Array();
  var bandwidths=new Array();

  // Before calculating the cumulative bandwidth, initialize bandwidths array to NULL
  for (var i = 0; i < count; i++)
      bandwidths[i] = 0.0;
  
  // Use the device asked by the user
  for (var d = startDevice; d <= endDevice; d++)
  {
      // Allocate command queue for the device (dealloc first if already allocated)
      createQueue(d);
  
      //run each of the copies
      for(var i = 0; i < count; i++)
      {
          memSizes[i] = start + i * increment;
          switch(memCpyKind)
          {
          case DEVICE_TO_HOST:    bandwidths[i] += testDeviceToHostTransfer(memSizes[i], accMode, memMode);
              break;
          case HOST_TO_DEVICE:    bandwidths[i] += testHostToDeviceTransfer(memSizes[i], accMode, memMode);
              break;
          case DEVICE_TO_DEVICE:  bandwidths[i] += testDeviceToDeviceTransfer(memSizes[i]);
              break;
          }
      }
  } // Complete the bandwidth computation on all the devices
  
  printResults(memSizes, bandwidths, count, memCpyKind, accMode, memMode, (1 + endDevice - startDevice));

  //clean up
}

/////////////////////////////////////////////////////////
//print results in an easily read format
////////////////////////////////////////////////////////
function printResults(memSizes, bandwidths, count, kind, accMode, memMode, iNumDevs)
{
  // log config information 
  var str="";
  if (kind == DEVICE_TO_DEVICE) {
      str += "Device -> Device Bandwidth, "+iNumDevs+" Device(s)";
  }
  else {
    str += (kind == DEVICE_TO_HOST) ? "Device -> Host" : "Host -> Device";
    str += " Bandwidth, "+iNumDevs+" Device(s), ";

    str += (memMode == PAGEABLE) ? "Paged memory" : "Pinned memory";
    str += (accMode == DIRECT) ? ", direct access\n" : ", mapped access";
  }

  log(str);
  log("   Transfer Size (Bytes)\tBandwidth(MB/s)");
  for(var i = 0; i < count; i++)
      log("   "+memSizes[i]+"\t\t\t"+((memSizes[i] < 10000)? "\t" : "")+ bandwidths[i]);
  log();
}

///////////////////////////////////////////////////////////////////////////////
//test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
function testDeviceToHostTransfer(memSize, accMode, memMode)
{
  var elapsedTimeInSec = 0.0;
  var bandwidthInMBs = 0.0;
  var cmPinnedData = null;
  var cmDevData = null;

  //allocate and init host memory, pinned or conventional
  if(memMode == PINNED) {
      // Create a host buffer
      cmPinnedData = ctx.createBuffer(WebCL.MEM_READ_WRITE | WebCL.MEM_ALLOC_HOST_PTR, memSize);
  
      // Get a mapped pointer
      h_data = cqCommandQueue.enqueueMapBuffer(cmPinnedData, WebCL.TRUE, WebCL.MAP_WRITE, 0, memSize);
  
      //initialize 
      for(var i = 0; i < memSize; i++)
          h_data[i] = (i & 0xff);
  
      // unmap and make data in the host buffer valid
      cqCommandQueue.enqueueUnmapMemObject(cmPinnedData, h_data);
  }
  else 
  {
      // standard host alloc
      h_data = new Uint8Array(memSize);
  
      //initialize 
      for(var i = 0; i < memSize; i++)
        h_data[i] = (i & 0xff);
  }
  
  // allocate device memory 
  cmDevData = ctx.createBuffer(WebCL.MEM_READ_WRITE, memSize);
  
  // initialize device memory 
  if(memMode == PINNED) {
    // Get a mapped pointer
    h_data = cqCommandQueue.enqueueMapBuffer(cmPinnedData, WebCL.TRUE, WebCL.MAP_WRITE, 0, memSize);
  
    cqCommandQueue.enqueueWriteBuffer(cmDevData, WebCL.FALSE, 0, memSize, h_data);
  }
  else {
      ciErrNum = cqCommandQueue.enqueueWriteBuffer(cmDevData, WebCL.FALSE, 0, memSize, h_data);
  }
  
  // Sync queue to host, start timer 0, and copy data from GPU to Host
  cqCommandQueue.finish();
  var start=new Date();
  if(accMode == DIRECT) { 
      // DIRECT:  API access to device buffer 
      for(var i = 0; i < MEMCOPY_ITERATIONS; i++) {
          ciErrNum = cqCommandQueue.enqueueReadBuffer(cmDevData, WebCL.FALSE, 0, memSize, h_data);
      }
      cqCommandQueue.finish();
  } 
  else {
      // MAPPED: mapped pointers to device buffer for conventional pointer access
      var dm_idata = cqCommandQueue.enqueueMapBuffer(cmDevData, WebCL.TRUE, WebCL.MAP_WRITE, 0, memSize);

      for(var i = 0; i < MEMCOPY_ITERATIONS; i++) {
          for(var j=0;j<memSize;++j)
            h_data[j]=dm_idata[j];
      }
      cqCommandQueue.enqueueUnmapMemObject(cmDevData, dm_idata);
  }
  
  //get the the elapsed time in seconds
  var elapsedTimeInSec = new Date()-start;
  
  //calculate bandwidth in MB/s
  bandwidthInMBs = (memSize * MEMCOPY_ITERATIONS) / (elapsedTimeInSec * (1 << 20));
  
  //clean up memory
  //if(cmDevData) cmDevData.release();
  if(cmPinnedData) {
    cqCommandQueue.enqueueUnmapMemObject(cmPinnedData, h_data);  
    //cmPinnedData.release(); 
  }
  h_data = null;
  
  return bandwidthInMBs;
}

///////////////////////////////////////////////////////////////////////////////
//test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
function testHostToDeviceTransfer(memSize, accMode, memMode) {
  var elapsedTimeInSec = 0.0;
  var bandwidthInMBs = 0.0;
  var cmPinnedData = null;
  var cmDevData = null;
  
  // Allocate and init host memory, pinned or conventional
  if(memMode == PINNED)
  { 
    // Create a host buffer
    cmPinnedData = ctx.createBuffer(WebCL.MEM_READ_WRITE | WebCL.MEM_ALLOC_HOST_PTR, memSize);

    // Get a mapped pointer
    h_data = cqCommandQueue.enqueueMapBuffer(cmPinnedData, WebCL.TRUE, WebCL.MAP_WRITE, 0, memSize);

    // initialize
    for(var i = 0; i < memSize; i++) {
        h_data[i] = (i & 0xff);
    }

    // unmap and make data in the host buffer valid
    cqCommandQueue.enqueueUnmapMemObject(cmPinnedData, h_data);
    h_data = null;  // buffer is unmapped
  }
  else {
      // standard host alloc
      h_data = new Uint8Array(memSize);
  
      // initialize
      for(var i = 0; i < memSize; i++) {
        h_data[i] = (i & 0xff);
    }
  }
  
  // allocate device memory
  cmDevData = ctx.createBuffer(WebCL.MEM_READ_WRITE, memSize);
  
  // Sync queue to host, start timer 0, and copy data from Host to GPU
  cqCommandQueue.finish();
  var start=new Date();
  
  if(accMode == DIRECT) { 
    if(memMode == PINNED) {
      // Get a mapped pointer
      h_data = cqCommandQueue.enqueueMapBuffer(cmPinnedData, WebCL.TRUE, WebCL.MAP_READ, 0, memSize);
    }
  
    // DIRECT: API access to device buffer
    for(var i = 0; i < MEMCOPY_ITERATIONS; i++) {
      ciErrNum = cqCommandQueue.enqueueWriteBuffer(cmDevData, WebCL.FALSE, 0, memSize, h_data);
    }
    cqCommandQueue.finish();
  } 
  else {
    // MAPPED: mapped pointers to device buffer and conventional pointer access
    var dm_idata = cqCommandQueue.enqueueMapBuffer(cmDevData, WebCL.TRUE, WebCL.MAP_WRITE, 0, memSize);
    if(memMode == PINNED ) {
      h_data = cqCommandQueue.enqueueMapBuffer(cmPinnedData, WebCL.TRUE, WebCL.MAP_READ, 0, memSize); 
    }
    
    for(var i = 0; i < MEMCOPY_ITERATIONS; i++) {
        for(var j=0;j<memSize;j++)
          dm_idata[j]=h_data[j];
    }
    ciErrNum = cqCommandQueue.enqueueUnmapMemObject(cmDevData, dm_idata);
  }
  
  // get the the elapsed time in seconds
  var elapsedTimeInSec = new Date()-start;
  
  // calculate bandwidth in MB/s
  bandwidthInMBs = (memSize * MEMCOPY_ITERATIONS)/(elapsedTimeInSec * (1 << 20));
  
  // clean up memory
  //if(cmDevData) cmDevData.release();
  if(cmPinnedData) {
    cqCommandQueue.enqueueUnmapMemObject(cmPinnedData, h_data);
    //cmPinnedData.release();
  }
  h_data = null;
  
  return bandwidthInMBs;
}

///////////////////////////////////////////////////////////////////////////////
//test the bandwidth of a device to host memcopy of a specific size
///////////////////////////////////////////////////////////////////////////////
function testDeviceToDeviceTransfer(memSize)
{
  var elapsedTimeInSec = 0.0;
  var bandwidthInMBs = 0.0;

  //allocate host memory
  var h_idata = new Uint8Array(memSize);
    
  //initialize the memory
  for(var i = 0; i < memSize; i++)
      h_idata[i] = (i & 0xff);

  // allocate device input and output memory and initialize the device input memory
  var d_idata = ctx.createBuffer(WebCL.MEM_READ_ONLY, memSize);
  var d_odata = ctx.createBuffer(WebCL.MEM_WRITE_ONLY, memSize);         
 
  cqCommandQueue.enqueueWriteBuffer(d_idata, WebCL.TRUE, 0, memSize, h_idata);

  // Sync queue to host, start timer 0, and copy data from one GPU buffer to another GPU bufffer
  cqCommandQueue.finish();
  var start=new Date();
  
  for(var i = 0; i < MEMCOPY_ITERATIONS; i++)
  {
    cqCommandQueue.enqueueCopyBuffer(d_idata, d_odata, 0, 0, memSize);
  }    

  // Sync with GPU
  cqCommandQueue.finish();

  //get the the elapsed time in seconds
  elapsedTimeInSec = new Date()-start;

  // Calculate bandwidth in MB/s 
  //      This is for kernels that read and write GMEM simultaneously 
  //      Obtained Throughput for unidirectional block copies will be 1/2 of this #
  bandwidthInMBs = 2.0 * (memSize * MEMCOPY_ITERATIONS)/(elapsedTimeInSec * (1 << 20));

  //clean up memory on host and device
  //free(h_idata);
  //d_idata.release();
  //d_odata.release();
  
  return bandwidthInMBs;
}
