/*
 ** This file contains proprietary software owned by Motorola Mobility, Inc. **
 ** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
 ** 
 ** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
 */

var cl = require('../webcl'), 
    clu = require('../lib/clUtils'), 
    util = require('util'), 
    fs = require('fs'), 
    WebGL = require('node_webgl'), 
    document = WebGL.document(), 
    Image = WebGL.Image, 
    log = console.log, 
    requestAnimationFrame = document.requestAnimationFrame,
    alert=console.log;

//First check if the webcl extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. "
      + "Make sure that you have the WebCL extension installed.");
  return;
}

// Defines and globals for box filter processing demo
// *****************************************************************************
var uiNumOutputPix = 64; // Default output pix per workgroup... may be modified depending HW/OpenCl caps
var iRadius = 10; // initial radius of 2D box filter mask
var fScale = 1 / (2 * iRadius + 1); // precalculated GV rescaling value
var iRadiusAligned;

// Image data vars
var image = new Image();
image.filename = "lenaRGB.jpg";

// OpenGL, Display Window and GUI Globals
var iGraphicsWinWidth = 800; // GL Window width
var iGraphicsWinHeight = 800; // GL Windows height

// OpenCL vars
var clSourcefile = "BoxFilter.cl"; // OpenCL kernel source file
var cSourceCL; // Buffer to hold source for compilation
var cpPlatform; // OpenCL platform
var cxGPUContext; // OpenCL context
var cqCommandQueue; // OpenCL command que
var cdDevices; // device list
var uiNumDevsUsed = 1; // Number of devices used in this sample
var cpProgram; // OpenCL program
var ckBoxRowsLmem; // OpenCL Kernel for row sum (using lmem)
var ckBoxRowsTex; // OpenCL Kernel for row sum (using 2d Image/texture)
var ckBoxColumns; // OpenCL for column sum and normalize
var cmDevBufIn; // OpenCL device memory object (buffer or 2d Image) for input
// data
var cmDevBufTemp; // OpenCL device memory temp buffer object
var cmDevBufOut; // OpenCL device memory output buffer object
var cmCL_PBO = 0; // OpenCL representation of GL pixel buffer
var InputFormat; // OpenCL format descriptor for 2D image useage
var RowSampler; // Image Sampler for box filter processing with texture (2d
// Image)
var szGlobalWorkSize = [ 0, 0 ]; // global # of work items
var szLocalWorkSize = [ 0, 0 ]; // work group # of work items
var szMaxWorkgroupSize = 512; // initial max # of work items
var szParmDataBytes; // Byte size of context information
var szKernelLength; // Byte size of kernel code
var ciErrNum; // Error code var

// OpenGL interop vars
var gl;
var tex_screen; // (offscreen) render target
var vbo = {}, pbo; // pixel buffer for interop
var prog; // shader program

document.setTitle("BoxFilterGL");
document.on('keydown', processKey);
document.on('quit', function() {
  log('exiting app');
  
});
main();

function main() {
  // load image
  image.onload=function() { 
    console.log("Loaded image: " + image.src);
    log("Image Width = " + image.width + ", Height = " + image.height
        + ", bpp = 32, Mask Radius = " + iRadius);
    
    // adjust window to pixel ratio
    iGraphicsWinHeight *= image.height/image.width;
    document.createWindow(iGraphicsWinWidth, iGraphicsWinHeight);
  };
  image.src = __dirname + '/' + image.filename;

  // Allocate intermediate and output host image buffers
  image.szBuffBytes = image.width * image.height * 4;

  var uiTemp = new Uint8Array(image.szBuffBytes); // Host buffer to hold intermediate image data

  // Initialize GL Context
  initGL(document.createElement("mycanvas",iGraphicsWinWidth, iGraphicsWinHeight));

  // Pick platform
  var platformList = cl.getPlatforms();
  cpPlatform = platformList[0];

  // Query the set of GPU devices on this platform
  cdDevices = cpPlatform.getDevices(cl.DEVICE_TYPE_GPU);
  log("  # of Devices Available = " + cdDevices.length);
  var device = cdDevices[0];
  log("  Using Device 0: " + device.getDeviceInfo(cl.DEVICE_NAME));

  // get CL-GL extension
  var extensions = device.getDeviceInfo(cl.DEVICE_EXTENSIONS);
  var hasGLSupport = extensions.search(/gl.sharing/i) >= 0;
  log(hasGLSupport ? "GL-CL extension available ;-)" : "No GL support");
  if (!hasGLSupport)
    process.exit(-1);

  // create the OpenCL context
  cxGPUContext =
      cl.createContext(device, [ cl.GL_CONTEXT_KHR, gl, cl.CONTEXT_PLATFORM,
          cpPlatform ]);

  // create a command-queue
  cqCommandQueue = cxGPUContext.createCommandQueue(device, 0);

  // Allocate OpenCL object for the source data
  // 2D Image (Texture) on device
  InputFormat = {
    order : cl.RGBA,
    data_type : cl.UNSIGNED_INT8
  };

  cmDevBufIn =
      cxGPUContext.createImage2D(cl.MEM_READ_ONLY | cl.MEM_USE_HOST_PTR,
          InputFormat, image.width, image.height, image.pitch, image);

  RowSampler =
      cxGPUContext.createSampler(false, cl.ADDRESS_CLAMP, cl.FILTER_NEAREST);

  // Allocate the OpenCL intermediate and result buffer memory objects on the
  // device GMEM
  cmDevBufTemp = cxGPUContext.createBuffer(cl.MEM_READ_WRITE, image.szBuffBytes);
  cmDevBufOut = cxGPUContext.createBuffer(cl.MEM_WRITE_ONLY, image.szBuffBytes);

  // Create OpenCL representation of OpenGL PBO
  cmCL_PBO = cxGPUContext.createFromGLBuffer(cl.MEM_WRITE_ONLY, pbo);

  // create the program
  var sourceCL = fs.readFileSync(__dirname + '/' + clSourcefile, 'ascii');
  cpProgram = cxGPUContext.createProgram(sourceCL);

  // build the program
  try {
    cpProgram.build(device, "-cl-fast-relaxed-math");
  } catch (err) {
    log('Error building program: ' + err);
    return;
  }
  log("Build Status: "
      + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_STATUS));
  log("Build Options: "
      + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_OPTIONS));
  log("Build Log: " + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_LOG));

  // Create kernels
  ckBoxRowsTex = cpProgram.createKernel("BoxRowsTex");
  ckBoxColumns = cpProgram.createKernel("BoxColumns");

  // set the kernel args
  ResetKernelArgs(image.width, image.height, iRadius, fScale);

  // start main rendering loop
  DisplayGL();
}

// Function to set kernel args that only change outside of GLUT loop
// *****************************************************************************
function ResetKernelArgs(uiWidth, uiHeight, r, fScale) {
  log("Radius: "+r);
  
  // Set the Argument values for the row kernel
  // (Image/texture version)
  ckBoxRowsTex.setArg(0, cmDevBufIn, cl.type.MEM);
  ckBoxRowsTex.setArg(1, cmDevBufTemp, cl.type.MEM);
  ckBoxRowsTex.setArg(2, RowSampler, cl.type.SAMPLER);
  ckBoxRowsTex.setArg(3, uiWidth, cl.type.INT | cl.type.UNSIGNED);
  ckBoxRowsTex.setArg(4, uiHeight, cl.type.INT | cl.type.UNSIGNED);
  ckBoxRowsTex.setArg(5, r, cl.type.INT);
  ckBoxRowsTex.setArg(6, fScale, cl.type.FLOAT);

  // Set the Argument values for the column kernel
  ckBoxColumns.setArg(0, cmDevBufTemp, cl.type.MEM);
  ckBoxColumns.setArg(1, cmDevBufOut, cl.type.MEM);
  ckBoxColumns.setArg(2, uiWidth, cl.type.INT | cl.type.UNSIGNED);
  ckBoxColumns.setArg(3, uiHeight, cl.type.INT | cl.type.UNSIGNED);
  ckBoxColumns.setArg(4, r, cl.type.INT);
  ckBoxColumns.setArg(5, fScale, cl.type.FLOAT);
}

// OpenCL computation function for GPU:
// Copies input data to the device, runs kernel, copies output data back to host
// *****************************************************************************
function BoxFilterGPU(image, cmOutputBuffer, r, fScale) {
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer, cl.type.MEM);

  // Copy input data from host to device
  // 2D Image (Texture)
  var szTexOrigin = [ 0, 0, 0 ]; // Offset of input texture origin relative to host image
  var szTexRegion = [ image.width, image.height, 1 ]; // Size of texture region to operate on
  cqCommandQueue.enqueueWriteImage(cmDevBufIn, cl.TRUE, szTexOrigin,
      szTexRegion, 0, 0, image);

  // Set global and local work sizes for row kernel
  szLocalWorkSize[0] = uiNumOutputPix;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0] = szLocalWorkSize[0] * clu.DivUp(image.height, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  //log("row kernel work sizes: global= " + szGlobalWorkSize[0] + " local= " + szLocalWorkSize[0]);

  // Sync host and start computation timer
  cqCommandQueue.finish();

  // Launch row kernel
  // 2D Image (Texture)
  cqCommandQueue.enqueueNDRangeKernel(ckBoxRowsTex, null, szGlobalWorkSize,
      szLocalWorkSize);

  // Set global and local work sizes for column kernel
  szLocalWorkSize[0] = 64;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0] = szLocalWorkSize[0] * clu.DivUp(image.width, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  //log("column kernel work sizes: global= " + szGlobalWorkSize[0] + " local= " + szLocalWorkSize[0]);

  // Launch column kernel
  cqCommandQueue.enqueueNDRangeKernel(ckBoxColumns, null, szGlobalWorkSize, szLocalWorkSize);

  // sync host
  cqCommandQueue.finish();
}

//Create PBO
//*****************************************************************************
function createPBO(image_width, image_height) {
  // set up data parameter
  var num_texels = image_width * image_height;
  var num_values = num_texels * 4;
  var size_tex_data = 1 * num_values;

  // create buffer object
  var pbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, pbo);

  // buffer data
  gl.bufferData(gl.ARRAY_BUFFER, size_tex_data, gl.DYNAMIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);

  return pbo;
}

//Init WebGL
//*****************************************************************************
function initGL(canvas) {
  try {
    gl = canvas.getContext("experimental-webgl");
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;
  } catch (e) {
  }
  if (!gl) {
    alert("Could not initialise WebGL, sorry :-(");
  }

  initBuffers();
  initShaders();

  // pixel buffer and texture for WebCL
  pbo = createPBO(image.width, image.height);
  tex_screen = createTexture(image.width, image.height);
}

function initBuffers() {
  var vertex_coords = [ -1, -1, 0, 
                        1, -1, 0, 
                        1, 1, 0, 
                        -1, 1, 0 ];

  var tex_coords = [ 0, 0, 
                     1, 0, 
                     1, 1, 
                     0, 1 ];

  /* Create VBOs */
  vbo.coord = gl.createBuffer();
  vbo.tc = gl.createBuffer();

  /* VBO to hold vertex coordinates */
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo.coord);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertex_coords), gl.STATIC_DRAW);
  gl.vertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 0, 0);
  gl.enableVertexAttribArray(0);

  /* VBO to hold texture coordinates */
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo.tc);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(tex_coords), gl.STATIC_DRAW);
  gl.vertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, 0, 0);
  gl.enableVertexAttribArray(1);
}

/* Create and compile shaders */
function initShaders() {
  var vs = gl.createShader(gl.VERTEX_SHADER);
  var fs = gl.createShader(gl.FRAGMENT_SHADER);

  var fs_source =
      [ "uniform sampler2D tex;", 
        "out varying vec4 new_color;", 
        "void main() {",
        "vec3 color = vec3(texture(tex, gl_TexCoord[0].st));",
        "new_color = vec4(color, 1.0);", 
        "}" ].join('\n');
  var vs_source =
      [ "in  vec3 in_coords;", 
        "in  vec2 in_texcoords;", 
        "void main(void) {",
        "gl_TexCoord[0].st = in_texcoords;",
        "gl_Position = vec4(in_coords, 1.0);", 
        "}" ].join('\n');
  
  gl.shaderSource(vs, vs_source);
  gl.shaderSource(fs, fs_source);

  gl.compileShader(vs);
  gl.compileShader(fs);

  var prog = gl.createProgram();

  gl.bindAttribLocation(prog, 0, "in_coords");
  gl.bindAttribLocation(prog, 1, "in_color");

  gl.attachShader(prog, vs);
  gl.attachShader(prog, fs);

  gl.linkProgram(prog);
  gl.useProgram(prog);
}

//Create texture for GL-GL Interop
//*****************************************************************************
function createTexture(size_x, size_y) {
  // create a texture
  tex_name = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, tex_name);

  // set basic parameters
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);

  // buffer data
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, size_x, size_y, 0, gl.RGBA,
      gl.UNSIGNED_BYTE, null);

  return tex_name;
}

// rendering loop
// *****************************************************************************
function DisplayGL() {
  // Run filter processing (if toggled on), then render
  // Sync GL and acquire buffer from GL
  gl.finish();
  cqCommandQueue.enqueueAcquireGLObjects(cmCL_PBO);

  // Compute on GPU and get kernel processing time
  BoxFilterGPU(image, cmCL_PBO, iRadius, fScale);

  // Release GL output or explicit output copy
  // Release buffer
  cqCommandQueue.enqueueReleaseGLObjects(cmCL_PBO);
  cqCommandQueue.finish();

  // Copy results back to host memory, block until complete
  /*var uiOutput=new Uint8Array(szBuffBytes);
  cqCommandQueue.enqueueReadBuffer(cmDevBufOut, cl.TRUE, {
    offset: 0, 
    size: szBuffBytes, 
    buffer: uiOutput
  });
  
  // PNG uses 32-bit images, JPG can only work on 24-bit images
  if(!image.save('out_'+iRadius+'.png',uiOutput, 
      image.width,image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
    log("Error saving image");

  return;*/
  
  // Update the texture from the pbo
  gl.bindTexture(gl.TEXTURE_2D, tex_screen);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, image.width, image.height,
      gl.RGBA, gl.UNSIGNED_BYTE, null);
  gl.activeTexture(gl.TEXTURE0);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);

  // Draw processed image
  displayTexture(tex_screen);

  // Flip backbuffer to screen
  requestAnimationFrame(DisplayGL);
}

function displayTexture(texture) {

  // render a screen sized quad
  gl.enable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, texture);

  gl.viewport(0, 0, iGraphicsWinWidth, iGraphicsWinHeight);

  gl.clear(gl.COLOR_BUFFER_BIT);

  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);

  gl.disable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, null);
}

function processKey(evt) {
  //log('process key: '+evt.sym);
  var oldr=iRadius;
  
  switch(evt.sym) {
  case 61:   // + or = increases filter radius
    if ((szMaxWorkgroupSize - (((iRadius + 1 + 15) / 16) * 16) - iRadius - 1) > 0)
      iRadius++;
    break;
  case 45: // - or _ decreases filter radius
    if (iRadius > 1)
      iRadius--;
    break;
  }

  // Update filter parameters
  if(oldr!=iRadius) {
    fScale = 1 / (2 * iRadius + 1);
    ResetKernelArgs(image.width, image.height, iRadius, fScale);
  }
}