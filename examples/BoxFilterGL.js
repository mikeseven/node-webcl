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
    WebGL = require('webgl'), 
    document = WebGL.document(), 
    Image = WebGL.Image, 
    log = console.log, 
    requestAnimationFrame = document.requestAnimationFrame;

//Read and eval library for mat/vec operations
eval(fs.readFileSync(__dirname + '/glMatrix-0.9.5.min.js', 'utf8'));

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
var cImageFile = "mike_scooter.jpg";
var uiImageWidth = 0; // Image width
var uiImageHeight = 0; // Image height
var uiInput; // Host buffer to hold input image data
var uiTemp; // Host buffer to hold intermediate image data

// OpenGL, Display Window and GUI Globals
var iGraphicsWinWidth = 800; // GL Window width
var iGraphicsWinHeight = 800; // GL Windows height
var iFrameCount = 0; // FPS count for averaging
var iFrameTrigger = 90; // FPS trigger for sampling
var iFramesPerSec = 60; // frames per second

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
var szBuffBytes; // Size of main image buffers
var szGlobalWorkSize = [ 0, 0 ]; // global # of work items
var szLocalWorkSize = [ 0, 0 ]; // work group # of work items
var szMaxWorkgroupSize = 512; // initial max # of work items
var szParmDataBytes; // Byte size of context information
var szKernelLength; // Byte size of kernel code
var ciErrNum; // Error code var

// OpenGL interop vars
var gl;
var tex_screen; // (offscreen) render target
var pbo; // pixel buffer for interop
var mvMatrix = mat4.create(); // model-view matrix
var pMatrix = mat4.create(); // projection matrix
var cubeVertexPositionBuffer;
var cubeVertexTextureCoordBuffer;
var cubeVertexIndexBuffer;
var shaderProgram;

document.createWindow(iGraphicsWinWidth, iGraphicsWinHeight);
document.setTitle("BoxFilterGL");
document.on('KEYDOWN', processKey);
main();

function main() {
  // load image
  uiInput = new Image();
  uiInput.onload(function(s) { // [MBS] was onload() event
    console.log("Loaded image: " + s);
    uiImageWidth = uiInput.width;
    uiImageHeight = uiInput.height;
    log("Image Width = " + uiImageWidth + ", Height = " + uiImageHeight
        + ", bpp = 32, Mask Radius = " + iRadius);
    // adjust window to pixel ratio
    iGraphicsWinHeight *= uiInput.height/uiInput.width;
    document.createWindow(iGraphicsWinWidth, iGraphicsWinHeight);
  });
  uiInput.src = __dirname + '/' + cImageFile;

  // Allocate intermediate and output host image buffers
  szBuffBytes = uiImageWidth * uiImageHeight * 4;
  uiTemp = new Uint8Array(szBuffBytes);

  // Initialize GL Context
  initGL(document.getElementById("mycanvas"));

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
    return;
  clgl = device.getExtension(cl.GL_CONTEXT_KHR);
  if (clgl == undefined)
    return;

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
          InputFormat, uiImageWidth, uiImageHeight, uiImageWidth * 4, uiInput);

  RowSampler =
      cxGPUContext.createSampler(false, cl.ADDRESS_CLAMP, cl.FILTER_NEAREST);

  // Allocate the OpenCL intermediate and result buffer memory objects on the
  // device GMEM
  cmDevBufTemp = cxGPUContext.createBuffer(cl.MEM_READ_WRITE, szBuffBytes);
  cmDevBufOut = cxGPUContext.createBuffer(cl.MEM_WRITE_ONLY, szBuffBytes);

  // Create OpenCL representation of OpenGL PBO
  cmCL_PBO = clgl.createFromGLBuffer(cxGPUContext, cl.MEM_WRITE_ONLY, pbo);

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
  ResetKernelArgs(uiImageWidth, uiImageHeight, iRadius, fScale);

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

//Helper to get next up value for integer division
//*****************************************************************************
function DivUp(dividend, divisor) {
  return Math.round(dividend / divisor);
}

// OpenCL computation function for GPU:
// Copies input data to the device, runs kernel, copies output data back to host
// *****************************************************************************
function BoxFilterGPU(uiInputImage, cmOutputBuffer, uiWidth, uiHeight, r,
    fScale) {
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer, cl.type.MEM);

  // Copy input data from host to device
  // 2D Image (Texture)
  var szTexOrigin = [ 0, 0, 0 ]; // Offset of input texture origin relative to host image
  var szTexRegion = [ uiWidth, uiHeight, 1 ]; // Size of texture region to operate on
  cqCommandQueue.enqueueWriteImage(cmDevBufIn, cl.TRUE, szTexOrigin,
      szTexRegion, 0, 0, uiInputImage);

  // Set global and local work sizes for row kernel
  szLocalWorkSize[0] = uiNumOutputPix;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0] =
      szLocalWorkSize[0] * DivUp(uiHeight, szLocalWorkSize[0]);
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
  szGlobalWorkSize[0] = szLocalWorkSize[0] * DivUp(uiWidth, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  //log("column kernel work sizes: global= " + szGlobalWorkSize[0] + " local= " + szLocalWorkSize[0]);

  // Launch column kernel
  cqCommandQueue.enqueueNDRangeKernel(ckBoxColumns, null, szGlobalWorkSize,
      szLocalWorkSize);

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
  gl.bufferData(gl.ARRAY_BUFFER, size_tex_data, gl.STATIC_DRAW);
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
  pbo = createPBO(uiImageWidth, uiImageHeight);
  tex_screen = createTexture(uiImageWidth, uiImageHeight);
}

function initBuffers() {
  cubeVertexPositionBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
  vertices =
      [ -1.0, -1.0, 0.5, 
        1.0, -1.0, 0.5, 
        1.0, 1.0, 0.5, 
        -1.0, 1.0, 0.5, ];
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);
  cubeVertexPositionBuffer.itemSize = 3;
  cubeVertexPositionBuffer.numItems = 4;

  cubeVertexTextureCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexTextureCoordBuffer);
  var textureCoords = [ 0.0, 0.0, 
                        1.0, 0.0, 
                        1.0, 1.0, 
                        0.0, 1.0, ];
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoords),
      gl.STATIC_DRAW);
  cubeVertexTextureCoordBuffer.itemSize = 2;
  cubeVertexTextureCoordBuffer.numItems = 4;

  cubeVertexIndexBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
  var cubeVertexIndices = [ 0, 1, 2, 0, 2, 3, ];
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(cubeVertexIndices),
      gl.STATIC_DRAW);
  cubeVertexIndexBuffer.itemSize = 1;
  cubeVertexIndexBuffer.numItems = 6;
}

function getShader(gl, id) {
  var shaders =
      {
        "shader-fs" : [ 
            "varying vec2 vTextureCoord;",
            "uniform sampler2D uSampler;", 
            "void main(void) {",
            "    gl_FragColor = texture2D(uSampler, vTextureCoord);", 
            "}" ].join("\n"),

        "shader-vs" : [
            "attribute vec3 aVertexPosition;",
            "attribute vec2 aTextureCoord;",
            "uniform mat4 uMVMatrix;",
            "uniform mat4 uPMatrix;",
            "varying vec2 vTextureCoord;",
            "void main(void) {",
            "    gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);",
            "    vTextureCoord = aTextureCoord;",
            "}" ].join("\n")
      };
  var shader;
  if (!shaders.hasOwnProperty(id))
    return null;
  var str = shaders[id];

  if (id.match(/-fs/)) {
    shader = gl.createShader(gl.FRAGMENT_SHADER);
  } else if (id.match(/-vs/)) {
    shader = gl.createShader(gl.VERTEX_SHADER);
  } else {
    return null;
  }

  gl.shaderSource(shader, str);
  gl.compileShader(shader);

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert(gl.getShaderInfoLog(shader));
    return null;
  }

  return shader;
}

function initShaders() {
  var fragmentShader = getShader(gl, "shader-fs");
  var vertexShader = getShader(gl, "shader-vs");

  shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert("Could not initialise shaders");
  }

  gl.useProgram(shaderProgram);

  shaderProgram.vertexPositionAttribute =
      gl.getAttribLocation(shaderProgram, "aVertexPosition");
  gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);

  shaderProgram.textureCoordAttribute =
      gl.getAttribLocation(shaderProgram, "aTextureCoord");
  gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);

  shaderProgram.pMatrixUniform =
      gl.getUniformLocation(shaderProgram, "uPMatrix");
  shaderProgram.mvMatrixUniform =
      gl.getUniformLocation(shaderProgram, "uMVMatrix");
  shaderProgram.samplerUniform =
      gl.getUniformLocation(shaderProgram, "uSampler");
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
  clgl.enqueueAcquireGLObjects(cqCommandQueue, cmCL_PBO);

  // Compute on GPU and get kernel processing time
  BoxFilterGPU(uiInput, cmCL_PBO, uiImageWidth, uiImageHeight, iRadius, fScale);

  // Release GL output or explicit output copy
  // Release buffer
  clgl.enqueueReleaseGLObjects(cqCommandQueue, cmCL_PBO);
  cqCommandQueue.finish();

  // Copy results back to host memory, block until complete
  /*var uiOutput=new Uint8Array(szBuffBytes);
  cqCommandQueue.enqueueReadBuffer(cmDevBufOut, cl.TRUE, {
    offset: 0, 
    size: szBuffBytes, 
    buffer: uiOutput
  });
  
  // PNG uses 32-bit images, JPG can only work on 24-bit images
  if(!uiInput.save('out_'+iRadius+'.png',uiOutput, 
      uiImageWidth,uiImageHeight, uiInput.pitch, uiInput.bpp, 0xFF0000, 0x00FF00, 0xFF))
    log("Error saving image");

  return;*/
  
  // Update the texture from the pbo
  gl.bindTexture(gl.TEXTURE_2D, tex_screen);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, uiImageWidth, uiImageHeight,
      gl.RGBA, gl.UNSIGNED_BYTE, null);
  gl.activeTexture(gl.TEXTURE0);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);

  // Draw processed image
  displayTexture(tex_screen);

  // Flip backbuffer to screen
  requestAnimationFrame(DisplayGL);
}

function setMatrixUniforms() {
  gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, pMatrix);
  gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, mvMatrix);
}

function displayTexture(texture) {

  // render a screen sized quad
  gl.disable(gl.DEPTH_TEST);
  gl.enable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, texture);

  mat4.ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0, pMatrix);
  mat4.identity(mvMatrix);

  gl.viewport(0, 0, iGraphicsWinWidth, iGraphicsWinHeight);

  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
  gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute,
      cubeVertexPositionBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexTextureCoordBuffer);
  gl.vertexAttribPointer(shaderProgram.textureCoordAttribute,
      cubeVertexTextureCoordBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.bindTexture(gl.TEXTURE_2D, tex_screen);
  gl.uniform1i(shaderProgram.samplerUniform, 0);

  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
  setMatrixUniforms();
  gl.drawElements(gl.TRIANGLES, cubeVertexIndexBuffer.numItems,
      gl.UNSIGNED_SHORT, 0);

  gl.disable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, null);
}

function processKey(evt) {
  //log('process key: '+evt.sym);
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
  fScale = 1 / (2 * iRadius + 1);
  ResetKernelArgs(uiImageWidth, uiImageHeight, iRadius, fScale);
}