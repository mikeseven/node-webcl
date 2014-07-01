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

var nodejs = (typeof window === 'undefined');
if(nodejs) {
  WebCL = require('../webcl');
  clu = require('../lib/clUtils');
  util = require('util');
  fs = require('fs');
  WebGL = require('node-webgl');
  document = WebGL.document();
  Image = WebGL.Image;
  log = console.log;
  alert = console.log;
}

requestAnimationFrame = document.requestAnimationFrame;

var use_gpu = true;
var image;

var COMPUTE_KERNEL_FILENAME = "BoxFilter.cl";
var WIDTH = 800;
var HEIGHT = 800;

// cl stuff
var /* cl_context */        ComputeContext;
var /* cl_command_queue */  ComputeCommands;
var /* cl_program */        ComputeProgram;
var /* cl_device_id */      ComputeDeviceId;
var /* cl_device_type */    ComputeDeviceType;
var /* cl_sampler */        RowSampler;
var /* cl_mem */            ComputeBufTemp, ComputePBO;
var /* cl_kernel */         ckBoxRowsTex, ckBoxColumns;
var MaxWorkGroupSize;

var Width = WIDTH;
var Height = HEIGHT;
var Reshaped = false;
var Update = false;
var newWidth, newHeight; // only when reshape

// simulation
var uiNumOutputPix = 32; // Default output pix per workgroup... may be modified depending HW/OpenCl caps
var iRadius = 10; // initial radius of 2D box filter mask
var fScale = 1.0 / (2 * iRadius + 1.0); // precalculated GV rescaling value

// gl stuff
var gl;
var shaderProgram;
var pbo;
var TextureId = null;
var TextureWidth = WIDTH;
var TextureHeight = HEIGHT;
var VertexPosBuffer, TexCoordsBuffer;
var canvas;

function initialize(device_type) {
  log('Initializing');
  document.setTitle("OpenCL GPU BoxFilter Demo");
  canvas = document.createElement("fbo-canvas", Width, Height);

  // install UX callbacks
  document.addEventListener('resize', reshape);
  document.addEventListener('keydown', keydown);

  var err = init_gl(canvas);
  if (err != WebCL.SUCCESS)
    return err;

  err = init_cl(device_type);
  if (err != 0)
    return err;

  configure_shared_data(image.width, image.height);

  // Warmup call to assure OpenCL driver is awake
  resetKernelArgs(image.width, image.height, iRadius, fScale);
  
  BoxFilterGPU(image, ComputePBO, iRadius, fScale);
  
  ComputeCommands.finish();

  return WebCL.SUCCESS;
}

// /////////////////////////////////////////////////////////////////////
// OpenGL stuff
// /////////////////////////////////////////////////////////////////////

function configure_shared_data(image_width, image_height) {
  log('configure shared data');
  
  // set up data parameter
  var num_texels = image_width * image_height;
  var num_values = num_texels * 4;
  var size_tex_data = 1 * num_values; // 1 is GL texture type UNSIGNED_BYTE size

  // create buffer object
  if (pbo) {
    gl.bindBuffer(gl.ARRAY_BUFFER, pbo);
    gl.deleteBuffer(pbo);
    gl.bindBuffer(gl.ARRAY_BUFFER, null);
  }
  pbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, pbo);

  // buffer data
  gl.bufferData(gl.ARRAY_BUFFER, size_tex_data, gl.DYNAMIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);

  // Create OpenCL representation of OpenGL PBO
  ComputePBO = ComputeContext.createFromGLBuffer(WebCL.MEM_WRITE_ONLY, pbo);
  if (!ComputePBO) {
    alert("Error: Failed to create CL PBO buffer");
    return -1;
  }
}

function init_textures(width, height) {
  log('  Init textures');

  if (TextureId)
    gl.deleteTexture(TextureId);
  TextureId = null;

  TextureWidth = width;
  TextureHeight = height;

  gl.activeTexture(gl.TEXTURE0);
  TextureId = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, TextureId);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, TextureWidth, TextureHeight, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  gl.bindTexture(gl.TEXTURE_2D, null);
}

function init_buffers() {
  log('  create buffers');
  var VertexPos = [ -1, -1, 
                    1, -1, 
                    1, 1, 
                    -1, 1 ];
  var TexCoords = [ 0, 0, 
                    1, 0, 
                    1, 1, 
                    0, 1 ];

  VertexPosBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, VertexPosBuffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(VertexPos), gl.STATIC_DRAW);
  VertexPosBuffer.itemSize = 2;
  VertexPosBuffer.numItems = 4;

  TexCoordsBuffer = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, TexCoordsBuffer);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(TexCoords), gl.STATIC_DRAW);
  TexCoordsBuffer.itemSize = 2;
  TexCoordsBuffer.numItems = 4;
}

function compile_shader(gl, id) {
  var shaders = {
    "shader-vs" : [ 
        "attribute vec3 aCoords;",
        "attribute vec2 aTexCoords;", 
        "varying vec2 vTexCoords;",
        "void main(void) {", 
        "    gl_Position = vec4(aCoords, 1.0);",
        "    vTexCoords = aTexCoords;", 
        "}" ].join("\n"),
    "shader-fs" : [
         "#ifdef GL_ES",
         "  precision mediump float;",
         "#endif",
         "varying vec2 vTexCoords;",
         "uniform sampler2D uSampler;",
         "void main(void) {",
         "    gl_FragColor = texture2D(uSampler, vTexCoords.st);",
         "}" ].join("\n"),
  };

  var shader;
  if (nodejs) {
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

  } else {
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

function init_shaders() {
  log('  Init shaders');
  var fragmentShader = compile_shader(gl, "shader-fs");
  var vertexShader = compile_shader(gl, "shader-vs");

  shaderProgram = gl.createProgram();
  gl.attachShader(shaderProgram, vertexShader);
  gl.attachShader(shaderProgram, fragmentShader);
  gl.linkProgram(shaderProgram);

  if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
    alert("Could not initialise shaders");
  }

  gl.useProgram(shaderProgram);

  shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aCoords");
  gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);

  shaderProgram.textureCoordAttribute = gl.getAttribLocation(shaderProgram, "aTexCoords");
  gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);
  
  shaderProgram.samplerUniform = gl.getUniformLocation(shaderProgram, "uSampler");
}

function init_gl(canvas) {
  log('Init GL');
  try {
    gl = canvas.getContext("experimental-webgl");
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;
  } catch (e) {
  }
  if (!gl) {
    alert("Could not initialise WebGL, sorry :-(");
    return -1;
  }

  init_buffers();
  init_shaders();
  init_textures(image.width, image.height);

  return WebCL.SUCCESS;
}

function renderTexture() {
  // we just draw a screen-aligned texture
  gl.viewport(0, 0, Width, Height);

  gl.enable(gl.TEXTURE_2D);
  gl.bindTexture(gl.TEXTURE_2D, TextureId);

  // draw screen aligned quad
  gl.bindBuffer(gl.ARRAY_BUFFER, VertexPosBuffer);
  gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute,
      VertexPosBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.bindBuffer(gl.ARRAY_BUFFER, TexCoordsBuffer);
  gl.vertexAttribPointer(shaderProgram.textureCoordAttribute,
      TexCoordsBuffer.itemSize, gl.FLOAT, false, 0, 0);

  gl.activeTexture(gl.TEXTURE0);
  gl.uniform1i(shaderProgram.samplerUniform, 0);

  gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);

  gl.bindTexture(gl.TEXTURE_2D, null);
  gl.disable(gl.TEXTURE_2D);
}

// /////////////////////////////////////////////////////////////////////
// OpenCL stuff
// /////////////////////////////////////////////////////////////////////

function init_cl(device_type) {
  log('init CL');
  ComputeDeviceType = device_type ? WebCL.DEVICE_TYPE_GPU : WebCL.DEVICE_TYPE_DEFAULT;

  // Pick platform
  // var platformList = WebCL.getPlatforms();
  // var platform = platformList[0];

  // create the OpenCL context
  ComputeContext = WebCL.createContext({
    deviceType: ComputeDeviceType, 
    shareGroup: gl, 
    // platform: platform 
  });

  var device_ids = ComputeContext.getInfo(WebCL.CONTEXT_DEVICES);
  if (!device_ids) {
    alert("Error: Failed to retrieve compute devices for context!");
    return -1;
  }

  var device_found = false;
  for(var i=0,l=device_ids.length;i<l;++i ) {
    device_type = device_ids[i].getInfo(WebCL.DEVICE_TYPE);
    if (device_type == ComputeDeviceType) {
      ComputeDeviceId = device_ids[i];
      device_found = true;
      break;
    }
  }

  if (!device_found) {
    alert("Error: Failed to locate compute device!");
    return -1;
  }

  var exts=ComputeDeviceId.getSupportedExtensions();
  log("Device extensions: "+exts);
  ComputeDeviceId.enableExtension("KHR_gl_sharing");

  // Create a command queue
  //
  ComputeCommands = ComputeContext.createCommandQueue(ComputeDeviceId, 0);
  if (!ComputeCommands) {
    alert("Error: Failed to create a command queue!");
    return -1;
  }

  // Report the device vendor and device name
  // 
  var vendor_name = ComputeDeviceId.getInfo(WebCL.DEVICE_VENDOR);
  var device_name = ComputeDeviceId.getInfo(WebCL.DEVICE_NAME);

  log("Connecting to " + vendor_name + " " + device_name);

  if (!ComputeDeviceId.getInfo(WebCL.DEVICE_IMAGE_SUPPORT)) {
    log("Application requires images: Images not supported on this device.");
    return WebCL.IMAGE_FORMAT_NOT_SUPPORTED;
  }

  err = init_cl_buffers();
  if (err != WebCL.SUCCESS) {
    log("Failed to create compute result! Error " + err);
    return err;
  }

  err = init_cl_kernels();
  if (err != WebCL.SUCCESS) {
    log("Failed to setup compute kernel! Error " + err);
    return err;
  }

  return WebCL.SUCCESS;
}

function init_cl_kernels() {
  log('  setup CL kernel');

  ComputeProgram = null;

  log("Loading kernel source from file '" + COMPUTE_KERNEL_FILENAME + "'...");
  source = fs.readFileSync(__dirname + '/' + COMPUTE_KERNEL_FILENAME, 'ascii');
  if (!source) {
    alert("Error: Failed to load kernel source!");
    return -1;
  }

  // Create the compute program from the source buffer
  //
  ComputeProgram = ComputeContext.createProgram(source);
  if (!ComputeProgram) {
    alert("Error: Failed to create compute program!");
    return -1;
  }

  // Build the program executable
  //
  try {
    ComputeProgram.build(ComputeDeviceId, "-cl-fast-relaxed-math");
  } catch (err) {
    log('Error building program: ' + err);
    alert("Error: Failed to build program executable!\n"
        + ComputeProgram.getBuildInfo(ComputeDeviceId, WebCL.PROGRAM_BUILD_LOG));
    return -1;
  }

  // Create the compute kernels from within the program
  //
  ckBoxRowsTex = ComputeProgram.createKernel('BoxRowsTex');
  if (!ckBoxRowsTex) {
    alert("Error: Failed to create compute row kernel!");
    return -1;
  }
  ckBoxColumns = ComputeProgram.createKernel('BoxColumns');
  if (!ckBoxColumns) {
    alert("Error: Failed to create compute column kernel!");
    return -1;
  }
  return WebCL.SUCCESS;
}

function resetKernelArgs(image_width, image_height, r, scale) {
  log('Reset kernel args to image ' + image_width + "x" + image_height + " r="
      + r + " scale=" + scale);

  // set the kernel args
  try {
    // Set the Argument values for the row kernel
    ckBoxRowsTex.setArg(0, ComputeTexture);
    ckBoxRowsTex.setArg(1, ComputeBufTemp);
    ckBoxRowsTex.setArg(2, RowSampler, WebCL.type.SAMPLER);
    ckBoxRowsTex.setArg(3, image_width, WebCL.type.INT);
    ckBoxRowsTex.setArg(4, image_height, WebCL.type.INT);
    ckBoxRowsTex.setArg(5, r, WebCL.type.INT);
    ckBoxRowsTex.setArg(6, scale, WebCL.type.FLOAT);
  } catch (err) {
    alert("Failed to set row kernel args! " + err);
    return -10;
  }

  try {
    // Set the Argument values for the column kernel
    ckBoxColumns.setArg(0, ComputeBufTemp);
    ckBoxColumns.setArg(1, ComputePBO);
    ckBoxColumns.setArg(2, image_width, WebCL.type.INT);
    ckBoxColumns.setArg(3, image_height, WebCL.type.INT);
    ckBoxColumns.setArg(4, r, WebCL.type.INT);
    ckBoxColumns.setArg(5, scale, WebCL.type.FLOAT);
  } catch (err) {
    alert("Failed to set column kernel args! " + err);
    return -10;
  }

  // Get the maximum work group size for executing the kernel on the device
  //
  MaxWorkGroupSize = ckBoxRowsTex.getWorkGroupInfo(ComputeDeviceId,
      WebCL.KERNEL_WORK_GROUP_SIZE);

  log("  MaxWorkGroupSize: " + MaxWorkGroupSize);
  return WebCL.SUCCESS;
}

function init_cl_buffers() {
  log('  create CL buffers');

  // 2D Image (Texture) on device
  var InputFormat = {
    order : WebCL.RGBA,
    data_type : WebCL.UNSIGNED_INT8,
    size: [ image.width, image.height ],
    rowPitch: image.pitch
  };
  ComputeTexture = ComputeContext.createImage(WebCL.MEM_READ_ONLY | WebCL.MEM_USE_HOST_PTR, InputFormat, image);
  if (!ComputeTexture) {
    alert("Error: Failed to create a Image2D on device");
    return -1;
  }

  RowSampler = ComputeContext.createSampler(false, WebCL.ADDRESS_CLAMP, WebCL.FILTER_NEAREST);
  if (!RowSampler) {
    alert("Error: Failed to create a row sampler");
    return -1;
  }

  // Allocate the OpenCL intermediate and result buffer memory objects on the device GMEM
  ComputeBufTemp = ComputeContext.createBuffer(WebCL.MEM_READ_WRITE, image.szBuffBytes);
  if (!ComputeBufTemp) {
    alert("Error: Failed to create temporary buffer");
    return -1;
  }

  return WebCL.SUCCESS;
}

function cleanup() {
  document.removeEventListener('resize', reshape);
  document.removeEventListener('keydown', keydown);
  ComputeCommands.finish();
  ckBoxRowsTex = null;
  ckBoxColumns = null;
  RowSampler = null;
  ComputeProgram = null;
  ComputeCommands = null;
  ComputePBO = null;
  ComputeBufTemp=null;
  ComputeContext = null;
}

function shutdown() {
  log("Shutting down...");
  cleanup();
  process.exit(0);
}

// /////////////////////////////////////////////////////////////////////
// rendering loop
// /////////////////////////////////////////////////////////////////////

function display(timestamp) {
  //FrameCount++;
  //var uiStartTime = new Date().getTime();

  gl.clearColor(0, 0, 0, 0);
  gl.clear(gl.COLOR_BUFFER_BIT);

  if (Reshaped) {
    Reshaped = false;
    Width = newWidth;
    Height = newHeight;
    gl.viewport(0, 0, Width, Height);
    gl.clear(gl.COLOR_BUFFER_BIT);
  }

  var err = execute_kernel();
  if (err != 0) {
    alert("Error " + err + " from execute_kernel!");
    process.exit(1);
  }

  renderTexture();
  //reportInfo();

  gl.finish(); // for timing

  //var uiEndTime = new Date().getTime();
  //ReportStats(uiStartTime, uiEndTime);
  //DrawText(TextOffset[0], TextOffset[1], 1, (Animated == 0) ? "Press space to animate" : " ");
  return WebCL.SUCCESS;
}

function reshape(evt) {
  newWidth = evt.width;
  newHeight = evt.height;
  Reshaped = true;
}

function keydown(evt) {
  log('process key: ' + evt.which);
  var oldr = iRadius;

  switch (evt.which) {
  case '='.charCodeAt(0): // + or = increases filter radius
    if ((MaxWorkGroupSize - (((iRadius + 1 + 15) / 16) * 16) - iRadius - 1) > 0)
      iRadius++;
    break;
  case '-'.charCodeAt(0): // - or _ decreases filter radius
    if (iRadius > 1)
      iRadius--;
    break;
  }
  if (oldr != iRadius) {
    Update = true;
  }
}

function execute_kernel() {
  //log('execute_kernel...');

  if (Update) {
    Update = false;

    // Update filter parameters
    log('Updating for new radius: ' + iRadius);
    fScale = 1 / (2 * iRadius + 1);
    resetKernelArgs(image.width, image.height, iRadius, fScale);
  }

  // Sync GL and acquire buffer from GL
  gl.finish();
  ComputeCommands.enqueueAcquireGLObjects(ComputePBO);

  BoxFilterGPU(image, ComputePBO, iRadius, fScale);

  // Release buffer
  ComputeCommands.enqueueReleaseGLObjects(ComputePBO);
  ComputeCommands.finish();
  
  // Update the texture from the pbo
  gl.bindTexture(gl.TEXTURE_2D, TextureId);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, image.width, image.height, gl.RGBA,
      gl.UNSIGNED_BYTE, null);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);

  return WebCL.SUCCESS;
}

function BoxFilterGPU(image, cmOutputBuffer, r, scale) {
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer, WebCL.type.MEM);

  // 2D Image (Texture)
  var TexOrigin = [ 0, 0, 0 ]; // Offset of input texture origin relative to host image
  var TexRegion = [ image.width, image.height, 1 ]; // Size of texture region to operate on
  ComputeCommands.enqueueWriteImage(ComputeTexture, WebCL.TRUE, TexOrigin,
      TexRegion, 0, 0, image);

  // Set global and local work sizes for row kernel
  var local = [ uiNumOutputPix, 1 ];
  var global = [ clu.DivUp(image.height, local[0]) * local[0], 1 ];

  try {
    ComputeCommands.enqueueNDRangeKernel(ckBoxRowsTex, null, global, local);
  } catch (err) {
    alert("Failed to enqueue row kernel! " + err);
    return err;
  }

  // Set global and local work sizes for column kernel
  local = [ uiNumOutputPix, 1 ];
  global = [ clu.DivUp(image.width, local[0]) * local[0], 1 ];

  try {
    ComputeCommands.enqueueNDRangeKernel(ckBoxColumns, null, global, local);
  } catch (err) {
    alert("Failed to enqueue column kernel! " + err);
    return err;
  }
}

function main() {
  // loading image
  image = new Image();
  image.onload=function() { 
    console.log("Loaded image: " + image.src);
    log("Image Width = " + image.width + ", Height = " + image.height + ", bpp = 32");
    image.szBuffBytes = image.height * image.pitch;
    Width=image.width;
    Height=image.height;
    
    // init window
    if(initialize(use_gpu)==WebCL.SUCCESS) {
      function update() {
        display();
        requestAnimationFrame(update);
      }
      update();
    }
  };
  image.src=__dirname+"/mike_scooter.jpg";
}

main();

