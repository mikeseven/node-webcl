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
  log = console.log;
  alert = console.log;

  //Read and eval library for mat/vec operations
  eval(fs.readFileSync(__dirname + '/glMatrix-0.9.5.min.js', 'utf8'));
}

requestAnimationFrame = document.requestAnimationFrame;

var cl = new WebCL();

//First check if the webcl extension is installed at all
if (cl == undefined) {
  alert("Unfortunately your system does not support WebCL. "
      + "Make sure that you have the WebCL extension installed.");
  return;
}

//Rendering window vars
var window_width = 512;
var window_height = 512;
var mesh_width = 512;
var mesh_height = 512;

//OpenCL vars
var cpPlatform;
var cxGPUContext;
var cdDevices;
var cqCommandQueue;
var ckKernel;
var vbo_cl;
var cpProgram;
var szGlobalWorkSize = [ mesh_width, mesh_height ];

//vbo variables
var vbo;
var gl;
var mvMatrix = mat4.create();
var pMatrix = mat4.create();
var shaderProgram;

//mouse controls
var mouse_old_x, mouse_old_y;
var mouse_buttons = 0;
var rotate_x = 0.0, rotate_y = 0.0;
var translate_z = -3.0;

//Sim and Auto-Verification parameters
var anim = 0.0;
var iFrameCount = 0; // FPS count for averaging
var iFrameTrigger = 90; // FPS trigger for sampling
var iFramesPerSec = 0; // frames per second
var iTestSets = 3;
var g_Index = 0;

document.setTitle("sineGL");
requestAnimFrame = document.requestAnimationFrame;
document.on("mousedown", function(evt) {
  mouse(evt, true);
});
document.on("mouseup", function(evt) {
  mouse(evt, false);
});
document.on("mousemove", motion);
document.on("resize",function(evt){
  console.log('resize to: ('+evt.width+", "+evt.height+")");
  document.createWindow(evt.width,evt.height);
  gl.viewportWidth=evt.width;
  gl.viewportHeight=evt.height;
  gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
});

main();

function main() {
  log('Init GL');
  initGL();

  // Pick platform
  var platformList = cl.getPlatforms();
  cpPlatform = platformList[0];

  // Query the set of GPU devices on this platform
  cdDevices = cpPlatform.getDevices(cl.DEVICE_TYPE_DEFAULT);
  log("  # of Devices Available = " + cdDevices.length);
  var device = cdDevices[0];
  log("  Using Device 0: " + device.getInfo(cl.DEVICE_NAME));

  // get CL-GL extension
  var extensions = device.getInfo(cl.DEVICE_EXTENSIONS);
  var hasGLSupport = extensions.search(/gl.sharing/i) >= 0;
  log(hasGLSupport ? "GL-CL extension available ;-)" : "No GL support");
  if (!hasGLSupport)
    return;

  // create the OpenCL context
  cxGPUContext = cl.createContext({
    devices: device, 
    shareGroup: gl, 
    platform: cpPlatform });

  // create a command-queue
  cqCommandQueue = cxGPUContext.createCommandQueue(device, 0);

  // create the program
  var sourceCL = fs.readFileSync(__dirname + '/sine.cl', 'ascii');
  cpProgram = cxGPUContext.createProgram(sourceCL);

  // build the program
  try {
    cpProgram.build(device, "-cl-fast-relaxed-math");
  } catch (err) {
    log('Error building program: ' + err);
  }
  log("Build Status: "
      + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_STATUS));
  log("Build Options: "
      + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_OPTIONS));
  log("Build Log: " + cpProgram.getBuildInfo(device, cl.PROGRAM_BUILD_LOG));

  // create the kernel
  try {
    ckKernel = cpProgram.createKernel("sine_wave");
  } catch (err) {
    log("Cannot create Kernel: " + err);
    return;
  }

  // create VBO (if using standard GL or CL-GL interop)
  createVBO();

  // set the args values
  ckKernel.setArg(0, vbo_cl);
  ckKernel.setArg(1, mesh_width, cl.type.UINT);
  ckKernel.setArg(2, mesh_height, cl.type.UINT);

  // run OpenCL kernel once to generate vertex positions
  runKernel(0);

  // start main rendering loop
  DisplayGL();
}

function getShader(gl, id) {
  var shaders =
      {
        "shader-fs" : [ 
            "varying vec4 vColor;", 
            "void main(void) {",
            "    gl_FragColor = vColor;", 
            "}" ].join("\n"),

        "shader-vs" : [
            "attribute vec3 aVertexPosition;",
            "attribute vec4 aVertexColor;",
            "uniform mat4 uMVMatrix;",
            "uniform mat4 uPMatrix;",
            "varying vec4 vColor;",
            "void main(void) {",
            "    gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);",
            "    vColor = vec4(1,0,0,1);//aVertexColor;", "}" ].join("\n")
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

  shaderProgram.vertexColorAttribute =
      gl.getAttribLocation(shaderProgram, "aVertexColor");
  gl.enableVertexAttribArray(shaderProgram.vertexColorAttribute);

  shaderProgram.pMatrixUniform =
      gl.getUniformLocation(shaderProgram, "uPMatrix");
  shaderProgram.mvMatrixUniform =
      gl.getUniformLocation(shaderProgram, "uMVMatrix");
}

function initGL() {
  var canvas = document.createElement("mycanvas", window_width, window_height);
  try {
    gl = canvas.getContext("experimental-webgl");
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;
  } catch (e) {
  }
  if (!gl) {
    alert("Could not initialise WebGL, sorry :-(");
  }

  // init shaders
  initShaders();

  // default initialization
  gl.clearColor(0.0, 0.0, 0.0, 1.0);
  gl.disable(gl.DEPTH_TEST);

  // viewport
  gl.viewport(0, 0, window_width, window_height);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  // projection
  mat4.perspective(60, window_width / window_height, 0.1, 10, pMatrix);

  // set view matrix
  mat4.identity(mvMatrix);
  mat4.translate(mvMatrix, [ 0.0, 0.0, translate_z ]);
  mat4.rotate(mvMatrix, rotate_x * Math.PI / 180, [ 1.0, 0.0, 0.0 ]);
  mat4.rotate(mvMatrix, rotate_y * Math.PI / 180, [ 0.0, 1.0, 0.0 ]);
}

function runKernel(time) {
  // map OpenGL buffer object for writing from OpenCL
  gl.finish();
  cqCommandQueue.enqueueAcquireGLObjects(vbo_cl);

  // Set arg 3 and execute the kernel
  ckKernel.setArg(3, time, cl.type.FLOAT);
  cqCommandQueue.enqueueNDRangeKernel(ckKernel, null, szGlobalWorkSize, null);

  // unmap buffer object
  cqCommandQueue.enqueueReleaseGLObjects(vbo_cl);
  cqCommandQueue.finish();
}

function createVBO() {
  // create VBO
  var size = mesh_width * mesh_height * 4 * 4;

  // create buffer object
  vbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);

  // initialize buffer object
  gl.bufferData(gl.ARRAY_BUFFER, size, gl.DYNAMIC_DRAW);

  // create OpenCL buffer from GL VBO
  vbo_cl = cxGPUContext.createFromGLBuffer(cl.MEM_WRITE_ONLY, vbo);
}

function setMatrixUniforms() {
  gl.uniformMatrix4fv(shaderProgram.pMatrixUniform, false, pMatrix);
  gl.uniformMatrix4fv(shaderProgram.mvMatrixUniform, false, mvMatrix);
}

function DisplayGL(time) {
  // increment the geometry computation parameter (or set to reference for Q/A check)
  anim += 0.01;

  // run OpenCL kernel to generate vertex positions
  runKernel(anim);

  // clear graphics then render from the vbo
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, 4, gl.FLOAT,
      false, 0, 0);

  setMatrixUniforms();
  gl.drawArrays(gl.POINTS, 0, mesh_width * mesh_height);

  requestAnimationFrame(DisplayGL);
}

function mouse(evt, isDown) {
  //log('mouse event: button=' + evt.button);
  if (isDown)
    mouse_buttons |= 1 << evt.button;
  else
    mouse_buttons = 0;

  mouse_old_x = evt.x;
  mouse_old_y = evt.y;
}

function motion(evt) {
  var dx = (evt.x - mouse_old_x);
  var dy = (evt.y - mouse_old_y);
  //log('mouse motion: dx=' + dx + " dy=" + dy+ "button="+mouse_buttons);

  if (mouse_buttons & 1) {
    rotate_x += dy * 0.2;
    rotate_y += dx * 0.2;
  } else if (mouse_buttons & 2) {
    translate_z += dy * 0.01;
  }

  mouse_old_x = evt.x;
  mouse_old_y = evt.y;

  // set view matrix
  //gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  mat4.identity(mvMatrix);
  mat4.translate(mvMatrix, [ 0.0, 0.0, translate_z ]);
  mat4.rotate(mvMatrix, rotate_x * Math.PI / 180, [ 1.0, 0.0, 0.0 ]);
  mat4.rotate(mvMatrix, rotate_y * Math.PI / 180, [ 0.0, 1.0, 0.0 ]);
}
