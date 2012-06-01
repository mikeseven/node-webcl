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
  WebCL = require('../../webcl');
  clu = require('../../lib/clUtils');
  util = require('util');
  fs = require('fs');
  WebGL = require('node-webgl');
  document = WebGL.document();
  Image = WebGL.Image;
  alert = console.log;
  Graphics = require('./graphics');
  Compute = require('./compute');
  //Compute = require('./compute_julia');
}

log = console.log;
requestAnimationFrame = document.requestAnimationFrame;

//var COMPUTE_KERNEL_ID = "704.cl";
var COMPUTE_KERNEL_ID = "mandelbulb2.cl";
//var COMPUTE_KERNEL_ID = "qjulia.cl";
var COMPUTE_KERNEL_NAME = "compute";
var WIDTH = 512;
var HEIGHT = 512;
var Width = WIDTH;
var Height = HEIGHT;
var Reshaped = true;

/*
 * reshape() is called if document is resized
 */
function reshape(evt) {
  Width = evt.width;
  Height = evt.height;
  Reshaped = true;
}

function keydown(evt) {
  //log('process key: ' + evt.which);

  //Update = true;
}


(function main() {
  log('Initializing');

  document.setTitle("ShaderToy with Image2D");
  var canvas = document.createElement("mycanvas", Width, Height);
  
  // install UX callbacks
  document.addEventListener('resize', reshape);
  //document.addEventListener('keydown', keydown);
  
  // init WebGL
  var gfx=Graphics();
  try {
    gfx.init(canvas);
  }
  catch(err) {
    log('[Error] While initializing GL: '+err);
    gfx.clean();
    return;
  }
  
  // init WebCL
  var compute=Compute();
  try {
    compute.init(gfx, COMPUTE_KERNEL_ID, COMPUTE_KERNEL_NAME);
  }
  catch(err) {
    log('[Error] While initializing CL: '+err);
    compute.clean();
    return;
  }
  
  // render scene
  var startTime=-1;
  
  (function update(timestamp) {
    if(timestamp) {
      if(startTime==-1) {
        startTime=fpsTo=timestamp;
      }
      var ltime = timestamp-startTime;
    }
   
    // reinit shared data if document is resized
    if (Reshaped) {
      log('reshaping texture');
      try {
        var glTexture=gfx.configure_shared_data(Width,Height);
        var clTexture=compute.configure_shared_data(gfx, glTexture);
        Reshaped=false;
      }
      catch(err) {
        log('[Error] While reshaping shared data: '+ex);
        return;
      }
    }
   
    // set kernel arguments
    compute.resetKernelArgs(ltime/1000.0, Width, Height);
    
    // compute texture for this timestamp
    try {
      compute.execute_kernel(gfx.gl());
    }
    catch(err) {
      alert('[Error] While executing kernel: '+err);
      return;
    }
    
    // render scene with updated texture from CL
    try {
      gfx.display(ltime);
    }
    catch(err) {
      alert('[Error] While rendering scene '+err);
      return;
    }
    
    gfx.gl().finish(); // for timing
    requestAnimationFrame(update,0);
  })();
})();

function initAntTweakBar(canvas) {
  if(!nodejs) return;
  
  ATB.Init();
  ATB.Define(" GLOBAL help='WebGL interop with WebCL' "); // Message added to the help bar.
  ATB.WindowSize(canvas.width, canvas.height);


  var twBar=new ATB.NewBar("clgl");
  
  /*var scenes=['mandelbulb','704'];
  var sceneTypes=ATB.DefineEnum('Scene Types',scenes, 2);
  log('[JS] returned type: '+sceneTypes);
  twBar.AddVar("scenes", sceneTypes, scenes,null);*/
  
  twBar.AddVar("epsilon", ATB.TYPE_FLOAT, {
    getter: function(data){ return Epsilon; },
    setter: function(val,data) { Epsilon=val; },
  },
  " label='epsilon' min=0.001 max=0.05 step=0.001 keyIncr=s keyDecr=S help='epsilon' ");

  twBar.AddVar("MuC", ATB.TYPE_QUAT4F, {
    getter: function(data){ return MuC; },
    //setter: function(val,data) { MuC=val; },
  },
  " label='Mu' opened=true help='Mu' ");

  twBar.AddVar("Color", ATB.TYPE_COLOR4F, {
    getter: function(data){ return ColorC; },
    //setter: function(val,data) { MuC=val; },
  },
  " label='Color' opened=true help='Color' ");

  twBar.AddVar("fps", ATB.TYPE_FLOAT, {
    getter: function(data){ return ffps; },
  },
  " label='fps' precision=0 help='frames-per-second.'");

  ATB.Define(" clgl valueswidth=fit "); // column width fits content 
}

/*
 * Before calling AntTweakBar or any other library that could use programs, one
 * must make sure to disable the VertexAttribArray used by the current program
 * otherwise this may have some unpredictable consequences aka wrong vertex
 * attrib arrays being used by another program!
 */
function drawATB(gl) {
  if(!nodejs) return;
  
  gl.disableVertexAttribArray(shaderProgram.vertexPositionAttribute);
  gl.disableVertexAttribArray(shaderProgram.textureCoordAttribute);
  gl.useProgram(null);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);
  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);

  ATB.Draw();

  gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
  gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);
  gl.useProgram(shaderProgram);
}
