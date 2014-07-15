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
  WebCL = require('../../../webcl');
  clu = require('../../../lib/clUtils');
  util = require('util');
  fs = require('fs');
  WebGL = require('node-webgl');
  document = WebGL.document();
  log = console.log;
  alert = console.log;
  ATB=document.AntTweakBar;
  Image = WebGL.Image;
}

requestAnimationFrame = document.requestAnimationFrame;
var use_gpu=true;

main();

function CLGL() {
  var COMPUTE_KERNEL_FILENAME         = "qjulia_kernel.cl";
  var COMPUTE_KERNEL_METHOD_NAME      = "QJuliaKernel";
  var WIDTH                           = 512;
  var HEIGHT                          = 512;
    
  var /* cl_context */                       ComputeContext;
  var /* cl_command_queue */                 ComputeCommands;
  var /* cl_kernel */                        ComputeKernel;
  var /* cl_program */                       ComputeProgram;
  var /* cl_device_id */                     ComputeDeviceId;
  var /* cl_device_type */                   ComputeDeviceType;
  var /* cl_mem */                           ComputeResult;
  var /* cl_mem */                           ComputeImage;
  var /* size_t */                           MaxWorkGroupSize;
  var /* int */                              WorkGroupSize=[0,0];
  var /* int */                              WorkGroupItems = 32;
  
  var Width                   = WIDTH;
  var Height                  = HEIGHT;
  var Animated                = true;
  var Update                  = true;
  var Reshaped                = false;
  var newWidth, newHeight; // only when reshape
  var Epsilon                 = 0.003;
  var colorT                  = 0;
  var ColorA                  = [ 0.25, 0.45, 1, 1 ];
  var ColorB                  = [ 0.25, 0.45, 1, 1 ];
  var ColorC                  = [ 0.25, 0.45, 1, 1 ];
  var t                       = 0;
  var MuA                     = [ -0.278, -0.479, 0, 0 ];
  var MuB                     = [ 0.278, 0.479, 0, 0 ];
  var MuC                     = [ -0.278, -0.479, -0.231, 0.235 ];

  var gl;
  var shaderProgram;
  var TextureId                   = null;
  var TextureTarget;
  var TextureInternal;
  var TextureFormat;
  var TextureType;
  var TextureWidth                = WIDTH;
  var TextureHeight               = HEIGHT;
  var TextureTypeSize             = 1; // sizeof(char);
  var ActiveTextureUnit;
  var HostImageBuffer             = 0;
  var twBar;
  var canvas;

  var VertexPos = [ 
     -1, -1,
      1, -1,
      1,  1,
     -1,  1 ];
  var TexCoords = [
       0, 0,
       1, 0,
       1, 1,
       0, 1 ];

  var framework={
    initialize: function(device_type) {
      log('Initializing');
      document.setTitle("fbo");
      canvas = document.createElement("fbo-canvas", Width, Height);
      
      // install UX callbacks
      document.addEventListener('resize', this.reshape);

      var err=this.setupGraphics(canvas);
      if(err != WebCL.SUCCESS)
        return err;
      this.initAntTweakBar(canvas);
      
      err=this.setupComputeDevices(device_type);
      if(err != 0)
        return err;
      
      var image_support = ComputeDeviceId.getInfo(WebCL.DEVICE_IMAGE_SUPPORT);
      if (!image_support) {
          printf("Unable to query device for image support");
          process.exit(-1);
      }
      if (image_support == 0) {
          log("Application requires images: Images not supported on this device.");
          return WebCL.IMAGE_FORMAT_NOT_SUPPORTED;
      }
      
      err = this.setupComputeKernel();
      if (err != WebCL.SUCCESS)
      {
          log("Failed to setup compute kernel! Error " + err);
          return err;
      }
      
      err = this.createComputeResult();
      if(err != WebCL.SUCCESS)
      {
          log ("Failed to create compute result! Error " + err);
          return err;
      }
      
      return WebCL.SUCCESS;
    },
    
    // /////////////////////////////////////////////////////////////////////
    // OpenGL stuff
    // /////////////////////////////////////////////////////////////////////
    
    createTexture: function (width, height) {
        log('  create texture');
        
        if(TextureId)
            gl.deleteTexture(TextureId);
        TextureId = null;
        
        log("Creating Texture "+width+" x "+height+"...");

        TextureWidth = width;
        TextureHeight = height;
        TextureTarget               = gl.TEXTURE_2D;
        TextureInternal             = gl.RGBA;
        TextureFormat               = gl.RGBA;
        TextureType                 = gl.UNSIGNED_BYTE;
        ActiveTextureUnit           = gl.TEXTURE0;

        gl.activeTexture(ActiveTextureUnit);
        TextureId=gl.createTexture();
        gl.bindTexture(TextureTarget, TextureId);
        gl.texParameteri(TextureTarget, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(TextureTarget, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texImage2D(TextureTarget, 0, TextureInternal, TextureWidth, TextureHeight, 0, 
                     TextureFormat, TextureType, null);
        gl.bindTexture(TextureTarget, null);
    },
    createBuffers: function() {
      log('  create buffers');
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
    },
    getShader: function (gl, id) {
      var shaders= {
          "shader-fs" : 
            [     
             "#ifdef GL_ES",
             "  precision mediump float;",
             "#endif",
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
                "varying vec2 vTextureCoord;",
                "void main(void) {",
                "    gl_Position = vec4(aVertexPosition, 1.0);",
                "    vTextureCoord = aTextureCoord;",
                "}"
                ].join("\n")
      };

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
    },
    createShaders: function() {
      log('  create shaders');
      var fragmentShader = this.getShader(gl, "shader-fs");
      var vertexShader = this.getShader(gl, "shader-vs");

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

      shaderProgram.samplerUniform = gl.getUniformLocation(shaderProgram, "uSampler");
    },
    setupGraphics:function(canvas) {
      log('Setup graphics');
      try {
        gl = canvas.getContext("experimental-webgl");
        gl.viewportWidth = canvas.width;
        gl.viewportHeight = canvas.height;
      } catch (e) {
      }
      if (!gl) {
        alert("Could not initialise WebGL, sorry :-(" );
        return -1;
      }
      
      this.createBuffers();
      this.createShaders();
      this.createTexture(Width, Height);
      
      gl.clearColor (0, 0, 0, 0);

      gl.disable(gl.DEPTH_TEST);
      gl.activeTexture(gl.TEXTURE0);
      gl.viewport(0, 0, canvas.width,canvas.height);
      
      gl.activeTexture(gl.TEXTURE0);
      return WebCL.SUCCESS;
    },
    renderTexture: function( pvData )
    {
      // we just draw a screen-aligned texture
      //gl.viewport( 0, 0, Width, Height );

      gl.enable( TextureTarget );
      gl.bindTexture( TextureTarget, TextureId );

      if(pvData)
          gl.texSubImage2D(TextureTarget, 0, 0, 0, TextureWidth, TextureHeight, 
                          TextureFormat, TextureType, pvData);

      // draw screen aligned quad
      gl.bindBuffer(gl.ARRAY_BUFFER, VertexPosBuffer);
      gl.vertexAttribPointer(shaderProgram.vertexPositionAttribute, VertexPosBuffer.itemSize, gl.FLOAT, false, 0, 0);

      gl.bindBuffer(gl.ARRAY_BUFFER, TexCoordsBuffer);
      gl.vertexAttribPointer(shaderProgram.textureCoordAttribute, TexCoordsBuffer.itemSize, gl.FLOAT, false, 0, 0);
      
      gl.activeTexture(gl.TEXTURE0);
      gl.uniform1i(shaderProgram.samplerUniform, 0);

      gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);

      gl.bindTexture( TextureTarget, null );
      gl.disable( TextureTarget );
    },

    // /////////////////////////////////////////////////////////////////////
    // OpenCL stuff
    // /////////////////////////////////////////////////////////////////////
    
    setupComputeDevices:function(device_type) {
      log('setup compute devices');
      
      // Pick platform
      var platformList = WebCL.getPlatforms();
      var platform = platformList[0];
      var devices = platform.getDevices(ComputeDeviceType ? WebCL.DEVICE_TYPE_GPU : WebCL.DEVICE_TYPE_DEFAULT);
      ComputeDeviceId=devices[0];

      // make sure we use a discrete GPU
      for(var i=0;i<devices.length;i++) {
        var vendor=devices[i].getInfo(WebCL.DEVICE_VENDOR);
        // log('found vendor '+vendor+', is Intel? '+(vendor.indexOf('Intel')>=0))
        if(vendor.indexOf('Intel')==-1)
          ComputeDeviceId=devices[i];
      }
      log('found '+devices.length+' devices, using device: '+ComputeDeviceId.getInfo(WebCL.DEVICE_NAME));

      if(!ComputeDeviceId.enableExtension('KHR_gl_sharing'))
        throw new Error("Can NOT use GL sharing");

      // create the OpenCL context
      try {
        ComputeContext = WebCL.createContext(gl, ComputeDeviceId);
      }
      catch(err) {
        throw "Error: Failed to create context! "+err;
      }

      // Create a command queue
      //
      ComputeCommands = ComputeContext.createCommandQueue(ComputeDeviceId, 0);
      if (!ComputeCommands)
      {
          alert("Error: Failed to create a command queue!");
          return -1;
      }

      // Report the device vendor and device name
      // 
      var vendor_name = ComputeDeviceId.getInfo(WebCL.DEVICE_VENDOR);
      var device_name = ComputeDeviceId.getInfo(WebCL.DEVICE_NAME);

      log("Connecting to "+vendor_name+" "+device_name);
      
      return WebCL.SUCCESS;
    },
    setupComputeKernel:function() {
      log('setup compute kernel');

      ComputeKernel = null;
      ComputeProgram = null;
      
      log("Loading kernel source from file '"+COMPUTE_KERNEL_FILENAME+"'...");
      source = fs.readFileSync(__dirname + '/' + COMPUTE_KERNEL_FILENAME, 'ascii');
      if (!source)
      {
          alert("Error: Failed to load kernel source!");
          return -1;
      }

      var width_macro = "#define WIDTH";
      var height_macro = "#define HEIGHT";
      
      source = width_macro+" "+Width+'\n'+
                height_macro+" "+Height+'\n'+
                source;

      // Create the compute program from the source buffer
      //
      ComputeProgram = ComputeContext.createProgram(source);
      if (!ComputeProgram)
      {
          alert("Error: Failed to create compute program!");
          return -1;
      }

      // Build the program executable
      //
      try {
        ComputeProgram.build(null, "-cl-fast-relaxed-math");
      } catch (err) {
        log('Error building program: ' + err);
          alert("Error: Failed to build program executable!\n"+
              ComputeProgram.getBuildInfo(ComputeDeviceId, WebCL.PROGRAM_BUILD_LOG));
          return -1;
      }

      // Create the compute kernel from within the program
      //
      log("Creating kernel '"+COMPUTE_KERNEL_METHOD_NAME+"'...");
      ComputeKernel = ComputeProgram.createKernel(COMPUTE_KERNEL_METHOD_NAME);
      if (!ComputeKernel)
      {
          alert("Error: Failed to create compute kernel!");
          return -1;
      }

      // Get the maximum work group size for executing the kernel on the device
      //
      MaxWorkGroupSize = ComputeKernel.getWorkGroupInfo(ComputeDeviceId, WebCL.KERNEL_WORK_GROUP_SIZE);

      log("MaxWorkGroupSize: " + MaxWorkGroupSize);
      log("WorkGroupItems: " + WorkGroupItems);

      WorkGroupSize[0] = (MaxWorkGroupSize > 1) ? (MaxWorkGroupSize / WorkGroupItems) : MaxWorkGroupSize;
      WorkGroupSize[1] = MaxWorkGroupSize / WorkGroupSize[0];
      log("WorkGroupSize: " + WorkGroupSize);
      return WebCL.SUCCESS;
    },
    createComputeResult: function() {
      log('create compute result');
      ComputeImage = null;

      log("Allocating compute result image in device memory...");
      ComputeImage = ComputeContext.createFromGLTexture(WebCL.MEM_WRITE_ONLY, TextureTarget, 0, TextureId);
      if (!ComputeImage)
      {
        alert("Failed to create OpenGL texture reference! " + err);
        return -1;
      }
      ComputeResult = null;

      log("Allocating compute result buffer in device memory...");
      ComputeResult = ComputeContext.createBuffer(WebCL.MEM_WRITE_ONLY, TextureTypeSize * 4 * TextureWidth * TextureHeight);
      if (!ComputeResult)
      {
        log("Failed to create OpenCL array!");
        return -1;
      }
      return WebCL.SUCCESS;
    },
    cleanup: function()
    {
      ComputeCommands.finish();
      ComputeKernel=null;
      ComputeProgram=null;
      ComputeCommands=null;
      ComputeResult=null;
      ComputeImage=null;
      ComputeContext=null;
    },
    shutdown: function()
    {
      log("Shutting down...");
      this.cleanup();
      process.exit(0);
    },
    
	/* Before calling AntTweakBar or any other library that could use programs,
	 * one must make sure to disable the VertexAttribArray used by the current
	 * program otherwise this may have some unpredictable consequences aka
	 * wrong vertex attrib arrays being used by another program!
	 */
	drawATB: function() {
	  gl.disableVertexAttribArray(shaderProgram.vertexPositionAttribute);
	  gl.disableVertexAttribArray(shaderProgram.textureCoordAttribute);
	  gl.useProgram(null);
	  gl.bindBuffer(gl.ARRAY_BUFFER, null);
	  gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, null);

	  ATB.Draw();

	  gl.useProgram(shaderProgram);
    gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);
    gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);
	},

    // /////////////////////////////////////////////////////////////////////
    // rendering loop
    // /////////////////////////////////////////////////////////////////////
    
    display: function(timestamp) {
      //FrameCount++;
      //var uiStartTime = new Date().getTime();
      
      gl.clearColor (0, 0, 0, 0);
      gl.clear (gl.COLOR_BUFFER_BIT);
   
      if(Reshaped) {
        Reshaped=false;
        Width=newWidth;
        Height=newHeight;
        gl.viewport(0, 0, Width, Height);
        gl.clear(gl.COLOR_BUFFER_BIT);
      
        // make sure AntTweakBar is repositioned correctly and events correct
        ATB.WindowSize(Width,Height);
      }
      
      if(Animated)
      {
        t = this.updateMu( t, MuA, MuB );
        this.interpolate( MuC, t, MuA, MuB );
    
        colorT = this.updateColor( colorT, ColorA, ColorB );
        this.interpolate(ColorC, colorT, ColorA, ColorB );
      }
      
      var err = this.recompute();
      if (err != 0)
      {
          alert("Error "+err+" from Recompute!");
          process.exit(1);
      }

      this.renderTexture(HostImageBuffer);
      //this.reportInfo();
      
      this.drawATB();

      gl.finish(); // for timing
      
      //var uiEndTime = new Date().getTime();
      //ReportStats(uiStartTime, uiEndTime);
      //DrawText(TextOffset[0], TextOffset[1], 1, (Animated == 0) ? "Press space to animate" : " ");
            
      return WebCL.SUCCESS;
    },
    reshape: function (evt)
    {
      newWidth=evt.width;
      newHeight=evt.height;
      log("reshape to "+newWidth+'x'+newHeight);
      Reshaped=true;
    },

    keyboard: function( key, x, y )
    {
      // TODO
       var fStepSize = 0.05;

       switch( key )
       {
          case 27:
             exit(0);
             break;

          case ' ':
             Animated = !Animated;
             sprintf(InfoString, "Animated = %s\n", Animated ? "true" : "false");
             ShowInfo = 1;
             break;

          case 'i':
             ShowInfo = ShowInfo > 0 ? false : true;
             break;
          
          case 's':
             ShowStats = ShowStats > 0 ? false : true;
             break;
             
          case '+':
          case '=':
             if(Epsilon >= 0.002)
                 Epsilon *= (1.0 / 1.05);
             sprintf(InfoString, "Epsilon = %f\n", Epsilon);
             ShowInfo = true;
             break;

          case '-':
             if(Epsilon < 0.01)
                 Epsilon *= 1.05;
             sprintf(InfoString, "Epsilon = %f\n", Epsilon);
             ShowInfo = true;
             break;

          case 'w':
             MuC[0] += fStepSize; 
             break;

          case 'x':
             MuC[0] -= fStepSize; 
             break;

          case 'q':
             MuC[1] += fStepSize; 
             break;

          case 'z':
             MuC[1] -= fStepSize; 
             break;

          case 'a':
             MuC[2] += fStepSize; 
             break;

          case 'd':
             MuC[2] -= fStepSize; 
             break;

          case 'e':
             MuC[3] += fStepSize; 
             break;

          case 'c':
             MuC[3] -= fStepSize; 
             break;

          case 'f':
             glutFullScreen(); 
             break;

        }
        Update = true;
    },
    interpolate: function( m, t, a, b )
    {
        m[ 0 ] = ( 1 - t ) * a[ 0 ] + t * b[ 0 ];
        m[ 1 ] = ( 1 - t ) * a[ 1 ] + t * b[ 1 ];
        m[ 2 ] = ( 1 - t ) * a[ 2 ] + t * b[ 2 ];
        m[ 3 ] = ( 1 - t ) * a[ 3 ] + t * b[ 3 ];
    },
    updateMu: function( t, a, b )
    {
        t += 0.01;

        if ( t >= 1 )
        {
            t = 0;

            a[ 0 ] = b[ 0 ];
            a[ 1 ] = b[ 1 ];
            a[ 2 ] = b[ 2 ];
            a[ 3 ] = b[ 3 ];

            b[ 0 ] = 2 * Math.random() - 1;
            b[ 1 ] = 2 * Math.random() - 1;
            b[ 2 ] = 2 * Math.random() - 1;
            b[ 3 ] = 2 * Math.random() - 1;
        }
        return t;
    },
    randomColor: function( v )
    {
        v[ 0 ] = 2 * Math.random() - 1;
        v[ 1 ] = 2 * Math.random() - 1;
        v[ 2 ] = 2 * Math.random() - 1;
        v[ 3 ] = 1;
    },
    updateColor: function( t, a, b )
    {
        t += 0.01;
       
        if ( t >= 1 )
        {
            t = 0;

            a[ 0 ] = b[ 0 ];
            a[ 1 ] = b[ 1 ];
            a[ 2 ] = b[ 2 ];
            a[ 3 ] = b[ 3 ];

            this.randomColor(b);
        }
        return t;
    },
    recompute: function()
    {
        if(!ComputeKernel || !ComputeResult)
            return WebCL.SUCCESS;
            
        if(Animated || Update)
        {
            Update = false;
            try {
              ComputeKernel.setArg(0, ComputeResult);
              ComputeKernel.setArg(1, MuC, WebCL.type.FLOAT | WebCL.type.VEC4);
              ComputeKernel.setArg(2, ColorC, WebCL.type.FLOAT | WebCL.type.VEC4);
              ComputeKernel.setArg(3, Epsilon, WebCL.type.FLOAT);
            } catch (err) {
              alert("Failed to set kernel args! " + err);
              return -10;
            }
        }
        
        var local= WorkGroupSize;
        var global = [ clu.DivUp(TextureWidth, local[0]) * local[0] , 
                       clu.DivUp(TextureHeight, local[1]) * local[1] ];
        
        try {
          ComputeCommands.enqueueNDRangeKernel(ComputeKernel, null, global, local);
        }
        catch(err)
        {
            alert("Failed to enqueue kernel! " + err);
            return err;
        }
        
        // sync GL
        gl.finish();
        try {
          ComputeCommands.enqueueAcquireGLObjects(ComputeImage);
        }
        catch(err)
        {
            alert("Failed to acquire GL object! " + err);
            return -1;
        }

        var origin = [ 0, 0, 0];
        var region = [ TextureWidth, TextureHeight, 1];
        
        try {
          // TODO this should be shared between CL and GL not copied as in Apple code
          ComputeCommands.enqueueCopyBufferToImage(ComputeResult, ComputeImage, 
                                         0, origin, region);
        }
        catch(err)
        {
            alert("Failed to copy buffer to image! " + err);
            return -1;
        }
        
        try {
          ComputeCommands.enqueueReleaseGLObjects(ComputeImage);
        }
        catch (err)
        {
            alert("Failed to release GL object! " + err);
            return -1;
        }

        // sync CL
        ComputeCommands.finish();
        return WebCL.SUCCESS;
    },

    initAntTweakBar: function (canvas) {
      ATB.Init();
      ATB.Define(" GLOBAL help='Quaternion Julia using WebCL.' "); // Message added to the help bar.
      ATB.WindowSize(canvas.width,canvas.height);

      twBar=new ATB.NewBar("qjulia");
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

      ATB.Define(" qjulia valueswidth=fit"); // column width fits content 
    }

  };


  return framework;
}

function main() {
  // init window
  clgl=new CLGL();
  if(clgl.initialize(use_gpu)==WebCL.SUCCESS) {
    function update() {
      clgl.display();
      requestAnimationFrame(update);
    }
    update();
  }

}
