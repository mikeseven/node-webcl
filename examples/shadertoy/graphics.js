var nodejs = (typeof window === 'undefined');

/*
 * Graphics module object contains all WebGL initializations for a simple
 * 2-triangle textured screen aligned scene and its rendering.
 */
function Graphics() {
  var gl;
  var shaderProgram;
  var TextureId = null;
  var VertexPosBuffer, TexCoordsBuffer;
  if(nodejs)
    var ATB;
  var fpsFrame=0, fpsTo=0, ffps=0;
  
  /*
   * Init WebGL array buffers
   */
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

  /*
   * Compile vertex and fragment shaders
   * 
   * @param gl WebGLContext
   * @param id <script> id where the source of the shader resides
   */
  function compile_shader(gl, id) {
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
    var str = shaders[id];

    if (id.match(/-fs/)) {
      shader = gl.createShader(gl.FRAGMENT_SHADER);
    } else if (id.match(/-vs/)) {
      shader = gl.createShader(gl.VERTEX_SHADER);
    } else {
      throw 'Shader '+id+' not found';
    }

    gl.shaderSource(shader, str);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      throw gl.getShaderInfoLog(shader);
    }

    return shader;
  }

  /*
   * Initialize vertex and fragment shaders, link program and scene objects
   */
  function init_shaders() {
    log('  Init shaders');
    var fragmentShader = compile_shader(gl, "shader-fs");
    var vertexShader = compile_shader(gl, "shader-vs");
    
    shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);

    if (!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS))
      throw "Could not link shaders";

    gl.useProgram(shaderProgram);

    shaderProgram.vertexPositionAttribute = gl.getAttribLocation(shaderProgram, "aVertexPosition");
    gl.enableVertexAttribArray(shaderProgram.vertexPositionAttribute);

    shaderProgram.textureCoordAttribute = gl.getAttribLocation(shaderProgram, "aTextureCoord");
    gl.enableVertexAttribArray(shaderProgram.textureCoordAttribute);
    
    shaderProgram.samplerUniform = gl.getUniformLocation(shaderProgram, "uSampler");
  }

  /*
   * Render the scene at a timestamp.
   * 
   * @param timestamp in ms as given by new Date().getTime()
   */
  function display(timestamp) {
   // we just draw a screen-aligned texture
   gl.viewport(0, 0, gl.viewportWidth, gl.viewportHeight);
  
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
   
   fpsFrame++;
   var dt=timestamp - fpsTo;
   if( dt>1000 ) {
       ffps = 1000.0 * fpsFrame / dt;
       fpsFrame = 0;
       fpsTo = timestamp;
   }

   //if(nodejs)
   //  drawATB();
  }
  
  /*
   * Initialize WebGL
   * 
   * @param canvas HTML5 canvas object
   */
  function init(canvas) {
    log('Init GL');
    gl = canvas.getContext("experimental-webgl");
    gl.viewportWidth = canvas.width;
    gl.viewportHeight = canvas.height;
  
    init_buffers();
    init_shaders();

    //ATB=canvas.AntTweakBar;
    //initATB(canvas);
  }
  
  /*
   * Configure shared data i.e. our WebGLImage
   * 
   * @param TextureWidth width of the shared texture
   * @param TextureHeight height of the shared texture
   */
  function configure_shared_data(TextureWidth, TextureHeight) {
    if (TextureId) {
      gl.deleteTexture(TextureId);
      TextureId = null;
    }

    //if(nodejs)
    //  ATB.WindowSize(TextureWidth,TextureHeight);
    
    gl.viewportWidth = TextureWidth;
    gl.viewportHeight = TextureHeight;

    // Create OpenGL texture object
    gl.activeTexture(gl.TEXTURE0);
    TextureId = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, TextureId);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, TextureWidth, TextureHeight, 0,
        gl.RGBA, gl.UNSIGNED_BYTE, null);
    gl.bindTexture(gl.TEXTURE_2D, null);

    return TextureId;
  }
    
  var twBar,sceneChoice=0;
  var scenes=['mandelbulb','704'];
  function initATB(canvas) {
    if(!nodejs) return;
    
    ATB.Init();
    ATB.Define(" GLOBAL help='WebGL interop with WebCL' "); // Message added to the help bar.
    ATB.WindowSize(canvas.width, canvas.height);

    twBar=new ATB.NewBar("clgl");
    ATB.Define("clgl label='CLGL' refresh=0.1 position='16 16' size='200 100' valueswidth='fit' ");
    
    var sceneTypes=ATB.DefineEnum('Scene Types',scenes, 2);
    twBar.AddVar("scenes", sceneTypes, {
      getter: function(){ return sceneChoice; },
      setter: function(val){ sceneChoice=val; },
    }, " label='scene' help='select a scene' ");
    
    twBar.AddButton("Pause", function() {
      log('Pause button clicked');
    }, "label='pause' help='Pause the animation'");
    
    twBar.AddButton("Reset", function() {
      log('Reset button clicked');
    }, "label='reset' help='Reset the animation'");
    
    twBar.AddVar("fps", ATB.TYPE_FLOAT, {
      getter: function(data){ return ffps; },
    },
    " label='fps' precision=0 help='frames-per-second.'");
  }

  /*
   * Before calling AntTweakBar or any other library that could use programs, one
   * must make sure to disable the VertexAttribArray used by the current program
   * otherwise this may have some unpredictable consequences aka wrong vertex
   * attrib arrays being used by another program!
   */
  function drawATB() {
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

  return {
    'gl': function() { return gl; },
    'TextureId': function() { return TextureId; },
    'configure_shared_data': configure_shared_data,
    'init': init,
    'display': display,
    'getATB': function () { return ATB}, 
    'getBar': function () { return twBar}, 
    'clean': function() {}
  };
}

module.exports=Graphics;