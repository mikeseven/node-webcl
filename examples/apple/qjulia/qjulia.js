var nodejs = (typeof window === 'undefined');
if(nodejs) {
  cl = require('../../../webcl');
  clu = require('../../../lib/clUtils');
  util = require('util');
  fs = require('fs');
  WebGL = require('node_webgl');
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
  var WIDTH                           = 800;
  var HEIGHT                          = 800;
    
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
      var canvas = document.createElement("fbo-canvas", Width, Height);
      
      // install UX callbacks
      document.addEventListener('resize', this.reshape);

      var err=this.setupGraphics(canvas);
      if(err != cl.SUCCESS)
        return err;
      this.initAntTweakBar(canvas);
      
      err=this.setupComputeDevices(device_type);
      if(err != 0)
        return err;
      
      var image_support = ComputeDeviceId.getInfo(cl.DEVICE_IMAGE_SUPPORT);
      if (!image_support) {
          printf("Unable to query device for image support");
          process.exit(-1);
      }
      if (image_support == 0) {
          log("Application requires images: Images not supported on this device.");
          return cl.IMAGE_FORMAT_NOT_SUPPORTED;
      }
      
      err = this.setupComputeKernel();
      if (err != cl.SUCCESS)
      {
          log("Failed to setup compute kernel! Error " + err);
          return err;
      }
      
      err = this.createComputeResult();
      if(err != cl.SUCCESS)
      {
          log ("Failed to create compute result! Error " + err);
          return err;
      }
      
      return cl.SUCCESS;
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
      gl.viewport(0, 0, Width, Height);
      
      /*pMatrix[0]=pMatrix[5]=pMatrix[15]=1;
      pMatrix[10]=-1;
      
      mvMatrix[0]=mvMatrix[5]=mvMatrix[10]=mvMatrix[15]=1;*/

      
      //gl.enableClientState(gl.VERTEX_ARRAY);
      //gl.enableClientState(gl.TEXTURE_COORD_ARRAY);
      //gl.vertexPointer(2, gl.FLOAT, 0, VertexPos);
      gl.activeTexture(gl.TEXTURE0);
      return cl.SUCCESS;
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
      
      //Pick platform
      var platformList=cl.getPlatforms();
      var platform=platformList[0];

      ComputeDeviceType = device_type ? cl.DEVICE_TYPE_GPU : cl.DEVICE_TYPE_CPU;
      ComputeContext = cl.createContext({
        deviceType: ComputeDeviceType,
        platform: platform,
        shareGroup: gl});

      var device_ids = ComputeContext.getInfo(cl.CONTEXT_DEVICES);
      if(!device_ids)
      {
          alert("Error: Failed to retrieve compute devices for context!");
          return -1;
      }
      
      var device_found=false;
      for(var i in device_ids) 
      {
        device_type=device_ids[i].getInfo(cl.DEVICE_TYPE);
        if(device_type == ComputeDeviceType) 
        {
            ComputeDeviceId = device_ids[i];
            device_found = true;
            break;
        } 
      }
      
      if(!device_found)
      {
          alert("Error: Failed to locate compute device!");
          return -1;
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
      var vendor_name = ComputeDeviceId.getInfo(cl.DEVICE_VENDOR);
      var device_name = ComputeDeviceId.getInfo(cl.DEVICE_NAME);

      log("Connecting to "+vendor_name+" "+device_name);
      
      return cl.SUCCESS;
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
              ComputeProgram.getBuildInfo(ComputeDeviceId, cl.PROGRAM_BUILD_LOG));
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
      MaxWorkGroupSize = ComputeKernel.getWorkGroupInfo(ComputeDeviceId, cl.KERNEL_WORK_GROUP_SIZE);

      log("MaxWorkGroupSize: " + MaxWorkGroupSize);
      log("WorkGroupItems: " + WorkGroupItems);

      WorkGroupSize[0] = (MaxWorkGroupSize > 1) ? (MaxWorkGroupSize / WorkGroupItems) : MaxWorkGroupSize;
      WorkGroupSize[1] = MaxWorkGroupSize / WorkGroupSize[0];
      log("WorkGroupSize: " + WorkGroupSize);
      return cl.SUCCESS;
    },
    createComputeResult: function() {
      log('create compute result');
      ComputeImage = null;

      log("Allocating compute result image in device memory...");
      ComputeImage = ComputeContext.createFromGLTexture2D(cl.MEM_WRITE_ONLY, TextureTarget, 0, TextureId);
      if (!ComputeImage)
      {
        alert("Failed to create OpenGL texture reference! " + err);
        return -1;
      }
      ComputeResult = null;

      log("Allocating compute result buffer in device memory...");
      ComputeResult = ComputeContext.createBuffer(cl.MEM_WRITE_ONLY, TextureTypeSize * 4 * TextureWidth * TextureHeight);
      if (!ComputeResult)
      {
        log("Failed to create OpenCL array!");
        return -1;
      }
      return cl.SUCCESS;
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
        if(newWidth > 1.25 * Width || newHeight > 1.25 * Height ||
            newWidth < Width/1.25 || newHeight < Height/1.25) {
          this.cleanup();
          Width=newWidth;
          Height=newHeight;
          if(this.initialize(ComputeDeviceType == cl.DEVICE_TYPE_GPU) != cl.SUCCESS)
            this.shutdown();
        }
        gl.viewport(0, 0, newWidth, newHeight);
        gl.clear(gl.COLOR_BUFFER_BIT);
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
      
      ATB.Draw();

      gl.finish(); // for timing
      
      //var uiEndTime = new Date().getTime();
      //ReportStats(uiStartTime, uiEndTime);
      //DrawText(TextOffset[0], TextOffset[1], 1, (Animated == 0) ? "Press space to animate" : " ");
            
      return cl.SUCCESS;
    },
    reshape: function (evt)
    {
      newWidth=evt.width;
      newHeight=evt.height;
      //log("reshape to "+w+'x'+h);
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
        //glutPostRedisplay();
    },
    interpolate: function( m, t, a, b )
    {
        for (var i = 0; i < 4; i++ )
            m[ i ] = ( 1 - t ) * a[ i ] + t * b[ i ];
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
            return cl.SUCCESS;
            
        if(Animated || Update)
        {
            Update = false;
            try {
              ComputeKernel.setArg(0, ComputeResult);
              ComputeKernel.setArg(1, MuC, cl.type.FLOAT | cl.type.VEC4);
              ComputeKernel.setArg(2, ColorC, cl.type.FLOAT | cl.type.VEC4);
              ComputeKernel.setArg(3, Epsilon, cl.type.FLOAT);
            } catch (err) {
              alert("Failed to set kernel args! " + err);
              return -10;
            }
        }
        
        var local= WorkGroupSize;
        var global = [ clu.DivUp(TextureWidth, local[0]) * local[0] , 
                       clu.DivUp(TextureHeight, local[1]) * local[1] ];
        
        try {
          ComputeCommands.enqueueNDRangeKernel(ComputeKernel, null, global, local, null, false);
        }
        catch(err)
        {
            alert("Failed to enqueue kernel! " + err);
            return err;
        }

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
          ComputeCommands.enqueueCopyBufferToImage(ComputeResult, ComputeImage, 
                                         0, origin, region, null, false);
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

        return cl.SUCCESS;
    },

    initAntTweakBar: function (canvas) {
      ATB.Init();
      ATB.Define(" GLOBAL help='Quaternion Julia using WebCL.' "); // Message added to the help bar.
      ATB.WindowSize(Width,Height);

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
  if(clgl.initialize(use_gpu)==cl.SUCCESS) {
    function update() {
      clgl.display();
      requestAnimationFrame(update);
    }
    update();
  }

}
