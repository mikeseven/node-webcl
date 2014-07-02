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

var COMPUTE_KERNEL_FILENAME = "gradient.cl";
var COMPUTE_KERNEL_NAME = "compute";
var WIDTH = 800;
var HEIGHT = 800;

// cl stuff
var /* cl_context */        ComputeContext;
var /* cl_command_queue */  ComputeCommands;
var /* cl_program */        ComputeProgram;
var /* cl_device_id */      ComputeDevice;
var /* cl_device_type */    ComputeDeviceType = WebCL.DEVICE_TYPE_GPU;
var /* cl_image */          ComputePBO;
var /* cl_kernel */         ckCompute;
var max_workgroup_size, max_workitem_sizes;

var Width = WIDTH;
var Height = HEIGHT;
var Reshaped = false;
var Update = false;
var iRadius = 1; //???
var newWidth, newHeight; // only when reshape

// gl stuff
var gl;
var shaderProgram;
var pbo;
var TextureId = null;
var TextureWidth = WIDTH;
var TextureHeight = HEIGHT;
var VertexPosBuffer, TexCoordsBuffer;
var canvas;

function initialize() {
  log('Initializing');
  document.setTitle("Test Image2D creation from GL");
  canvas = document.createElement("mycanvas", Width, Height);

  // install UX callbacks
  document.addEventListener('resize', reshape);
  document.addEventListener('keydown', keydown);

  var err = init_gl(canvas);
  if (err != WebCL.SUCCESS)
    return err;

  err = init_cl();
  if (err != 0)
    return err;

  configure_shared_data(TextureWidth, TextureHeight);

  Update=true;
  
  return WebCL.SUCCESS;
}

// /////////////////////////////////////////////////////////////////////
// OpenGL stuff
// /////////////////////////////////////////////////////////////////////

function configure_shared_data(width, height) {
  log('configure shared data');
  
  if (TextureId) {
    gl.deleteTexture(TextureId);
    TextureId = null;
  }

  TextureWidth = width;
  TextureHeight = height;

  gl.activeTexture(gl.TEXTURE0);
  TextureId = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, TextureId);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, TextureWidth, TextureHeight, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);
  gl.bindTexture(gl.TEXTURE_2D, null);

  // Create OpenCL representation of OpenGL PBO
  try {
    ComputePBO = ComputeContext.createFromGLTexture(WebCL.MEM_WRITE_ONLY, gl.TEXTURE_2D, 0, TextureId);
  }
  catch(ex) {
    alert("Error: Failed to create CL PBO buffer. "+ex);
    return -1;
  }
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

  return WebCL.SUCCESS;
}

// /////////////////////////////////////////////////////////////////////
// OpenCL stuff
// /////////////////////////////////////////////////////////////////////

function init_cl() {
  log('init CL');

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
      ComputeDevice = device_ids[i];
      device_found = true;
      break;
    }
  }

  if (!device_found) {
    alert("Error: Failed to locate compute device!");
    return -1;
  }

  // Create a command queue
  //
  ComputeCommands = ComputeContext.createCommandQueue(ComputeDevice, 0);
  if (!ComputeCommands) {
    alert("Error: Failed to create a command queue!");
    return -1;
  }

  // Report the device vendor and device name
  // 
  var vendor_name = ComputeDevice.getInfo(WebCL.DEVICE_VENDOR);
  var device_name = ComputeDevice.getInfo(WebCL.DEVICE_NAME);

  log("Connecting to " + vendor_name + " " + device_name);

  if (!ComputeDevice.getInfo(WebCL.DEVICE_IMAGE_SUPPORT)) {
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
  try {
    ComputeProgram = ComputeContext.createProgram(source);
  }
  catch(ex) {
    alert("Error: Failed to create compute program! "+ex);
    return -1;
  }

  // Build the program executable
  //
  try {
    ComputeProgram.build(ComputeDevice, "-cl-fast-relaxed-math");
  } catch (err) {
    log('Error building program: ' + err);
    alert("Error: Failed to build program executable!\n"
        + ComputeProgram.getBuildInfo(ComputeDevice, WebCL.PROGRAM_BUILD_LOG));
    return -1;
  }

  // Create the compute kernels from within the program
  //
  try {
    ckCompute = ComputeProgram.createKernel(COMPUTE_KERNEL_NAME);
  }
  catch(ex) {
    alert("Error: Failed to create compute row kernel! "+ex);
    return -1;
  }

  // Get the maximum work group size for executing the kernel on the device
  //
  max_workgroup_size = ckCompute.getWorkGroupInfo(ComputeDevice, WebCL.KERNEL_WORK_GROUP_SIZE);
  max_workitem_sizes=ComputeDevice.getInfo(WebCL.DEVICE_MAX_WORK_ITEM_SIZES);
  log('  max workgroup size: '+max_workgroup_size);
  log('  max workitem sizes: '+max_workitem_sizes);
  return WebCL.SUCCESS;
}

function resetKernelArgs(image_width, image_height) {
  log('Reset kernel args to image ' + image_width + "x" + image_height);

  // set the kernel args
  try {
    // Set the Argument values for the row kernel
    ckCompute.setArg(0, ComputePBO);
  } catch (err) {
    alert("Failed to set row kernel args! " + err);
    return -10;
  }

  return WebCL.SUCCESS;
}

function init_cl_buffers() {
  log('  create CL buffers');

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

  if (Reshaped) {
    log('reshaping texture to '+newWidth+" x "+newHeight);
    Reshaped = false;
    Width = newWidth;
    Height = newHeight;
    configure_shared_data(Width,Height);
    resetKernelArgs(Width,Height);
  }

  var err = execute_kernel();
  if (err != 0) {
    alert("Error " + err + " from execute_kernel!");
    process.exit(1);
  }

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

  //reportInfo();

  // gl.finish(); // for timing

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
}

function execute_kernel() {
  //log('execute_kernel...');

  if (Update) {
    Update = false;

    // Update filter parameters
    TextureWidth=Width;
    TextureHeight=Height;
    resetKernelArgs(TextureWidth, TextureHeight);
  }

  // Sync GL and acquire buffer from GL
  gl.flush();
  ComputeCommands.enqueueAcquireGLObjects(ComputePBO);

  // Set global and local work sizes for row kernel
  // var local = [ 16, max_workgroup_size/16 ];
  // var global = [ clu.DivUp(TextureWidth, local[0]) * local[0],
  //                clu.DivUp(TextureHeight, local[1]) * local[1] ];
  var local = null;
  var global = [ TextureWidth, TextureHeight ];

  try {
    ComputeCommands.enqueueNDRangeKernel(ckCompute, null, global, local);
  } catch (err) {
    alert("Failed to enqueue row kernel! " + err);
    return err;
  }

  // Release buffer
  ComputeCommands.enqueueReleaseGLObjects(ComputePBO);
  ComputeCommands.flush();
  
  // Update the texture from the pbo
  gl.bindTexture(gl.TEXTURE_2D, TextureId);

  return WebCL.SUCCESS;
}

(function main() {
  // init window
  if(initialize()==WebCL.SUCCESS) {
    function update() {
      display();
      requestAnimationFrame(update);
    }
    update();
  }
}());

