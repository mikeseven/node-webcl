var cl = require('../webcl'), 
    clu = require('../lib/clUtils'), 
    util = require('util'), 
    fs = require('fs'), 
    WebGL = require('node_webgl'), 
    document = WebGL.document(), 
    Image = WebGL.Image, 
    log = console.log, 
    requestAnimationFrame = document.requestAnimationFrame, 
    alert = console.log;
var use_gpu = true;
var nodejs = true;
var image;

var COMPUTE_KERNEL_FILENAME = "BoxFilter.cl";
var WIDTH = 800;
var HEIGHT = 800;

// cl stuff
var /* cl_context */ComputeContext;
var /* cl_command_queue */ComputeCommands;
var /* cl_kernel */ComputeKernel;
var /* cl_program */ComputeProgram;
var /* cl_device_id */ComputeDeviceId;
var /* cl_device_type */ComputeDeviceType;
var /* cl_mem */ComputeResult;
var /* cl_mem */ComputeImage;
var MaxWorkGroupSize;
var WorkGroupSize = [ 0, 0 ];
var WorkGroupItems = 32;

var Width = WIDTH;
var Height = HEIGHT;
var Reshaped = false;
var newWidth, newHeight; // only when reshape

// simulation
var uiNumOutputPix = WorkGroupItems; // Default output pix per workgroup... may be modified depending HW/OpenCl caps
var iRadius = 10; // initial radius of 2D box filter mask
var fScale = 1.0 / (2 * iRadius + 1.0); // precalculated GV rescaling value
var iRadiusAligned = false;
var pbo;
var RowSampler;
var ComputeBufTemp, ComputeBufOut, ComputePBO;
var ckBoxRowsTex, ckBoxColumns;

// gl stuff
var gl;
var shaderProgram;
var TextureId = null;
var TextureTarget;
var TextureInternal;
var TextureFormat;
var TextureType;
var TextureWidth = WIDTH;
var TextureHeight = HEIGHT;
var TextureTypeSize = 1; // sizeof(char);
var ActiveTextureUnit;

function initialize(device_type) {
  log('Initializing');
  document.setTitle("OpenCL GPU BoxFilter Demo");
  var canvas = document.createElement("fbo-canvas", image.width, image.height);

  // install UX callbacks
  document.addEventListener('resize', reshape);
  document.addEventListener('keydown', keydown);

  var err = setupGraphics(canvas);
  if (err != cl.SUCCESS)
    return err;

  err = setupComputeDevices(device_type);
  if (err != 0)
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

  err = createComputeResult();
  if (err != cl.SUCCESS) {
    log("Failed to create compute result! Error " + err);
    return err;
  }

  err = setupComputeKernel();
  if (err != cl.SUCCESS) {
    log("Failed to setup compute kernel! Error " + err);
    return err;
  }

  // Warmup call to assure OpenCL driver is awake
  BoxFilterGPU(image, ComputePBO, iRadius, fScale);
  ComputeCommands.finish();

  return cl.SUCCESS;
}

// /////////////////////////////////////////////////////////////////////
// OpenGL stuff
// /////////////////////////////////////////////////////////////////////

function createPBO(image_width, image_height) {
  // set up data parameter
  log('  create PBO');
  var num_texels = image_width * image_height;
  var num_values = num_texels * 4;
  var size_tex_data = TextureTypeSize * num_values;

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
}

function createTexture(width, height) {
  log('  create texture');

  if (TextureId)
    gl.deleteTexture(TextureId);
  TextureId = null;

  TextureWidth = width;
  TextureHeight = height;
  TextureTarget = gl.TEXTURE_2D;
  TextureInternal = gl.RGBA;
  TextureFormat = gl.RGBA;
  TextureType = gl.UNSIGNED_BYTE;
  ActiveTextureUnit = gl.TEXTURE0;

  gl.activeTexture(ActiveTextureUnit);
  TextureId = gl.createTexture();
  gl.bindTexture(TextureTarget, TextureId);
  gl.texParameteri(TextureTarget, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
  gl.texParameteri(TextureTarget, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
  gl.texImage2D(TextureTarget, 0, TextureInternal, TextureWidth, TextureHeight,
      0, TextureFormat, TextureType, null);
  gl.bindTexture(TextureTarget, null);
}

function createBuffers() {
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

function getShader(gl, id) {
  var shaders = {
    "shader-fs" : [
        "#ifdef GL_ES",
        "  precision mediump float;",
        "#endif",
        "varying vec2 vTextureCoord;",
        "uniform sampler2D uSampler;",
        "void main(void) {",
        "    gl_FragColor = texture2D(uSampler, vec2(vTextureCoord.s, vTextureCoord.t));",
        "}" ].join("\n"),

    "shader-vs" : [ "attribute vec3 aVertexPosition;",
        "attribute vec2 aTextureCoord;", "varying vec2 vTextureCoord;",
        "void main(void) {", "    gl_Position = vec4(aVertexPosition, 1.0);",
        "    vTextureCoord = aTextureCoord;", "}" ].join("\n")
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

function createShaders() {
  log('  create shaders');
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

  shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram,
      "aVertexPosition");
  gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);

  shaderProgram.textureCoordAttribute = gl.getAttribLocation(shaderProgram,
      "aTextureCoord");
  gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);

  shaderProgram.samplerUniform = gl.getUniformLocation(shaderProgram,
      "uSampler");
}

function setupGraphics(canvas) {
  log('Setup graphics');
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

  createBuffers();
  createShaders();
  createPBO(image.width, image.height);
  createTexture(image.width, image.height);

  gl.clearColor(0, 0, 0, 0);

  gl.disable(gl.DEPTH_TEST);
  gl.activeTexture(gl.TEXTURE0);
  gl.viewport(0, 0, image.width, image.height);

  return cl.SUCCESS;
}

function renderTexture() {
  // we just draw a screen-aligned texture
  gl.viewport(0, 0, image.width, image.height);

  gl.enable(TextureTarget);
  gl.bindTexture(TextureTarget, TextureId);

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

  gl.bindTexture(TextureTarget, null);
  gl.disable(TextureTarget);
}

// /////////////////////////////////////////////////////////////////////
// OpenCL stuff
// /////////////////////////////////////////////////////////////////////

function setupComputeDevices(device_type) {
  log('setup compute devices');
  ComputeDeviceType = device_type ? cl.DEVICE_TYPE_GPU : cl.DEVICE_TYPE_CPU;
  ComputeContext = cl.createContext();

  var device_ids = ComputeContext.getInfo(cl.CONTEXT_DEVICES);
  if (!device_ids) {
    alert("Error: Failed to retrieve compute devices for context!");
    return -1;
  }

  var device_found = false;
  for ( var i in device_ids) {
    device_type = device_ids[i].getInfo(cl.DEVICE_TYPE);
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

  // Create a command queue
  //
  ComputeCommands = ComputeContext.createCommandQueue(ComputeDeviceId, 0);
  if (!ComputeCommands) {
    alert("Error: Failed to create a command queue!");
    return -1;
  }

  // Report the device vendor and device name
  // 
  var vendor_name = ComputeDeviceId.getInfo(cl.DEVICE_VENDOR);
  var device_name = ComputeDeviceId.getInfo(cl.DEVICE_NAME);

  log("Connecting to " + vendor_name + " " + device_name);

  return cl.SUCCESS;
}

function setupComputeKernel() {
  log('setup compute kernel');

  ComputeKernel = null;
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
    ComputeProgram.build(null, "-cl-fast-relaxed-math");
  } catch (err) {
    log('Error building program: ' + err);
    alert("Error: Failed to build program executable!\n"
        + ComputeProgram.getBuildInfo(ComputeDeviceId, cl.PROGRAM_BUILD_LOG));
    return -1;
  }

  // Create the compute kernels from within the program
  //
  log("Creating kernel BoxRowsTex...");
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

  return resetKernelArgs(image.width, image.height, iRadius, fScale);
}

function resetKernelArgs(image_width, image_height, r, scale) {
  log('Reset kernel args to image ' + image_width + "x" + image_height + " r="
      + r + " scale=" + scale);

  // set the kernel args
  try {
    // Set the Argument values for the row kernel
    ckBoxRowsTex.setArg(0, ComputeTexture, cl.type.MEM);
    ckBoxRowsTex.setArg(1, ComputeBufTemp, cl.type.MEM);
    ckBoxRowsTex.setArg(2, RowSampler, cl.type.SAMPLER);
    ckBoxRowsTex.setArg(3, image_width, cl.type.INT);
    ckBoxRowsTex.setArg(4, image_height, cl.type.INT);
    ckBoxRowsTex.setArg(5, r, cl.type.INT);
    ckBoxRowsTex.setArg(6, scale, cl.type.FLOAT);
  } catch (err) {
    alert("Failed to set row kernel args! " + err);
    return -10;
  }

  try {
    // Set the Argument values for the column kernel
    ckBoxColumns.setArg(0, ComputeBufTemp, cl.type.MEM);
    ckBoxColumns.setArg(1, ComputePBO, cl.type.MEM);
    ckBoxColumns.setArg(2, image_width, cl.type.INT);
    ckBoxColumns.setArg(3, image_height, cl.type.INT);
    ckBoxColumns.setArg(4, r, cl.type.INT);
    ckBoxColumns.setArg(5, scale, cl.type.FLOAT);
  } catch (err) {
    alert("Failed to set column kernel args! " + err);
    return -10;
  }

  // Get the maximum work group size for executing the kernel on the device
  //
  MaxWorkGroupSize = ckBoxRowsTex.getWorkGroupInfo(ComputeDeviceId,
      cl.KERNEL_WORK_GROUP_SIZE);

  log("MaxWorkGroupSize: " + MaxWorkGroupSize);
  log("WorkGroupItems: " + WorkGroupItems);

  WorkGroupSize[0] = (MaxWorkGroupSize > 1) ? Math.round(MaxWorkGroupSize
      / WorkGroupItems) : MaxWorkGroupSize;
  WorkGroupSize[1] = Math.round(MaxWorkGroupSize / WorkGroupSize[0]);
  log("WorkGroupSize: " + WorkGroupSize);
  return cl.SUCCESS;
}

function createComputeResult() {
  log('create compute result');

  // 2D Image (Texture) on device
  var InputFormat = {
    order : cl.RGBA,
    data_type : cl.UNSIGNED_INT8
  };
  ComputeTexture = ComputeContext.createImage2D(cl.MEM_READ_ONLY
      | cl.MEM_USE_HOST_PTR, InputFormat, image.width, image.height, image.pitch, image);
  if (!ComputeTexture) {
    alert("Error: Failed to create a Image2D on device");
    return -1;
  }

  RowSampler = ComputeContext.createSampler(false, cl.ADDRESS_CLAMP,
      cl.FILTER_NEAREST);
  if (!RowSampler) {
    alert("Error: Failed to create a row sampler");
    return -1;
  }

  // Allocate the OpenCL intermediate and result buffer memory objects on the device GMEM
  ComputeBufTemp = ComputeContext.createBuffer(cl.MEM_READ_WRITE, image.szBuffBytes);
  if (!ComputeBufTemp) {
    alert("Error: Failed to create temporary buffer");
    return -1;
  }
  /*ComputeBufOut = ComputeContext.createBuffer(cl.MEM_WRITE_ONLY, image.szBuffBytes);
  if(!ComputeBufOut) {
    alert("Error: Failed to create result buffer");
    return -1;
  }*/

  // Create OpenCL representation of OpenGL PBO
  ComputePBO = ComputeContext.createFromGLBuffer(cl.MEM_WRITE_ONLY, pbo);
  if (!ComputePBO) {
    alert("Error: Failed to create CL PBO buffer");
    return -1;
  }
  return cl.SUCCESS;
}

function cleanup() {
  ComputeCommands.finish();
  ckBoxRowsTex = null;
  ckBoxColumns = null;
  RowSampler = null;
  ComputeProgram = null;
  ComputeCommands = null;
  ComputeResult = null;
  ComputeImage = null;
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
    if (newWidth > 1.25 * Width || newHeight > 1.25 * Height
        || newWidth < Width / 1.25 || newHeight < Height / 1.25) {
      cleanup();
      Width = newWidth;
      Height = newHeight;
      if (initialize(ComputeDeviceType == cl.DEVICE_TYPE_GPU) != cl.SUCCESS)
        shutdown();
    }
    gl.viewport(0, 0, newWidth, newHeight);
    gl.clear(gl.COLOR_BUFFER_BIT);
  }

  var err = recompute();
  if (err != 0) {
    alert("Error " + err + " from Recompute!");
    process.exit(1);
  }

  renderTexture();
  //reportInfo();

  gl.finish(); // for timing

  //var uiEndTime = new Date().getTime();
  //ReportStats(uiStartTime, uiEndTime);
  //DrawText(TextOffset[0], TextOffset[1], 1, (Animated == 0) ? "Press space to animate" : " ");
  return cl.SUCCESS;
}

function reshape(evt) {
  newWidth = evt.width;
  newHeight = evt.height;
  //log("reshape to "+w+'x'+h);
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

function recompute() {
  //log('recompute...');
  if (!ComputeKernel || !ComputeResult)
    return cl.SUCCESS;

  if (Animated || Update) {
    Update = false;

    // Update filter parameters
    log('Updating for new radius: ' + iRadius);
    fScale = 1 / (2 * iRadius + 1);
    ResetKernelArgs(image.width, image.height, iRadius, fScale);
  }

  // Sync GL and acquire buffer from GL
  gl.finish();
  computeCommands.enqueueAcquireGLObject(ComputePBO);

  BoxFilterGPU(image, computePBO, iRadius, fScale);

  // Release buffer
  computeCommands.enqueueReleaseGLObject(ComputePBO);

  // Update the texture from the pbo
  gl.bindTexture(gl.TEXTURE_2D, TextureId);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, image.width, image.height, gl.RGBA,
      gl.UNSIGNED_BYTE, null);
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, null);
  gl.bindTexture(gl.TEXTURE_2D, null);

  return cl.SUCCESS;
}

function BoxFilterGPU(image, cmOutputBuffer, r, scale) {
  // Setup Kernel Args
  ckBoxColumns.setArg(1, cmOutputBuffer, cl.type.MEM);

  // 2D Image (Texture)
  var TexOrigin = [ 0, 0, 0 ]; // Offset of input texture origin relative to host image
  var TexRegion = [ image.width, image.height, 1 ]; // Size of texture region to operate on
  ComputeCommands.enqueueWriteImage(ComputeTexture, cl.TRUE, TexOrigin,
      TexRegion, 0, 0, image, null, false);

  // sync host
  //ComputeCommands.finish();

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

  // sync host
  ComputeCommands.finish();
}

function main() {
  // loading image
  image = new Image();
  image.onload=function() { 
    console.log("Loaded image: " + image.src);
    log("Image Width = " + image.width + ", Height = " + image.height
        + ", bpp = 32");
    image.szBuffBytes = image.width * image.height * 4;
    
    // init window
    if(initialize(use_gpu)==cl.SUCCESS) {
      function update() {
        display();
        requestAnimationFrame(update,0);
      }
      update();
    }
  };
  image.src=__dirname+"/lenaRGB.jpg";
}

main();

