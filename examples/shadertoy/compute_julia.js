/*
 * Compute contains all WebCL initializations and runtime for our kernel
 * that update a texture.
 */
function Compute() {
  var cl=new WebCL();
  var orig=require('./compute')();
  var orig_resetKernelArgs = orig.resetKernelArgs;
  var orig_init = orig.init;

  var Epsilon                 = 0.003;
  var ColorA                  = [ 0.25, 0.45, 1, 1 ];
  var ColorB                  = [ 0.25, 0.45, 1, 1 ];
  var diffuse                  = [ 0.25, 0.45, 1, 1 ];
  var MuA                     = [ -0.278, -0.479, 0, 0 ];
  var MuB                     = [ 0.278, 0.479, 0, 0 ];
  var mu                     = [ -0.278, -0.479, -0.231, 0.235 ];
  var t=0;
  
  function updateMu( m, t, a, b) {
    m[0] = (1 - t) * a[0] + t * b[0];
    m[1] = (1 - t) * a[1] + t * b[1];
    m[2] = (1 - t) * a[2] + t * b[2];
    m[3] = (1 - t) * a[3] + t * b[3];

    t+=0.01;
    if(t>=1) {
      t=0;
      a[0] = b[0];
      a[1] = b[1];
      a[2] = b[2];
      a[3] = b[3];
  
      b[0] = 2 * Math.random() - 1;
      b[1] = 2 * Math.random() - 1;
      b[2] = 2 * Math.random() - 1;
      b[3] = 2 * Math.random() - 1;
    }
    return t;
  }

  function updateColor( m, t, a, b) {
    t=updateMu(m, t, a, b);
    b[3] = 1;
    return t;
  }

  function init(gfx, kernel_id, kernel_name) {
    orig_init(gfx, kernel_id, kernel_name);
    var ATB=gfx.getATB();
    var twBar=gfx.getBar();
    
    twBar.AddVar("epsilon", ATB.TYPE_FLOAT, {
      getter: function(data){ return Epsilon; },
      setter: function(val,data) { Epsilon=val; },
    },
    " label='epsilon' min=0.001 max=0.05 step=0.001 keyIncr=s keyDecr=S help='epsilon' ");

    twBar.AddVar("MuC", ATB.TYPE_QUAT4F, {
      getter: function(data){ return mu; },
    },
    " label='Mu' opened=true help='Mu' ");

    twBar.AddVar("Color", ATB.TYPE_COLOR4F, {
      getter: function(data){ return diffuse; },
    },
    " label='Color' opened=true help='Color' ");
  }
  
  /*
   * (Re-)set kernel arguments
   * 
   * @param time timestamp in ms (as given by new Date().getTime()
   * @param image_width width of the image
   * @param image_height height of the image
   */
  function resetKernelArgs(time, image_width, image_height) {
    orig_resetKernelArgs(time, image_width, image_height);
    
    updateMu(mu, t, MuA, MuB);
    t = updateColor(diffuse, t, ColorA, ColorB);
    
    // set the kernel args
    var clKernel = orig.getKernel();
    try {
      // Set the Argument values for the row kernel
      clKernel.setArg(2, mu, cl.type.FLOAT | cl.type.VEC4);
      clKernel.setArg(3, diffuse, cl.type.FLOAT | cl.type.VEC4);
      clKernel.setArg(4, Epsilon, cl.type.FLOAT);
    } catch (err) {
      throw "Failed to set Julia kernel args! " + err;
    }
  }
  
  orig.init = init;
  orig.resetKernelArgs = resetKernelArgs;
  return orig;
}

module.exports=Compute;
