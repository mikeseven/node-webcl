// Adapted from: OpenCL Programming by Example, Ravishekhar Banger, Packt Publishing 2013
/*
 * This test will work with power-of-two images.
 */

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL    = require('../webcl');
  clu   = require('../lib/clUtils');
  util  = require("util"),
  fs    = require("fs");
  Image = require("node-image").Image;
  log   = console.log;
}

//First check if the webcl extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  process.exit(-1);
}

var histogram_kernel = [
"#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable                       ",
"__kernel                                                                              ",
"void histogram_kernel(__global const uint* data,                                      ",
"                  __local uchar* sharedArray,                                         ",
"                  __global uint* binResultR,                                          ",
"                  __global uint* binResultG,                                          ",
"                  __global uint* binResultB)                                          ",
"{                                                                                     ",
"    size_t localId = get_local_id(0);                                                 ",
"    size_t globalId = get_global_id(0);                                               ",
"    size_t groupId = get_group_id(0);                                                 ",
"    size_t groupSize = get_local_size(0);                                             ",
"     __local uchar* sharedArrayR = sharedArray;                                       ",
"     __local uchar* sharedArrayG = sharedArray + groupSize * BIN_SIZE;                ",
"     __local uchar* sharedArrayB = sharedArray + 2 * groupSize * BIN_SIZE;            ",
"                                                                                      ",
"    /* initialize shared array to zero */                                             ",
"    for(int i = 0; i < BIN_SIZE; ++i)                                                 ",
"    {                                                                                 ",
"        sharedArrayR[localId * BIN_SIZE + i] = 0;                                     ",
"        sharedArrayG[localId * BIN_SIZE + i] = 0;                                     ",
"        sharedArrayB[localId * BIN_SIZE + i] = 0;                                     ",
"    }                                                                                 ",
"                                                                                      ",
"    barrier(CLK_LOCAL_MEM_FENCE);                                                     ",
"                                                                                      ",
"    /* calculate thread-histograms */                                                 ",
"    for(int i = 0; i < BIN_SIZE; ++i)                                                 ",
"    {                                                                                 ",
"        uint value = data[globalId * BIN_SIZE + i];                                   ",
"        uint valueR = value & 0xFF;                                                   ",
"        uint valueG = (value & 0xFF00) >> 8;                                          ",
"        uint valueB = (value & 0xFF0000) >> 16;                                       ",
"        sharedArrayR[localId * BIN_SIZE + valueR]++;                                  ",
"        sharedArrayG[localId * BIN_SIZE + valueG]++;                                  ",
"        sharedArrayB[localId * BIN_SIZE + valueB]++;                                  ",
"    }                                                                                 ",
"                                                                                      ",
"    barrier(CLK_LOCAL_MEM_FENCE);                                                     ",
"                                                                                      ",
"    /* merge all thread-histograms into block-histogram */                            ",
"    for(int i = 0; i < BIN_SIZE / groupSize; ++i)                                     ",
"    {                                                                                 ",
"        uint binCountR = 0;                                                           ",
"        uint binCountG = 0;                                                           ",
"        uint binCountB = 0;                                                           ",
"        for(int j = 0; j < groupSize; ++j)                                            ",
"        {                                                                             ",
"            binCountR += sharedArrayR[j * BIN_SIZE + i * groupSize + localId];        ",
"            binCountG += sharedArrayG[j * BIN_SIZE + i * groupSize + localId];        ",
"            binCountB += sharedArrayB[j * BIN_SIZE + i * groupSize + localId];        ",
"        }                                                                             ",
"                                                                                      ",
"        binResultR[groupId * BIN_SIZE + i * groupSize + localId] = binCountR;         ",
"        binResultG[groupId * BIN_SIZE + i * groupSize + localId] = binCountG;         ",
"        binResultB[groupId * BIN_SIZE + i * groupSize + localId] = binCountB;         ",
"    }                                                                                 ",
"}"].join("\n");

var binSize = 256;  // number of histogram bins
var groupSize = 16; // local workgroup size

var file = __dirname+'/lenaRGB.jpg';
log('Loading image '+file);
var img=Image.load(file);
var image=img.convertTo32Bits();
log('Image '+file+': \n'+util.inspect(image));

image.size = image.height*image.pitch;
log('Total number of pixels: '+ (image.width*image.height));

var binsRGB=histogram(image);

// Validate the histogram operation. 
// The idea behind this is that once a histogram is computed the sum of all the bins should be equal 
// to the number of pixels.
log('Validation: total number of pixels should be: '+ (image.width*image.height));
var totalPixelsR = 0, totalPixelsG = 0, totalPixelsB = 0;
for(var j = 0; j < binSize; ++j)
{
    totalPixelsR += binsRGB[0][j];
    totalPixelsG += binsRGB[1][j];
    totalPixelsB += binsRGB[2][j];
}
log ("Total Number of Red Pixels = ",totalPixelsR);
log ("Total Number of Green Pixels = ",totalPixelsG);
log ("Total Number of Blue Pixels = ",totalPixelsB);

// cleanup
image.unload();

function histogram(image) {
  var status = 0;
  var /*cl_mem*/  imageBuffer;
  var /*cl_mem*/  intermediateHistR, intermediateHistG, intermediateHistB; // Intermediate Image Histogram buffers

  var subHistgCnt  = clu.DivUp(image.width * image.height,binSize*groupSize);
  var szIntermediateHist = binSize * subHistgCnt;
  var midDeviceBinR = new Uint32Array(szIntermediateHist);
  var midDeviceBinG = new Uint32Array(szIntermediateHist);
  var midDeviceBinB = new Uint32Array(szIntermediateHist);
  var deviceBinR    = new Uint32Array(binSize);
  var deviceBinG    = new Uint32Array(binSize);
  var deviceBinB    = new Uint32Array(binSize);

  // create GPU context for this platform
  var context=WebCL.createContext(WebCL.DEVICE_TYPE_GPU);

  // find the device for this context
  var devices = context.getInfo(WebCL.CONTEXT_DEVICES);
  var device=devices[0];

  // Report the device vendor and device name
  var vendor_name = device.getInfo(WebCL.DEVICE_VENDOR);
  var device_name = device.getInfo(WebCL.DEVICE_NAME);
  log("Connecting to: "+vendor_name+" "+device_name);

  // create device buffers
  try {
    imageBuffer = context.createBuffer(WebCL.MEM_READ_ONLY | WebCL.MEM_ALLOC_HOST_PTR, image.size);
  }
  catch(err) {
    log('error creating input image buffer. '+err);
    process.exit(-1);
  }

  //Create command queue
  queue=context.createCommandQueue(device, 0);

  // Write our data set into the input array in device memory asynchronously
  queue.enqueueWriteBuffer(imageBuffer, false, 0, image.size, image.buffer);

  // wait for image data transfered
  queue.finish();

  var szBytesIntermediateHist = Int32Array.BYTES_PER_ELEMENT * szIntermediateHist;
  try {
    intermediateHistR = context.createBuffer(WebCL.MEM_WRITE_ONLY, szBytesIntermediateHist);
    intermediateHistG = context.createBuffer(WebCL.MEM_WRITE_ONLY, szBytesIntermediateHist);
    intermediateHistB = context.createBuffer(WebCL.MEM_WRITE_ONLY, szBytesIntermediateHist);
  }
  catch(err) {
    log('error creating output buffers of size '+szBytesIntermediateHist+' bytes. '+err);
    process.exit(-1);
  }

  //Create and program from source
  var program=context.createProgram(histogram_kernel);
  
  //Build program
  program.build(device,"-cl-kernel-arg-info -DBIN_SIZE="+binSize);

  //Create kernel object
  var kernel;
  try {
    kernel= program.createKernel("histogram_kernel");
  }
  catch(err) {
    log(program.getBuildInfo(device,WebCL.PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  kernel.setArg(0, imageBuffer);
  kernel.setArg(1, new Uint32Array([3 * groupSize * binSize * 1])); // for __local array
  kernel.setArg(2, intermediateHistR);
  kernel.setArg(3, intermediateHistG);
  kernel.setArg(4, intermediateHistB);

  //Create command queue
  queue=context.createCommandQueue(device, 0);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var globalWS = [ clu.DivUp(image.width * image.height, binSize*groupSize) * groupSize ];
  var localWS=[ groupSize ];
 
  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel, 1, 
      null,
      globalWS,
      localWS);

  queue.finish(); //Finish all the operations

  // read histograms
  var readEvt=[new WebCL.WebCLEvent(), new WebCL.WebCLEvent(), new WebCL.WebCLEvent()];
  status = queue.enqueueReadBuffer(intermediateHistR, false, 0, szBytesIntermediateHist, midDeviceBinR, null, readEvt[0]);
  status |= queue.enqueueReadBuffer(intermediateHistG, false, 0, szBytesIntermediateHist, midDeviceBinG, null, readEvt[1]);
  status |= queue.enqueueReadBuffer(intermediateHistB, false, 0, szBytesIntermediateHist, midDeviceBinB, null, readEvt[2]);

  status = WebCL.waitForEvents(readEvt);

  // Calculate final histogram bin 
  for(var i = 0; i < subHistgCnt; ++i)
  {
      for(var j = 0; j < binSize; ++j)
      {
          deviceBinR[j] += midDeviceBinR[i * binSize + j];
          deviceBinG[j] += midDeviceBinG[i * binSize + j];
          deviceBinB[j] += midDeviceBinB[i * binSize + j];
      }
  }

  return [deviceBinR, deviceBinG, deviceBinB];
}


