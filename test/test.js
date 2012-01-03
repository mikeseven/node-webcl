/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var cl=require("../webcl"),
    sys=require('util'),
    log=console.log;

//log("cl objects: "+sys.inspect(cl));

//list of platforms
log("OpenCL SW Info:\n");
var platforms=cl.getPlatforms();
console.log('Found '+platforms.length+' plaforms');
for(var i in platforms) {
  var p=platforms[i];  
  log("  PLATFORM_NAME: \t"+p.getPlatformInfo(cl.PLATFORM_NAME));
  log("  PLATFORM_PROFILE: \t"+p.getPlatformInfo(cl.PLATFORM_PROFILE));
  log("  PLATFORM_VERSION: \t"+p.getPlatformInfo(cl.PLATFORM_VERSION));
  log("  PLATFORM_VENDOR: \t"+p.getPlatformInfo(cl.PLATFORM_VENDOR));
  log("  PLATFORM_EXTENSIONS: \t"+p.getPlatformInfo(cl.PLATFORM_EXTENSIONS));

  //list for devices for a platform
  log("OpenCL Device Info:\n");
  var devices=p.getDevices(cl.DEVICE_TYPE_ALL);
  log('Found '+devices.length+' devices');
  for(i=0;i<devices.length;i++) {
    var device=devices[i];
    log(" ---------------------------------");
    log(' Device: '+device.getDeviceInfo(cl.DEVICE_NAME));
    log(" ---------------------------------");
    printDeviceInfo(device);
  }

  //Create a context for the devices
  var cxGPUContext = cl.createContext(cl.DEVICE_TYPE_GPU, [cl.CONTEXT_PLATFORM, p]);
  var ctxDevices=cxGPUContext.getContextInfo(cl.CONTEXT_DEVICES);
}

function printDeviceInfo(device)
{
  //log("  Device ID: \t\t\t0x"+device.getID().toString(16));
  log("  DEVICE_NAME: \t\t\t"+device.getDeviceInfo(cl.DEVICE_NAME));
  log("  DEVICE_VENDOR: \t\t\t"+device.getDeviceInfo(cl.DEVICE_VENDOR));
  log("  DRIVER_VERSION: \t\t\t"+device.getDeviceInfo(cl.DRIVER_VERSION));
  log("  DEVICE_VERSION: \t\t\t"+device.getDeviceInfo(cl.DEVICE_VERSION));
  log("  DEVICE_PROFILE: \t\t\t"+device.getDeviceInfo(cl.DEVICE_PROFILE));
  log("  DEVICE_PLATFORM: \t\t\t0x"+device.getDeviceInfo(cl.DEVICE_PLATFORM).toString(16));
  log("  DEVICE_OPENCL_C_VERSION: \t\t"+device.getDeviceInfo(cl.DEVICE_OPENCL_C_VERSION));
  var type=parseInt(device.getDeviceInfo(cl.DEVICE_TYPE));
  if( type & cl.DEVICE_TYPE_CPU )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_CPU");
  if( type & cl.DEVICE_TYPE_GPU )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_GPU");
  if( type & cl.DEVICE_TYPE_ACCELERATOR )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_ACCELERATOR");
  if( type & cl.DEVICE_TYPE_DEFAULT )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_DEFAULT");

  var compute_units=0;
  log("  DEVICE_MAX_COMPUTE_UNITS:\t\t"+(compute_units=device.getDeviceInfo(cl.DEVICE_MAX_COMPUTE_UNITS)));
  log("  DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t"+device.getDeviceInfo(cl.DEVICE_MAX_WORK_ITEM_DIMENSIONS));

  var workitem_size=device.getDeviceInfo(cl.DEVICE_MAX_WORK_ITEM_SIZES);
  log("  DEVICE_MAX_WORK_ITEM_SIZES:\t"+workitem_size[0]+" / "+workitem_size[1]+" / "+workitem_size[2]);

  log("  DEVICE_MAX_WORK_GROUP_SIZE:\t"+device.getDeviceInfo(cl.DEVICE_MAX_WORK_GROUP_SIZE));
  log("  DEVICE_MAX_CLOCK_FREQUENCY:\t"+device.getDeviceInfo(cl.DEVICE_MAX_CLOCK_FREQUENCY)+" MHz");
  log("  DEVICE_ADDRESS_BITS:\t\t"+device.getDeviceInfo(cl.DEVICE_ADDRESS_BITS));
  log("  DEVICE_MAX_MEM_ALLOC_SIZE:\t\t"+(device.getDeviceInfo(cl.DEVICE_MAX_MEM_ALLOC_SIZE) / (1024 * 1024))+" MBytes");
  log("  DEVICE_GLOBAL_MEM_SIZE:\t\t"+(device.getDeviceInfo(cl.DEVICE_GLOBAL_MEM_SIZE) / (1024 * 1024))+" MBytes");
  log("  DEVICE_ERROR_CORRECTION_SUPPORT:\t"+(device.getDeviceInfo(cl.DEVICE_ERROR_CORRECTION_SUPPORT) == cl.TRUE ? "yes" : "no"));
  log("  DEVICE_LOCAL_MEM_TYPE:\t\t"+(device.getDeviceInfo(cl.DEVICE_LOCAL_MEM_TYPE) == 1 ? "local" : "global"));
  log("  DEVICE_LOCAL_MEM_SIZE:\t\t"+(device.getDeviceInfo(cl.DEVICE_LOCAL_MEM_SIZE) / 1024)+" KBytes");
  log("  DEVICE_MAX_CONSTANT_BUFFER_SIZE:\t"+(device.getDeviceInfo(cl.DEVICE_MAX_CONSTANT_BUFFER_SIZE) / 1024)+" KBytes");
  log("  DEVICE_MAX_CONSTANT_BUFFER_SIZE:\t"+(device.getDeviceInfo(cl.DEVICE_MAX_CONSTANT_BUFFER_SIZE) / 1024)+" KBytes");

  log("  DEVICE_MAX_SAMPLERS:\t"+device.getDeviceInfo(cl.DEVICE_MAX_SAMPLERS));
  log("  DEVICE_MAX_PARAMETER_SIZE:\t"+device.getDeviceInfo(cl.DEVICE_MAX_PARAMETER_SIZE));
  log("  DEVICE_MEM_BASE_ADDR_ALIGN:\t"+device.getDeviceInfo(cl.DEVICE_MEM_BASE_ADDR_ALIGN));
  log("  DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:\t"+device.getDeviceInfo(cl.DEVICE_MIN_DATA_TYPE_ALIGN_SIZE));

  var cache_type=device.getDeviceInfo(cl.DEVICE_GLOBAL_MEM_CACHE_TYPE);
  if( cache_type & cl.NONE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tNONE");
  if( cache_type & cl.READ_ONLY_CACHE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tREAD_ONLY_CACHE");
  if( cache_type & cl.READ_WRITE_CACHE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tREAD_WRITE_CACHE");
    
  log("  DEVICE_MAX_CONSTANT_ARGS:\t"+device.getDeviceInfo(cl.DEVICE_MAX_CONSTANT_ARGS));
  log("  DEVICE_HOST_UNIFIED_MEMORY:\t"+device.getDeviceInfo(cl.DEVICE_HOST_UNIFIED_MEMORY));
  log("  DEVICE_PROFILING_TIMER_RESOLUTION:\t"+device.getDeviceInfo(cl.DEVICE_PROFILING_TIMER_RESOLUTION));
  log("  DEVICE_ENDIAN_LITTLE:\t"+device.getDeviceInfo(cl.DEVICE_ENDIAN_LITTLE));

  log("  DEVICE_AVAILABLE:\t"+device.getDeviceInfo(cl.DEVICE_AVAILABLE));
  log("  DEVICE_COMPILER_AVAILABLE:\t"+device.getDeviceInfo(cl.DEVICE_COMPILER_AVAILABLE));
  log("  DEVICE_EXECUTION_CAPABILITIES:\t"+device.getDeviceInfo(cl.DEVICE_EXECUTION_CAPABILITIES));

  // DEVICE_QUEUE_PROPERTIES
  var queue_properties=device.getDeviceInfo(cl.DEVICE_QUEUE_PROPERTIES);
  if( queue_properties & cl.QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
    log("  DEVICE_QUEUE_PROPERTIES:\t\tQUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE");
  if( queue_properties & cl.QUEUE_PROFILING_ENABLE )
    log("  DEVICE_QUEUE_PROPERTIES:\t\tQUEUE_PROFILING_ENABLE");

  // image support
  log("  DEVICE_IMAGE_SUPPORT:\t\t"+device.getDeviceInfo(cl.DEVICE_IMAGE_SUPPORT));
  log("  DEVICE_MAX_READ_IMAGE_ARGS:\t"+device.getDeviceInfo(cl.DEVICE_MAX_READ_IMAGE_ARGS));
  log("  DEVICE_MAX_WRITE_IMAGE_ARGS:\t"+device.getDeviceInfo(cl.DEVICE_MAX_WRITE_IMAGE_ARGS));

  // DEVICE_SINGLE_FP_CONFIG
  var fp_config=device.getDeviceInfo(cl.DEVICE_SINGLE_FP_CONFIG);
  log("  DEVICE_SINGLE_FP_CONFIG:\t\t"+
      (fp_config & cl.FP_DENORM ? "denorms " : "")+
      (fp_config & cl.FP_INF_NAN ? "INF-quietNaNs " : "")+
      (fp_config & cl.FP_ROUND_TO_NEAREST ? "round-to-nearest " : "")+
      (fp_config & cl.FP_ROUND_TO_ZERO ? "round-to-zero " : "")+
      (fp_config & cl.FP_ROUND_TO_INF ? "round-to-inf " : "")+
      (fp_config & cl.FP_FMA ? "fma " : ""));

  log("\n  DEVICE_IMAGE <dim>"); 
  log("\t\t\t\t\t2D_MAX_WIDTH\t "+device.getDeviceInfo(cl.DEVICE_IMAGE2D_MAX_WIDTH));
  log("\t\t\t\t\t2D_MAX_HEIGHT\t "+device.getDeviceInfo(cl.DEVICE_IMAGE2D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_WIDTH\t "+device.getDeviceInfo(cl.DEVICE_IMAGE3D_MAX_WIDTH));
  log("\t\t\t\t\t3D_MAX_HEIGHT\t "+device.getDeviceInfo(cl.DEVICE_IMAGE3D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_DEPTH\t "+device.getDeviceInfo(cl.DEVICE_IMAGE3D_MAX_DEPTH));

  // DEVICE_EXTENSIONS: get device extensions, and if any then parse & log the string onto separate lines
  log("\n  DEVICE_EXTENSIONS:");
  var device_string=device.getDeviceInfo(cl.DEVICE_EXTENSIONS);
  var nv_device_attibute_query=false;
  if (device_string.length > 0) 
  {
    var extensions=device_string.split(' ');
    for(var i=0;i<extensions.length;i++) {
      if("nv_device_attribute_query"===extensions[i])
        nv_device_attibute_query=true;
      log("\t\t\t\t\t"+extensions[i]);
    }
  }
  else 
    log("  DEVICE_EXTENSIONS: None");

  if(nv_device_attibute_query) 
  {
    var compute_capability_major=device.getDeviceInfo(cl.DEVICE_COMPUTE_CAPABILITY_MAJOR_NV);
    var compute_capability_minor=device.getDeviceInfo(cl.DEVICE_COMPUTE_CAPABILITY_MINOR_NV);
    log("\n  DEVICE_COMPUTE_CAPABILITY_NV:\t"+compute_capability_major+'.'+compute_capability_minor);
    log("  NUMBER OF MULTIPROCESSORS:\t\t"+compute_units); // this is the same value reported by DEVICE_MAX_COMPUTE_UNITS
    log("  NUMBER OF CUDA CORES:\t\t\t"+NV_ConvertSMVer2Cores(compute_capability_major, compute_capability_minor) * compute_units);
    log("  DEVICE_REGISTERS_PER_BLOCK_NV:\t"+device.getDeviceInfo(cl.DEVICE_REGISTERS_PER_BLOCK_NV));
    log("  DEVICE_WARP_SIZE_NV:\t\t"+device.getDeviceInfo(cl.DEVICE_WARP_SIZE_NV));
    log("  DEVICE_GPU_OVERLAP_NV:\t\t"+(device.getDeviceInfo(cl.DEVICE_GPU_OVERLAP_NV) == cl.TRUE ? "TRUE" : "FALSE"));
    log("  DEVICE_KERNEL_EXEC_TIMEOUT_NV:\t"+(device.getDeviceInfo(cl.DEVICE_KERNEL_EXEC_TIMEOUT_NV) == cl.TRUE ? "TRUE" : "FALSE"));
    log("  DEVICE_INTEGRATED_MEMORY_NV:\t"+(device.getDeviceInfo(cl.DEVICE_INTEGRATED_MEMORY_NV) == cl.TRUE ? "TRUE" : "FALSE"));
  }

  // DEVICE_PREFERRED_VECTOR_WIDTH_<type>
  log("  DEVICE_PREFERRED_VECTOR_WIDTH_<t>"); 
  var vec_width= [
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_CHAR),
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_SHORT),
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_INT),
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_LONG),
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT),
                  device.getDeviceInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE),
                  ];
  log('\tCHAR '+vec_width[0]+', SHORT '+vec_width[1]+', INT '+vec_width[2]+', LONG '+vec_width[3]+
      ', FLOAT '+vec_width[4]+', DOUBLE '+vec_width[5]);

  log("  DEVICE_NATIVE_VECTOR_WIDTH_<t>"); 
  var vec_native_width= [
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_CHAR),
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_SHORT),
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_INT),
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_LONG),
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_FLOAT),
                         device.getDeviceInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE),
                         ];
  log('\tCHAR '+vec_native_width[0]+', SHORT '+vec_native_width[1]+', INT '+vec_native_width[2]+
      ', LONG '+vec_native_width[3]+', FLOAT '+vec_native_width[4]+', DOUBLE '+vec_native_width[5]);
}
