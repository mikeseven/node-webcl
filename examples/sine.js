/*
 * * This file contains proprietary software owned by Motorola Mobility, Inc. ** *
 * No rights, expressed or implied, whatsoever to this software are provided by
 * Motorola Mobility, Inc. hereunder. ** * * (c) Copyright 2011 Motorola
 * Mobility, Inc. All Rights Reserved. **
 */

var cl = require('../webcl'), clu = require('../lib/clUtils'), util =
    require('util'), fs = require('fs'), WebGL = require('webgl'), document =
    WebGL.document(), nodejs = true, log = console.log, requestAnimationFrame =
    document.requestAnimationFrame;

//Read and eval library for mat/vec operations
eval(fs.readFileSync(__dirname + '/glMatrix-0.9.5.min.js', 'utf8'));

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
var clgl;
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

document.createWindow(window_width, window_height);
document.setTitle("sineGL");
requestAnimFrame = document.requestAnimationFrame;
document.on("MOUSEBUTTONDOWN", function(evt) {
  mouse(evt, true);
});
document.on("MOUSEBUTTONUP", function(evt) {
  mouse(evt, false);
});
document.on("MOUSEMOTION", motion);
document.on("VIDEORESIZE",function(evt){
  console.log('resize to: ('+evt.width+", "+evt.height+")");
  document.createWindow(evt.width,evt.height);
  gl.viewportWidth=evt.width;
  gl.viewportHeight=evt.height;
});

main();

function main() {
  log('Init GL');
  initGL();

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
  cxGPUContext = cl.createContext(device, [ cl.GL_CONTEXT_KHR, gl, cl.CONTEXT_PLATFORM, cpPlatform ]);

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
  ckKernel.setArg(0, vbo_cl, cl.type.MEM);
  ckKernel.setArg(1, mesh_width, cl.type.INT | cl.type.UNSIGNED);
  ckKernel.setArg(2, mesh_height, cl.type.INT | cl.type.UNSIGNED);

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
  var canvas = document.getElementById("mycanvas");
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
  clgl.enqueueAcquireGLObjects(cqCommandQueue, vbo_cl);

  // Set arg 3 and execute the kernel
  ckKernel.setArg(3, time, cl.type.FLOAT);
  cqCommandQueue.enqueueNDRangeKernel(ckKernel, null, szGlobalWorkSize, null);

  // unmap buffer object
  clgl.enqueueReleaseGLObjects(cqCommandQueue, vbo_cl);
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
  vbo_cl = clgl.createFromGLBuffer(cxGPUContext, cl.MEM_WRITE_ONLY, vbo);
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
  log('mouse event: button=' + evt.button);
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
  log('mouse motion: dx=' + dx + " dy=" + dy);

  if (mouse_buttons & 2) {
    rotate_x += dy * 0.2;
    rotate_y += dx * 0.2;
  } else if (mouse_buttons & 8) {
    translate_z += dy * 0.01;
  }

  mouse_old_x = evt.x;
  mouse_old_y = evt.y;

  // set view matrix
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
  mat4.identity(mvMatrix);
  mat4.translate(mvMatrix, [ 0.0, 0.0, translate_z ]);
  mat4.rotate(mvMatrix, rotate_x * Math.PI / 180, [ 1.0, 0.0, 0.0 ]);
  mat4.rotate(mvMatrix, rotate_y * Math.PI / 180, [ 0.0, 1.0, 0.0 ]);
}
