/*
 Adapted from OpenCL Programming Guide, Aaftab Munshi, Benedict Gaster, Timothy Mattson, James Fung, Dan Ginsburg, Addison-Wesley Professional, http://www.openclprogrammingguide.com
 Chapter 8 - ImageFilter2D.cpp
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
else
  WebCL = window.webcl;

//First check if the webcl extension is installed at all 
if (WebCL == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  process.exit(-1);
}

var file = __dirname+'/mike_scooter.jpg';

log('Loading image '+file);
var img=Image.load(file);
var image=img.convertTo32Bits();
log('Image '+file+': \n'+util.inspect(image));

image.size = image.height*image.pitch;

var outBuf=ImageFilter(image);

// PNG uses 32-bit images, JPG can only work on 24-bit images
if(!Image.save('out.png',outBuf, image.width, image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
  log("Error saving image");
image.unload();

function ImageFilter(image) {

  // create GPU context for this platform
  var context=WebCL.createContext(WebCL.DEVICE_TYPE_GPU);

  // find the device for this context
  var devices = context.getInfo(WebCL.CONTEXT_DEVICES);
  device=devices[0];

  // Report the device vendor and device name
  // 
  var vendor_name = device.getInfo(WebCL.DEVICE_VENDOR);
  var device_name = device.getInfo(WebCL.DEVICE_NAME);
  var has_image_support = device.getInfo(WebCL.DEVICE_IMAGE_SUPPORT)
  log("Connecting to: "+vendor_name+" "+device_name+", has image support = "+has_image_support);

  kernelSourceCode = fs.readFileSync(__dirname+'/gaussian_filter.cl','ascii');
  
  //Create and program from source
  program=context.createProgram(kernelSourceCode);
  
  // create device buffers
  var /* WebCLImage */    clImage, clOutImage;
  var /* WebCLSampler */  sampler;

  var image_desc={
    channelOrder : WebCL.RGBA,
    channelType : WebCL.UNORM_INT8,
    width : image.width,
    height : image.height,
    rowPitch : image.pitch
  };
  try {
    var image_desc={
      channelOrder : WebCL.RGBA,
      channelType : WebCL.UNORM_INT8,
      width : image.width,
      height : image.height,
      rowPitch : image.pitch
    };
    clImage = context.createImage(WebCL.MEM_READ_ONLY | WebCL.MEM_COPY_HOST_PTR, image_desc, image.buffer);
  }
  catch(err) {
    console.log('error creating input CL image. '+err);
    process.exit(-1);
  }
  try {
    var image_desc={
        channelOrder : WebCL.RGBA,
        channelType : WebCL.UNORM_INT8,
        width : image.width,
        height : image.height,
        rowPitch : image.pitch
      };
    clOutImage = context.createImage(WebCL.MEM_WRITE_ONLY, image_desc, null);
  }
  catch(err) {
    console.log('error creating output CL image. '+err);
    process.exit(-1);
  }

  // create sampler
  try {
    sampler = context.createSampler(false, WebCL.ADDRESS_CLAMP_TO_EDGE,WebCL.FILTER_NEAREST);
  }
  catch(ex) {
    log("Cant' create sampler. "+ex);
    process.exit(-1);
  }

  //Build program
  try {
    program.build(device);
  } catch (err) {
    log('Error building program: ' + err);
    log(program.getBuildInfo(device, WebCL.PROGRAM_BUILD_LOG));
    process.exit(-1);
  }

  //Create kernel object
  try {
    kernel= program.createKernel("gaussian_filter");
  }
  catch(err) {
    console.log(program.getBuildInfo(device,WebCL.PROGRAM_BUILD_LOG));
  }

  // Set the arguments to our compute kernel
  var aints=new Int32Array(3);
  aints.set([image.width, image.height, 0]);
  try {
    kernel.setArg(0, clImage);
    kernel.setArg(1, clOutImage);
    kernel.setArg(2, sampler);
    kernel.setArg(3, new Uint8Array([image.width]));
    kernel.setArg(4, new Uint8Array([image.height]));
  }
  catch(ex) {
    log("Can't set kernel arguments. "+ex);
    process.exit(-1);
  }

  //Create command queue
  queue=context.createCommandQueue(device);

  // Init ND-range
  // Get the maximum work group size for executing the kernel on the device
  var localWS =[ 16, 16 ];
  var globalWS = [ localWS[0] * clu.DivUp(image.width, localWS[0]), 
                   localWS[1] * clu.DivUp(image.height, localWS[1]) ];

  log("Global work item size: " + globalWS);
  log("Local work item size: " + localWS);

  // Execute (enqueue) kernel
  queue.enqueueNDRangeKernel(kernel, 2,
      null,
      globalWS,
      localWS);

  var outBuf = new Uint32Array(image.size);
  queue.enqueueReadImage(clOutImage, true, 
    [0,0],[image.width,image.height],0,
    outBuf);

  queue.finish(); //Finish all the operations

  return outBuf;
}


