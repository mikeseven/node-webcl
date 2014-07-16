/*
 * Compute contains all WebCL initializations and runtime for our kernel
 * that update a texture.
 */
function Compute() {
  var /* cl_context */        clContext;
  var /* cl_command_queue */  clQueue;
  var /* cl_program */        clProgram;
  var /* cl_device_id */      clDevice;
  var /* cl_device_type */    clDeviceType = WebCL.DEVICE_TYPE_GPU;
  var /* cl_image */          clTexture;
  var /* cl_kernel */         clKernel;
  var max_workgroup_size, max_workitem_sizes, warp_size;
  var TextureWidth, TextureHeight;
  var COMPUTE_KERNEL_ID;
  var COMPUTE_KERNEL_NAME;
  var nodejs = (typeof window === 'undefined');

  /*
   * Initialize WebCL context sharing WebGL context
   * 
   * @param gl WebGLContext
   * @param kernel_id the <script> id of the kernel source code
   * @param kernel_name name of the __kernel method
   */
  function init(gfx, kernel_id, kernel_name) {
    log('init CL');
    var gl = gfx.gl();
    if(gl === 'undefined' || kernel_id === 'undefined'
      || kernel_name === 'undefined')
      throw 'Expecting init(gl, kernel_id, kernel_name)';
    
    COMPUTE_KERNEL_ID = kernel_id;
    COMPUTE_KERNEL_NAME = kernel_name;
    
    // Pick platform
    var platformList = WebCL.getPlatforms();
    var platform = platformList[0];
    var devices = platform.getDevices(clDeviceType ? WebCL.DEVICE_TYPE_GPU : WebCL.DEVICE_TYPE_DEFAULT);
    clDevice=devices[0];

    // make sure we use a discrete GPU
    for(var i=0;i<devices.length;i++) {
      var vendor=devices[i].getInfo(WebCL.DEVICE_VENDOR);
      // log('found vendor '+vendor+', is Intel? '+(vendor.indexOf('Intel')>=0))
      if(vendor.indexOf('Intel')==-1)
        clDevice=devices[i];
    }
    log('found '+devices.length+' devices, using device: '+clDevice.getInfo(WebCL.DEVICE_NAME));

    if(!clDevice.enableExtension('KHR_gl_sharing'))
      throw new Error("Can NOT use GL sharing");

    // create the OpenCL context
    try {
      clContext = WebCL.createContext(gl, clDevice);
    }
    catch(err) {
      throw "Error: Failed to create context! "+err;
    }
    
    // Create a command queue
    try {
      clQueue = clContext.createCommandQueue(clDevice, 0);
    }
    catch(ex) {
      throw "Error: Failed to create a command queue! "+ex;
    }

    // Report the device vendor and device name
    var vendor_name = clDevice.getInfo(WebCL.DEVICE_VENDOR);
    var device_name = clDevice.getInfo(WebCL.DEVICE_NAME);
    log("  Connecting to " + vendor_name + " " + device_name);

    log("    Global mem cache size: " + (clDevice.getInfo(WebCL.DEVICE_GLOBAL_MEM_CACHE_SIZE)/1024) + "kB " );
    log("    Local mem size       : " + (clDevice.getInfo(WebCL.DEVICE_LOCAL_MEM_SIZE)/1024) + "kB " );
    log("    Max compute units    : " + clDevice.getInfo(WebCL.DEVICE_MAX_COMPUTE_UNITS));
    log("    Max work-item sizes  : "+clDevice.getInfo(WebCL.DEVICE_MAX_WORK_ITEM_SIZES));
    log("    Max work-group size  : "+clDevice.getInfo(WebCL.DEVICE_MAX_WORK_GROUP_SIZE));

    init_cl_buffers();
    init_cl_kernels();
  }

  /*
   * Initialize WebCL kernels
   */
  function init_cl_kernels() {
    log('  setup CL kernel');

    clProgram = null;

    if(!nodejs) {
      var sourceScript = document.getElementById(COMPUTE_KERNEL_ID);
      if (!sourceScript) 
        throw "Can't find CL source <script>";
  
      var str = "";
      var k = sourceScript.firstChild;
      while (k) {
        if (k.nodeType == 3) {
          str += k.textContent;
        }
        k = k.nextSibling;
      }
      if (sourceScript.type == "x-webcl") 
        source = str;
      else 
        throw "<script> type should be x-webcl";
    }
    else {
      log("Loading kernel source from file '" + COMPUTE_KERNEL_ID + "'...");
      source = fs.readFileSync(__dirname + '/' + COMPUTE_KERNEL_ID, 'ascii');
      if (!source) 
        throw "Error: Failed to load kernel source!";
    }
    

    // Create the compute program from the source buffer
    try {
      clProgram = clContext.createProgram(source);
    }
    catch(ex) {
      throw "Error: Failed to create compute program! "+ex;
    }

    // Build the program executable
    try {
      clProgram.build(clDevice, '-cl-fast-relaxed-math -cl-mad-enable -DMAC');
    } catch (err) {
      throw "Error: Failed to build program executable!\n"
          + clProgram.getBuildInfo(clDevice, WebCL.PROGRAM_BUILD_LOG);
    }

    // Create the compute kernels from within the program
    try {
      clKernel = clProgram.createKernel(COMPUTE_KERNEL_NAME);
    }
    catch(ex) {
      throw "Error: Failed to create compute kernel! "+ex;
    }

    // Get the device intrinsics for executing the kernel on the device
    max_workgroup_size = clKernel.getWorkGroupInfo(clDevice, WebCL.KERNEL_WORK_GROUP_SIZE);
    warp_size=clKernel.getWorkGroupInfo(clDevice, WebCL.KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE);
    log('  max workgroup size: '+max_workgroup_size);
    log('  local mem used    : '+clKernel.getWorkGroupInfo(clDevice, WebCL.KERNEL_LOCAL_MEM_SIZE)+" bytes");
    log('  private mem used  : '+clKernel.getWorkGroupInfo(clDevice, WebCL.KERNEL_PRIVATE_MEM_SIZE)+" bytes");
    log('  warp size         : '+warp_size);
  }

  /*
   * (Re-)set kernel arguments
   * 
   * @param time timestamp in ms (as given by new Date().getTime()
   * @param image_width width of the image
   * @param image_height height of the image
   */
  function resetKernelArgs(time, image_width, image_height) {
    TextureWidth = image_width;
    TextureHeight = image_height;
    
    // set the kernel args
    try {
      clKernel.setArg(0, clTexture);
      clKernel.setArg(1, new Float32Array([time]));
    } catch (err) {
      throw "Failed to set kernel args! " + err;
    }
  }

  /*
   * Initialize WebCL buffers
   */
  function init_cl_buffers() {
    //log('  create CL buffers');
  }
  
  /*
   * Configure shared data with WebGL i.e. our texture
   * 
   * @param gl WebGLContext
   * @param glTexture WebGLTexture to share with WebCL
   */
  function configure_shared_data(gfx, glTexture) {
    var gl=gfx.gl();
    
    // Create OpenCL representation of OpenGL Texture
    if(clTexture) clTexture.release();
    clTexture = null;
    try {
      clTexture = clContext.createFromGLTexture(WebCL.MEM_WRITE_ONLY,
          gl.TEXTURE_2D, 0, glTexture);
    } catch (ex) {
      throw "Error: Failed to create CL Texture object. " + ex;
    }

    return clTexture;
  }
  
  /*
   * Execute kernel possibly at each frame before rendering results with WebGL
   * 
   * @param gl WebGLContext
   */
  function execute_kernel(gl) {
    // Sync GL and acquire buffer from GL
    gl.flush();
    clQueue.enqueueAcquireGLObjects(clTexture);

    // Set global and local work sizes for kernel
    // doesn't work well on non-power-of-2 sizes on NVidia?
    // var local = [];
    // local[0] = warp_size;
    // local[1] = max_workgroup_size / local[0];
    // var global = [ clu.DivUp(TextureWidth, local[0]) * local[0],
    //                clu.DivUp(TextureHeight, local[1]) * local[1] ];

    // default values
    var local = null;
    var global = [ TextureWidth, TextureHeight ];

    try {
      clQueue.enqueueNDRangeKernel(clKernel, 2, null, global, local);
    } catch (err) {
      throw "Failed to enqueue kernel! " + err;
    }

    // Release GL texture
    clQueue.enqueueReleaseGLObjects(clTexture);
    clQueue.flush();
  }
  
  return {
    'init':init,
    'configure_shared_data': configure_shared_data,
    'resetKernelArgs': resetKernelArgs,
    'execute_kernel': execute_kernel,
    'getKernel' : function() { return clKernel; },
    'clean': function() {}
  }
}

module.exports=Compute;
