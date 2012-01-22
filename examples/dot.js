var cl = require('../webcl'), 
    util = require('util'), 
    fs = require('fs'), 
    WebGL = require('webgl'), 
    document = WebGL.document(), 
    Image = WebGL.Image, 
    log = console.log, 
    requestAnimationFrame = document.requestAnimationFrame;

var platform;
var device;
var context;
var program;
var queue;
var kernel;
var in_texture, out_buffer;
var vbo = {}, pbo, sampler, texture;
var width, height;
var tex_pixels;
var window_width=800, window_height=800;

var vertex_coords = [ -1, -1, 0, 
                      1, -1, 0, 
                      1, 1, 0, 
                      -1, 1, 0 ];

var tex_coords = [ 0, 0, 
                   1, 0, 
                   1, 1, 
                   0, 1 ];

main();

function read_image_data(filename) {
  var image = new Image();
  image.onload=function(s) { // [MBS] was onload() event
    console.log("Loaded image: " + s);
    log("Image Width = " + image.width + ", Height = " + image.height);

    // adjust window to pixel ratio
    window_height *= image.height/image.width;
    document.createWindow(window_width, window_height);
  };
  image.src = filename;

  return image;
}

/* Initialize OpenCL processing */
function init_cl() {

  /* Identify a platform */
  platform = cl.getPlatforms()[0];

  /* Access a device */
  device = platform.getDevices(cl.DEVICE_TYPE_GPU)[0];

  // get CL-GL extension
  var extensions = device.getDeviceInfo(cl.DEVICE_EXTENSIONS);
  var hasGLSupport = extensions.search(/gl.sharing/i) >= 0;
  log(hasGLSupport ? "GL-CL extension available ;-)" : "No GL support");
  if (!hasGLSupport)
    return;
  clgl = device.getExtension(cl.GL_CONTEXT_KHR);
  if (clgl == undefined)
    process.exit(-1);

  /* Create OpenCL context properties */
  var properties = [ cl.GL_CONTEXT_KHR, gl, cl.CONTEXT_PLATFORM, platform ];

  /* Create context */
  context = cl.createContext(device, properties);

  /* Create program from file */
  var program_buffer = fs.readFileSync(__dirname + '/dot.cl', 'ascii');
  program = context.createProgram(program_buffer);

  /* Build program */
  program.build(device);

  /* Create a command queue */
  queue = context.createCommandQueue(device, 0);

  /* Create kernel */
  kernel = program.createKernel("texture_filter");

  /* Read pixel data */
  tex_pixels = read_image_data(__dirname + '/' + 'mike_scooter.jpg');
  
  /* Create the input image object from the PNG data */
  var img_format = {
    order : cl.RGBA,
    data_type : cl.UNSIGNED_INT8
  };

  in_texture =
      context.createImage2D(cl.MEM_READ_ONLY | cl.MEM_COPY_HOST_PTR,
          img_format, tex_pixels.width, tex_pixels.height, 
          tex_pixels.width * 4, tex_pixels);

  /* Create kernel arguments */
  kernel.setArg(0, in_texture, cl.type.MEM);
}

/* Create and configure vertex buffer objects (VBOs) */
function init_buffers() {

  /* Create VBOs */
  vbo.coord = gl.createBuffer();
  vbo.tc = gl.createBuffer();

  /* VBO to hold vertex coordinates */
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo.coord);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertex_coords), gl.STATIC_DRAW);
  gl.vertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 0, 0);
  gl.enableVertexAttribArray(0);

  /* VBO to hold texture coordinates */
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo.tc);
  gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(tex_coords), gl.STATIC_DRAW);
  gl.vertexAttribPointer(1, 2, gl.FLOAT, gl.FALSE, 0, 0);
  gl.enableVertexAttribArray(1);
}

function init_textures() {

  /* Create texture and make it active */
  gl.enable(gl.TEXTURE_2D);
  texture = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, texture);

  /* Set texture parameters */
  //gl.pixelStorei(gl.UNPACK_ALIGNMENT, 4);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.REPEAT);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
}

/* Create and compile shaders */
function init_shaders() {
  var vs = gl.createShader(gl.VERTEX_SHADER);
  var fs = gl.createShader(gl.FRAGMENT_SHADER);

  var fs_source =
      [ "uniform sampler2D tex;", 
        "out varying vec4 new_color;", 
        "void main() {",
        "vec3 color = vec3(texture(tex, gl_TexCoord[0].st));",
        "new_color = vec4(color, 1.0);", 
        "}" ].join('\n');
  var vs_source =
      [ "in  vec3 in_coords;", 
        "in  vec2 in_texcoords;", 
        "void main(void) {",
        "gl_TexCoord[0].st = in_texcoords;",
        "gl_Position = vec4(in_coords, 1.0);", 
        "}" ].join('\n');

  gl.shaderSource(vs, vs_source);
  gl.shaderSource(fs, fs_source);

  gl.compileShader(vs);
  gl.compileShader(fs);

  var prog = gl.createProgram();

  gl.bindAttribLocation(prog, 0, "in_coords");
  gl.bindAttribLocation(prog, 1, "in_color");

  gl.attachShader(prog, vs);
  gl.attachShader(prog, fs);

  gl.linkProgram(prog);
  gl.useProgram(prog);
}

/* Initialize the rendering objects and context properties */
function init_gl() {

  /* Initialize the main window */
  document.setTitle("Texture filter");
  canvas = document.createElement("mycanvas", window_width, window_height);
  try {
    gl = canvas.getContext("experimental-webgl");
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;
  } catch (e) {
  }
  if (!gl) {
    alert("Could not initialise WebGL, sorry :-(");
    process.exit(-1);
  }

  gl.clearColor(1, 1, 1, 1);

  /* Create VBOs */
  init_buffers();

  /* Create texture */
  init_textures();

  /* Create and compile shaders */
  init_shaders();
}

function configure_shared_data() {

  /* Create and configure pixel buffer */
  pbo = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, pbo);
  gl.bufferData(gl.ARRAY_BUFFER, tex_pixels.width * tex_pixels.height * 4, gl.STATIC_DRAW);
  gl.bindBuffer(gl.ARRAY_BUFFER, null);

  out_buffer = clgl.createFromGLBuffer(context, cl.MEM_WRITE_ONLY, pbo);

  kernel.setArg(1, out_buffer, cl.type.MEM);
}

function execute_kernel() {

  var kernel_event;
  var global_size = [ 0, 0 ];

  /* Complete OpenGL processing */
  gl.finish();

  /* Acquire exclusive access to OpenGL buffer */
  clgl.enqueueAcquireGLObjects(queue, out_buffer);

  /* Execute the kernel */
  global_size[0] = tex_pixels.width;
  global_size[1] = tex_pixels.height;
  queue.enqueueNDRangeKernel(kernel, null, global_size, null);

  clgl.enqueueReleaseGLObjects(queue, out_buffer);
  queue.finish();

  /* Copy pixels from pbo to texture */
  gl.bindBuffer(gl.PIXEL_UNPACK_BUFFER, pbo);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, tex_pixels.width, tex_pixels.height, 
      0, gl.LUMINANCE, gl.UNSIGNED_BYTE, null);
  gl.activeTexture(gl.TEXTURE0);
}

function display() {
  gl.viewport(0, 0, window_width, window_height);
  gl.clear(gl.COLOR_BUFFER_BIT);

  gl.bindTexture(gl.TEXTURE_2D, texture);
  gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);

  requestAnimationFrame(display);
}

function main() {

  /* Start GL processing */
  init_gl();

  /* Initialize CL data structures */
  init_cl();

  /* Create CL and GL data objects */
  configure_shared_data();

  /* Execute kernel */
  execute_kernel();

  display();
}
