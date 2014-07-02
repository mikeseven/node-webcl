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
  argv=require('optimist').argv;
  Compute = require(argv.compute || './compute');
  //Compute = require('./compute_droplet2d');
  //Compute = require('./compute_droplet3d');
}

log = console.log;
requestAnimationFrame = document.requestAnimationFrame;

var COMPUTE_KERNEL_ID = "704.cl";
// var COMPUTE_KERNEL_ID = argv.kernel || "mandelbulb_AoS.cl";
// var COMPUTE_KERNEL_ID = "qjulia.cl";
var COMPUTE_KERNEL_NAME = "compute";
var WIDTH = argv.width || 512;
var HEIGHT = argv.height || 512;
var Width = WIDTH;
var Height = HEIGHT;
var Reshaped = true;
var cango=false;
var canvas;

/*
 * reshape() is called if document is resized
 */
function reshape(evt) {
    Width = evt.width;
    Height = evt.height;
    Reshaped = true;
    log('reshape texture to '+Width+'x'+Height);
}

function keydown(evt) {
  //log('process key: ' + evt.which);

  Update = true;
  //cango=true;
}


(function main() {
  log('Initializing');

  document.setTitle("ShaderToy with Image2D");
  canvas = document.createElement("mycanvas", Width, Height);
  
  // install UX callbacks
  document.addEventListener('resize', reshape);
  document.addEventListener('keydown', keydown);
  
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
      log('reshaping texture to '+Width+'x'+Height);
      try {
        var glTexture=gfx.configure_shared_data(Width,Height);
        var clTexture=compute.configure_shared_data(gfx, glTexture);
        Reshaped=false;
      }
      catch(ex) {
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
    
    //gfx.gl().flush(); // for timing
    //if(!cango)
    //  startTime=-1;
    requestAnimationFrame(update /*,cango ? 0 : 5000*/);
  })();
})();

