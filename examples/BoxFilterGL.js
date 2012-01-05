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
    nodejs=true,
    log=console.log,
    requestAnimationFrame = document.requestAnimationFrame;

//Read and eval library for mat/vec operations
eval(fs.readFileSync(__dirname+ '/glMatrix-0.9.5.min.js','utf8'));

//First check if the webcl extension is installed at all 
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. " +
  "Make sure that you have the WebCL extension installed.");
  return;
}

// Box processing params
var uiNumOutputPix = 64;                    // Default output pix per workgroup... may be modified depending HW/OpenCl caps
var iRadius = 10;                           // initial radius of 2D box filter mask
var fScale = 1/(2 * iRadius + 1);  // precalculated GV rescaling value

// OpenCL variables
var ckBoxRowsLmem;            // OpenCL Kernel for row sum (using lmem)
var ckBoxRowsTex;             // OpenCL Kernel for row sum (using 2d Image/texture)
var ckBoxColumns;             // OpenCL for column sum and normalize
var cmDevBufIn;               // OpenCL device memory object (buffer or 2d Image) for input data
var cmDevBufTemp;             // OpenCL device memory temp buffer object  
var cmDevBufOut;              // OpenCL device memory output buffer object
var szBuffBytes;              // Size of main image buffers
var szGlobalWorkSize=[0,0];      // global # of work items
var szLocalWorkSize= [0,0];       // work group # of work items 
var szMaxWorkgroupSize = 512; // initial max # of work items
var cmCL_PBO=0;          // OpenCL representation of GL pixel buffer 
var clgl; // CL's OpenGL extensions

// GL variables
var gl;                       // GL Context
var pbo;                      // Pixel buffer
var tex_screen;               // WebGLTexture
var image;                    // Image
var mvMatrix = mat4.create(); // model-view matrix
var pMatrix = mat4.create();  // projection matrix
var squareVertexPositionBuffer;
var shaders= {
    "shader-fs" : 
      [     
       "precision mediump float;",
       "varying vec2 vTextureCoord;",
       "uniform sampler2D uSampler;",
       "void main(void) {",
       "    gl_FragColor = texture2D(uSampler, vec2(vTextureCoord.s, vTextureCoord.t));",
       "}"
       ].join("\n"),

     "shader-vs" : 
     [ 
      "attribute vec3 aVertexPosition;",
    "attribute vec2 aTextureCoord;",
    "uniform mat4 uMVMatrix;",
    "uniform mat4 uPMatrix;",
    "varying vec2 vTextureCoord;",
    "void main(void) {",
    "    gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);",
    "    vTextureCoord = aTextureCoord;",
    "}"
          ].join("\n")
};

document.createWindow(800,800);
document.setTitle("BoxFilterGL");
requestAnimFrame = document.requestAnimationFrame;

WebGLStart();
WebCLStart(gl);
tick();

function WebCLStart(gl) {
  //Pick platform
  var platformList=cl.getPlatforms();
  var platform=platformList[0];
  
  //Query the set of GPU devices on this platform
  var devices = platform.getDevices(cl.DEVICE_TYPE_GPU);
  log("  # of Devices Available = "+devices.length); 
  var uiTargetDevice = clu.clamp(uiTargetDevice, 0, (devices.length - 1));
  var device=devices[uiTargetDevice];
  log("  Using Device "+ uiTargetDevice+": "+device.getDeviceInfo(cl.DEVICE_NAME)); 
  
  var hasImageSupport=device.getDeviceInfo(cl.DEVICE_IMAGE_SUPPORT);
  log(hasImageSupport ? "Image supported ;-)" : "No image support");
  if(!hasImageSupport) return;
  
  var extensions=device.getDeviceInfo(cl.DEVICE_EXTENSIONS);
  var hasGLSupport = extensions.search(/gl.sharing/i) >= 0;
  log(hasGLSupport ? "GL-CL extension available ;-)" : "No GL support");
  if(!hasGLSupport) return;
  clgl=device.getExtension(cl.GL_CONTEXT_KHR);
  if(clgl==undefined) return;
  
  var numComputeUnits=device.getDeviceInfo(cl.DEVICE_MAX_COMPUTE_UNITS);
  log('  # of Compute Units = '+numComputeUnits);
  
  if(hasGLSupport) {
    var props = [
        cl.GL_CONTEXT_KHR, gl, 
        cl.CONTEXT_PLATFORM, platform, 
    ];
    context=cl.createContext(device, props);
  }
  else
    context=cl.createContext(device);

  // Create a command-queue 
  queue=context.createCommandQueue(device, 0);
  
  // Allocate OpenCL object for the source data
  var InputFormat= {
      order : cl.RGBA,
      data_type : cl.UNSIGNED_INT8
  };
  
  //2D Image (Texture) on device
  log("Image: "+image.width+"x"+image.height+" pitch: "+image.pitch);
  cmDevBufIn = context.createImage2D(cl.MEM_READ_ONLY | cl.MEM_USE_HOST_PTR, InputFormat, 
      image.width, image.height, image.pitch , image);
  
  RowSampler = context.createSampler(false, cl.ADDRESS_CLAMP, cl.FILTER_NEAREST);
  
  // Allocate the OpenCL intermediate and result buffer memory objects on the device GMEM
  cmDevBufTemp = context.createBuffer(cl.MEM_READ_WRITE, szBuffBytes);
  cmDevBufOut = context.createBuffer(cl.MEM_WRITE_ONLY, szBuffBytes);
  
  // Create OpenCL representation of OpenGL PBO
  cmCL_PBO = clgl.createFromGLBuffer(context, cl.MEM_WRITE_ONLY, pbo);

  //Create the program 
  sourceCL = fs.readFileSync(__dirname+'/BoxFilter.cl','ascii');
  cpProgram = context.createProgram(sourceCL);
  
  sBuildOpts = "-cl-fast-relaxed-math -D USETEXTURE";
  ciErrNum = cpProgram.build(device, sBuildOpts);
  
  // Create kernels
  ckBoxRowsTex = cpProgram.createKernel("BoxRowsTex");
  ckBoxColumns = cpProgram.createKernel("BoxColumns");
  
  // set the kernel args
  ResetKernelArgs(image.width, image.height, iRadius, fScale);
}

function renderToImage() {
  // Warmup call to assure OpenCL driver is awake
  BoxFilterGPU (image, cmDevBufOut, iRadius, fScale);
  queue.finish();
  
  // launch processing on the GPU
  BoxFilterGPU (image, cmDevBufOut, iRadius, fScale);
  queue.finish();
  
  // Copy results back to host memory, block until complete
  var uiOutput=new Uint8Array(szBuffBytes);
  queue.enqueueReadBuffer(cmDevBufOut, cl.TRUE, {
    offset: 0, 
    size: szBuffBytes, 
    buffer: uiOutput
  });
  
  // PNG uses 32-bit images, JPG can only work on 24-bit images
  if(!Image.save('out_'+iRadius+'.png',uiOutput, image.width,image.height, image.pitch, image.bpp, 0xFF0000, 0x00FF00, 0xFF))
    log("Error saving image");
}

function ResetKernelArgs(width, height, r, fScale)
{
  // (Image/texture version)
  ckBoxRowsTex.setArg(0, cmDevBufIn, cl.type.MEM);
  ckBoxRowsTex.setArg(1, cmDevBufTemp, cl.type.MEM);
  ckBoxRowsTex.setArg(2, RowSampler, cl.type.SAMPLER); 
  ckBoxRowsTex.setArg(3, width, cl.type.INT | cl.type.UNSIGNED);
  ckBoxRowsTex.setArg(4, height, cl.type.INT | cl.type.UNSIGNED);
  ckBoxRowsTex.setArg(5, r, cl.type.INT);
  ckBoxRowsTex.setArg(6, fScale, cl.type.FLOAT);

  // Set the Argument values for the column kernel
  ckBoxColumns.setArg(0, cmDevBufTemp, cl.type.MEM);
  ckBoxColumns.setArg(1, cmDevBufOut, cl.type.MEM);
  ckBoxColumns.setArg(2, width, cl.type.INT | cl.type.UNSIGNED);
  ckBoxColumns.setArg(3, height, cl.type.INT | cl.type.UNSIGNED);
  ckBoxColumns.setArg(4, r, cl.type.INT);
  ckBoxColumns.setArg(5, fScale, cl.type.FLOAT);
}

//OpenCL computation function for GPU:  
//Copies input data to the device, runs kernel, copies output data back to host  
//*****************************************************************************
function BoxFilterGPU(image, cmOutputBuffer, r, fScale)
{
  log('BoxFilterGPU');
  
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer, cl.type.MEM);

  // Copy input data from host to device 
  var szTexOrigin = [0, 0, 0];                // Offset of input texture origin relative to host image
  var szTexRegion = [image.width, image.height, 1];   // Size of texture region to operate on
  log('  enqueue image: origin='+szTexOrigin+", region="+szTexRegion);
  queue.enqueueWriteImage(cmDevBufIn, cl.TRUE, szTexOrigin, szTexRegion, 0, 0, image);

  // Set global and local work sizes for row kernel
  szLocalWorkSize[0] = uiNumOutputPix;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0]= szLocalWorkSize[0] * clu.DivUp(image.height, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  log("row kernel work sizes: global="+szGlobalWorkSize+" local="+szLocalWorkSize);
  
  //Sync host
  queue.finish();

  //Launch row kernel
  queue.enqueueNDRangeKernel(ckBoxRowsTex, null, szGlobalWorkSize, szLocalWorkSize);

  //Set global and local work sizes for column kernel
  szLocalWorkSize[0] = 64;
  szLocalWorkSize[1] = 1;
  szGlobalWorkSize[0] = szLocalWorkSize[0] * clu.DivUp(image.width, szLocalWorkSize[0]);
  szGlobalWorkSize[1] = 1;
  log("  column kernel work sizes: global="+szGlobalWorkSize+" local="+szLocalWorkSize);

  //Launch column kernel
  queue.enqueueNDRangeKernel(ckBoxColumns, null, szGlobalWorkSize, szLocalWorkSize);

  //sync host
  queue.finish();
}

/////////////////////////////////////////////////////////////////////////
//
//              WebGL section
//
/////////////////////////////////////////////////////////////////////////
function createPBO(image_width, image_height)
{
    // set up data parameter
    var num_texels = image_width * image_height;
    var num_values = num_texels * 4;
    var size_tex_data = Int8Array.BYTES_PER_ELEMENT * num_values;

    // create pixel buffer object
    var pbo = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, pbo);

    // buffer data
    gl.bufferData(gl.ARRAY_BUFFER, size_tex_data, gl.DYNAMIC_DRAW);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
    
    return pbo;
}

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
  
}


function getShader(gl, id) {
  var shader;
  if(nodejs) {
    if (!shaders.hasOwnProperty(id)) return null;
    var str = shaders[id];

    if (id.match(/-fs/)) {
      shader = gl.createShader(gl.FRAGMENT_SHADER);
    } else if (id.match(/-vs/)) {
      shader = gl.createShader(gl.VERTEX_SHADER);
    } else { return null; }

  }
  else {
    var shaderScript = document.getElementById(id);
    if (!shaderScript) {
      return null;
    }

    var str = "";
    var k = shaderScript.firstChild;
    while (k) {
      if (k.nodeType == 3) {
        str += k.textContent;
      }
      k = k.nextSibling;
    }
    if (shaderScript.type == "x-shader/x-fragment") {
      shader = gl.createShader(gl.FRAGMENT_SHADER);
    } else if (shaderScript.type == "x-shader/x-vertex") {
      shader = gl.createShader(gl.VERTEX_SHADER);
    } else {
      return null;
    }
  }

  gl.shaderSource(shader, str);
  gl.compileShader(shader);

  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    alert(gl.getShaderInfoLog(shader));
    return null;
  }

  return shader;
}

var shaderProgram;

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

  shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
  gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);

  shaderProgram.textureCoordAttribute = gl.getAttribLocation(shaderProgram, "aTextureCoord");
  gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);

  shaderProgram.pMatrixUniform = gl.getUniformLocation(shaderProgram, "uPMatrix");
  shaderProgram.mvMatrixUniform = gl.getUniformLocation(shaderProgram, "uMVMatrix");
  shaderProgram.samplerUniform = gl.getUniformLocation(shaderProgram, "uSampler");
}


function handleLoadedTexture(texture) {
  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, texture.image.width,texture.image.height,
      0,gl.RGBA, gl.UNSIGNED_BYTE, null /*texture.image*/);
  gl.bindTexture(gl.TEXTURE_2D, null);
}


function initTexture() {
  tex_screen = gl.createTexture();
  image=tex_screen.image = new Image();
  tex_screen.image.onload(function (s) { // [MBS] was onload() event
    console.log("Loaded image: "+s);
    console.log("size: "+tex_screen.image.width+"x"+tex_screen.image.height);
    szBuffBytes = image.height*image.pitch;
    handleLoadedTexture(tex_screen)
  });

  tex_screen.image.src = nodejs ? __dirname+"/lenaRGB.jpg" : "lenaRGB.jpg";
}


var mvMatrix = mat4.create();
var mvMatrixStack = [];
var pMatrix = mat4.create();

function mvPushMatrix() {
  var copy = mat4.create();
  mat4.set(mvMatrix, copy);
  mvMatrixStack.push(copy);
}

function mvPopMatrix() {
  if (mvMatrixStack.length == 0) {
    throw "Invalid popMatrix!";
  }
  mvMatrix = mvMatrixStack.pop();
}


function setMatrixUniforms() {
  gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, pMatrix);
  gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, mvMatrix);
}


function degToRad(degrees) {
  return degrees * Math.PI / 180;
}

var cubeVertexPositionBuffer;
var cubeVertexTextureCoordBuffer;
var cubeVertexIndexBuffer;

function initBuffers() {
  cubeVertexPositionBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
  vertices = [
              // Front face
              -1.0, -1.0,  0.5,
              1.0, -1.0,  0.5,
              1.0,  1.0,  0.5,
              -1.0,  1.0,  0.5,
              ];
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);
  cubeVertexPositionBuffer.itemSize = 3;
  cubeVertexPositionBuffer.numItems = 4;

  cubeVertexTextureCoordBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexTextureCoordBuffer);
  var textureCoords = [
                       // Front face
                       0.0, 0.0,
                       1.0, 0.0,
                       1.0, 1.0,
                       0.0, 1.0,
                       ];
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoords), gl.STATIC_DRAW);
  cubeVertexTextureCoordBuffer.itemSize = 2;
  cubeVertexTextureCoordBuffer.numItems = 4;

  cubeVertexIndexBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
  var cubeVertexIndices = [
                           0, 1, 2,      0, 2, 3,    // Front face
                           ];
  gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(cubeVertexIndices), gl.STATIC_DRAW);
  cubeVertexIndexBuffer.itemSize = 1;
  cubeVertexIndexBuffer.numItems = 6;
}


function drawScene() {
  log('drawScene');
  // Sync GL and acquire buffer from GL
  log('  Sync GL and acquire buffer from GL');
  gl.finish();
  clgl.enqueueAcquireGLObjects(queue, [cmCL_PBO]);

  // Compute on GPU
  log('  Compute on GPU');
  BoxFilterGPU (image, cmCL_PBO, iRadius, fScale);

  // Release buffer
  log('  Release buffer');
  clgl.enqueueReleaseGLObjects(queue, [cmCL_PBO]);
  queue.finish();

  // Update the texture from the pbo
  log('  Update the texture from the pbo');
  gl.bindTexture(gl.TEXTURE_2D, tex_screen);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, image.width, image.height, gl.RGBA, gl.UNSIGNED_BYTE, null);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);
  
  // Draw processed image
  log('  Draw processed image');
  gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  gl.enable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, tex_screen);

  mat4.ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0, pMatrix);

  mat4.identity(mvMatrix);

  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexPositionBuffer);
  gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, cubeVertexPositionBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.bindBuffer(gl.ARRAY_BUFFER, cubeVertexTextureCoordBuffer);
  gl.vertexAttribPointer(shaderProgram.textureCoordAttribute, cubeVertexTextureCoordBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.uniform1i(shaderProgram.samplerUniform, 0);

  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, cubeVertexIndexBuffer);
  setMatrixUniforms();
  gl.drawElements(gl.TRIANGLES, cubeVertexIndexBuffer.numItems, gl.UNSIGNED_SHORT, 0);

  gl.disable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, null);

  requestAnimFrame(drawScene);
}


function WebGLStart() {
  var canvas = document.getElementById("mycanvas");
  initGL(canvas);
  initShaders();
  initBuffers();
  initTexture();

  // pixel buffer for WebCL
  pbo = createPBO(image.width, image.height);

  gl.clearColor(0.0, 0.0, 0.0, 1.0);
  gl.enable(gl.DEPTH_TEST);
}

