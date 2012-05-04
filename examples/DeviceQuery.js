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
  WebCL = require('../webcl');
  clu = require('../lib/clUtils');
  util = require('util');
  log = console.log;
}

var cl=new WebCL();

//list of platforms
log("OpenCL SW Info:\n");
var platforms=cl.getPlatforms();
console.log('Found '+platforms.length+' plaforms');
platforms.forEach(function(p) {
  //log("  Plaftorm ID: 0x"+p.getID().toString(16));
  log("  PLATFORM_NAME: \t"+p.getInfo(cl.PLATFORM_NAME));
  log("  PLATFORM_PROFILE: \t"+p.getInfo(cl.PLATFORM_PROFILE));
  log("  PLATFORM_VERSION: \t"+p.getInfo(cl.PLATFORM_VERSION));
  log("  PLATFORM_VENDOR: \t"+p.getInfo(cl.PLATFORM_VENDOR));
  log("  PLATFORM_EXTENSIONS: \t"+p.getInfo(cl.PLATFORM_EXTENSIONS));

  //list for devices for a platform
  log("OpenCL Device Info:\n");
  var devices = p.getDevices(cl.DEVICE_TYPE_ALL);
  log('Found '+devices.length+' devices');
  devices.forEach(function(device) {
    log(" ---------------------------------");
    log(' Device: '+device.getInfo(cl.DEVICE_NAME));
    log(" ---------------------------------");
    printDeviceInfo(device);
  
    //Create a context for the devices
    var context;
    try {
      context = cl.createContext({ 
        devices: device,
        platform: p} ); //cl.createContext(cl.DEVICE_TYPE_DEFAULT, [cl.CONTEXT_PLATFORM, p]);
    }
    catch(ex) {
      log("createContext() exception: "+ex);
      log();
      log("*************************************************");
      log("* ERROR: Can NOT create context for this device *");
      log("*************************************************");
      log();
      return;
    }
    
    var ctxDevices=context.getInfo(cl.CONTEXT_DEVICES);
    
    // Determine and show image format support 
    // 2D
    ImageFormats=context.getSupportedImageFormats(cl.MEM_READ_ONLY, cl.MEM_OBJECT_IMAGE2D);
    uiNumSupportedFormats=ImageFormats.length;
    
    log("  ---------------------------------");
    log("  2D Image Formats Supported", uiNumSupportedFormats); 
    log("  ---------------------------------");
    log(clu.sprintf("%-6s%-16s%-22s\n", "#", "Channel Order", "Channel Type"));
    for(var j = 0; j < uiNumSupportedFormats; j++) 
    {  
      log(clu.sprintf("  %-6u%-16s%-22s", (j + 1),
          oclImageFormatString(ImageFormats[j].order), 
          oclImageFormatString(ImageFormats[j].data_type)));
    }
    log("");
    
    // 3D
    ImageFormats=context.getSupportedImageFormats(cl.MEM_READ_ONLY, cl.MEM_OBJECT_IMAGE3D);
    uiNumSupportedFormats=ImageFormats.length;
    
    log("  ---------------------------------");
    log("  3D Image Formats Supported", uiNumSupportedFormats); 
    log("  ---------------------------------");
    log(clu.sprintf("%-6s%-16s%-22s\n", "#", "Channel Order", "Channel Type"));
    for(var j = 0; j < uiNumSupportedFormats; j++) 
    {  
      log(clu.sprintf("  %-6u%-16s%-22s", (j + 1),
          oclImageFormatString(ImageFormats[j].order), 
          oclImageFormatString(ImageFormats[j].data_type)));
    }
    log(""); 
  });
  
  // Size of basic types
  log("  size of char: \t"+cl.size.CHAR);
  log("  size of short: \t"+cl.size.SHORT);
  log("  size of int: \t"+cl.size.INT);
  log("  size of long: \t"+cl.size.LONG);
  log("  size of float: \t"+cl.size.FLOAT);
  log("  size of double: \t"+cl.size.DOUBLE);
  log("  size of half: \t"+cl.size.HALF);
});

function printDeviceInfo(device)
{
  log("  DEVICE_NAME: \t\t\t"+device.getInfo(cl.DEVICE_NAME));
  log("  DEVICE_VENDOR: \t\t\t"+device.getInfo(cl.DEVICE_VENDOR));
  log("  DRIVER_VERSION: \t\t\t"+device.getInfo(cl.DRIVER_VERSION));
  log("  DEVICE_VERSION: \t\t\t"+device.getInfo(cl.DEVICE_VERSION));
  log("  DEVICE_PROFILE: \t\t\t"+device.getInfo(cl.DEVICE_PROFILE));
  log("  DEVICE_PLATFORM: \t\t\t0x"+device.getInfo(cl.DEVICE_PLATFORM).toString(16));
  log("  DEVICE_OPENCL_C_VERSION: \t\t"+device.getInfo(cl.DEVICE_OPENCL_C_VERSION));
  var type=parseInt(device.getInfo(cl.DEVICE_TYPE));
  if( type & cl.DEVICE_TYPE_CPU )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_CPU");
  if( type & cl.DEVICE_TYPE_GPU )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_GPU");
  if( type & cl.DEVICE_TYPE_ACCELERATOR )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_ACCELERATOR");
  if( type & cl.DEVICE_TYPE_DEFAULT )
    log("  DEVICE_TYPE:\t\t\tDEVICE_TYPE_DEFAULT");

  var compute_units=0;
  log("  DEVICE_MAX_COMPUTE_UNITS:\t\t"+(compute_units=device.getInfo(cl.DEVICE_MAX_COMPUTE_UNITS)));
  log("  DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t"+device.getInfo(cl.DEVICE_MAX_WORK_ITEM_DIMENSIONS));

  var workitem_size=device.getInfo(cl.DEVICE_MAX_WORK_ITEM_SIZES);
  log("  DEVICE_MAX_WORK_ITEM_SIZES:\t"+workitem_size[0]+" / "+workitem_size[1]+" / "+workitem_size[2]);

  log("  DEVICE_MAX_WORK_GROUP_SIZE:\t"+device.getInfo(cl.DEVICE_MAX_WORK_GROUP_SIZE));
  log("  DEVICE_MAX_CLOCK_FREQUENCY:\t"+device.getInfo(cl.DEVICE_MAX_CLOCK_FREQUENCY)+" MHz");
  log("  DEVICE_ADDRESS_BITS:\t\t"+device.getInfo(cl.DEVICE_ADDRESS_BITS));
  log("  DEVICE_MAX_MEM_ALLOC_SIZE:\t\t"+(device.getInfo(cl.DEVICE_MAX_MEM_ALLOC_SIZE) / (1024 * 1024))+" MBytes");
  log("  DEVICE_GLOBAL_MEM_SIZE:\t\t"+(device.getInfo(cl.DEVICE_GLOBAL_MEM_SIZE) / (1024 * 1024))+" MBytes");
  log("  DEVICE_ERROR_CORRECTION_SUPPORT:\t"+(device.getInfo(cl.DEVICE_ERROR_CORRECTION_SUPPORT) == cl.TRUE ? "yes" : "no"));
  log("  DEVICE_LOCAL_MEM_TYPE:\t\t"+(device.getInfo(cl.DEVICE_LOCAL_MEM_TYPE) == 1 ? "local" : "global"));
  log("  DEVICE_LOCAL_MEM_SIZE:\t\t"+(device.getInfo(cl.DEVICE_LOCAL_MEM_SIZE) / 1024)+" KBytes");
  log("  DEVICE_MAX_CONSTANT_BUFFER_SIZE:\t"+(device.getInfo(cl.DEVICE_MAX_CONSTANT_BUFFER_SIZE) / 1024)+" KBytes");
  log("  DEVICE_MAX_CONSTANT_BUFFER_SIZE:\t"+(device.getInfo(cl.DEVICE_MAX_CONSTANT_BUFFER_SIZE) / 1024)+" KBytes");

  log("  DEVICE_MAX_SAMPLERS:\t"+device.getInfo(cl.DEVICE_MAX_SAMPLERS));
  log("  DEVICE_MAX_PARAMETER_SIZE:\t"+device.getInfo(cl.DEVICE_MAX_PARAMETER_SIZE));
  log("  DEVICE_MEM_BASE_ADDR_ALIGN:\t"+device.getInfo(cl.DEVICE_MEM_BASE_ADDR_ALIGN));
  log("  DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:\t"+device.getInfo(cl.DEVICE_MIN_DATA_TYPE_ALIGN_SIZE));

  var cache_type=device.getInfo(cl.DEVICE_GLOBAL_MEM_CACHE_TYPE);
  if( cache_type & cl.NONE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tNONE");
  if( cache_type & cl.READ_ONLY_CACHE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tREAD_ONLY_CACHE");
  if( cache_type & cl.READ_WRITE_CACHE)
    log("  DEVICE_GLOBAL_MEM_CACHE_TYPE:\t\tREAD_WRITE_CACHE");
    
  log("  DEVICE_MAX_CONSTANT_ARGS:\t"+device.getInfo(cl.DEVICE_MAX_CONSTANT_ARGS));
  log("  DEVICE_HOST_UNIFIED_MEMORY:\t"+device.getInfo(cl.DEVICE_HOST_UNIFIED_MEMORY));
  log("  DEVICE_PROFILING_TIMER_RESOLUTION:\t"+device.getInfo(cl.DEVICE_PROFILING_TIMER_RESOLUTION));
  log("  DEVICE_ENDIAN_LITTLE:\t"+device.getInfo(cl.DEVICE_ENDIAN_LITTLE));

  log("  DEVICE_AVAILABLE:\t"+device.getInfo(cl.DEVICE_AVAILABLE));
  log("  DEVICE_COMPILER_AVAILABLE:\t"+device.getInfo(cl.DEVICE_COMPILER_AVAILABLE));
  log("  DEVICE_EXECUTION_CAPABILITIES:\t"+device.getInfo(cl.DEVICE_EXECUTION_CAPABILITIES));

  // DEVICE_QUEUE_PROPERTIES
  var queue_properties=device.getInfo(cl.DEVICE_QUEUE_PROPERTIES);
  if( queue_properties & cl.QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
    log("  DEVICE_QUEUE_PROPERTIES:\t\tQUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE");
  if( queue_properties & cl.QUEUE_PROFILING_ENABLE )
    log("  DEVICE_QUEUE_PROPERTIES:\t\tQUEUE_PROFILING_ENABLE");

  // image support
  log("  DEVICE_IMAGE_SUPPORT:\t\t"+device.getInfo(cl.DEVICE_IMAGE_SUPPORT));
  log("  DEVICE_MAX_READ_IMAGE_ARGS:\t"+device.getInfo(cl.DEVICE_MAX_READ_IMAGE_ARGS));
  log("  DEVICE_MAX_WRITE_IMAGE_ARGS:\t"+device.getInfo(cl.DEVICE_MAX_WRITE_IMAGE_ARGS));

  // DEVICE_SINGLE_FP_CONFIG
  var fp_config=device.getInfo(cl.DEVICE_SINGLE_FP_CONFIG);
  log("  DEVICE_SINGLE_FP_CONFIG:\t\t"+
      (fp_config & cl.FP_DENORM ? "denorms " : "")+
      (fp_config & cl.FP_INF_NAN ? "INF-quietNaNs " : "")+
      (fp_config & cl.FP_ROUND_TO_NEAREST ? "round-to-nearest " : "")+
      (fp_config & cl.FP_ROUND_TO_ZERO ? "round-to-zero " : "")+
      (fp_config & cl.FP_ROUND_TO_INF ? "round-to-inf " : "")+
      (fp_config & cl.FP_FMA ? "fma " : ""));

  log("\n  DEVICE_IMAGE <dim>"); 
  log("\t\t\t\t\t2D_MAX_WIDTH\t "+device.getInfo(cl.DEVICE_IMAGE2D_MAX_WIDTH));
  log("\t\t\t\t\t2D_MAX_HEIGHT\t "+device.getInfo(cl.DEVICE_IMAGE2D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_WIDTH\t "+device.getInfo(cl.DEVICE_IMAGE3D_MAX_WIDTH));
  log("\t\t\t\t\t3D_MAX_HEIGHT\t "+device.getInfo(cl.DEVICE_IMAGE3D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_DEPTH\t "+device.getInfo(cl.DEVICE_IMAGE3D_MAX_DEPTH));

  // DEVICE_EXTENSIONS: get device extensions, and if any then parse & log the string onto separate lines
  log("\n  DEVICE_EXTENSIONS:");
  var device_string=device.getInfo(cl.DEVICE_EXTENSIONS);
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
    var compute_capability_major=device.getInfo(cl.DEVICE_COMPUTE_CAPABILITY_MAJOR_NV);
    var compute_capability_minor=device.getInfo(cl.DEVICE_COMPUTE_CAPABILITY_MINOR_NV);
    log("\n  DEVICE_COMPUTE_CAPABILITY_NV:\t"+compute_capability_major+'.'+compute_capability_minor);
    log("  NUMBER OF MULTIPROCESSORS:\t\t"+compute_units); // this is the same value reported by DEVICE_MAX_COMPUTE_UNITS
    log("  NUMBER OF CUDA CORES:\t\t\t"+NV_ConvertSMVer2Cores(compute_capability_major, compute_capability_minor) * compute_units);
    log("  DEVICE_REGISTERS_PER_BLOCK_NV:\t"+device.getInfo(cl.DEVICE_REGISTERS_PER_BLOCK_NV));
    log("  DEVICE_WARP_SIZE_NV:\t\t"+device.getInfo(cl.DEVICE_WARP_SIZE_NV));
    log("  DEVICE_GPU_OVERLAP_NV:\t\t"+(device.getInfo(cl.DEVICE_GPU_OVERLAP_NV) == cl.TRUE ? "TRUE" : "FALSE"));
    log("  DEVICE_KERNEL_EXEC_TIMEOUT_NV:\t"+(device.getInfo(cl.DEVICE_KERNEL_EXEC_TIMEOUT_NV) == cl.TRUE ? "TRUE" : "FALSE"));
    log("  DEVICE_INTEGRATED_MEMORY_NV:\t"+(device.getInfo(cl.DEVICE_INTEGRATED_MEMORY_NV) == cl.TRUE ? "TRUE" : "FALSE"));
  }

  // DEVICE_PREFERRED_VECTOR_WIDTH_<type>
  log("  DEVICE_PREFERRED_VECTOR_WIDTH_<t>"); 
  var vec_width= [
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_CHAR),
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_SHORT),
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_INT),
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_LONG),
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT),
                  device.getInfo(cl.DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE),
                  ];
  log('\tCHAR '+vec_width[0]+', SHORT '+vec_width[1]+', INT '+vec_width[2]+', LONG '+vec_width[3]+
      ', FLOAT '+vec_width[4]+', DOUBLE '+vec_width[5]);

  log("  DEVICE_NATIVE_VECTOR_WIDTH_<t>"); 
  var vec_native_width= [
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_CHAR),
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_SHORT),
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_INT),
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_LONG),
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_FLOAT),
                         device.getInfo(cl.DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE),
                         ];
  log('\tCHAR '+vec_native_width[0]+', SHORT '+vec_native_width[1]+', INT '+vec_native_width[2]+
      ', LONG '+vec_native_width[3]+', FLOAT '+vec_native_width[4]+', DOUBLE '+vec_native_width[5]);
}

function NV_ConvertSMVer2Cores(major, minor)
{
  // Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
  //0xMm (hexadecimal notation), M = SM Major version, and m = SM minor version
  var nGpuArchCoresPerSM = [
                            {'SM': 0x10,  'Cores': 8 },
                            { 'SM': 0x11, 'Cores': 8 },
                            { 'SM': 0x12, 'Cores': 8 },
                            { 'SM': 0x13, 'Cores': 8 },
                            { 'SM': 0x20, 'Cores': 32 },
                            { 'SM': 0x21, 'Cores': 48 },
                            { 'SM':   -1, 'Cores': -1 }
                            ];

  var index = 0;
  while (nGpuArchCoresPerSM[index].SM != -1) {
    if (nGpuArchCoresPerSM[index].SM == ((major << 4) + minor) ) {
      return nGpuArchCoresPerSM[index].Cores;
    }
    index++;
  }
  log("MapSMtoCores undefined SMversion "+major+'.'+minor);
  return -1;
}

//Helper function to get OpenCL image format string (channel order and type) from constant
//*********************************************************************
function oclImageFormatString(uiImageFormat)
{
 // channel_order 
 if (uiImageFormat == cl.R)return "R";  
 if (uiImageFormat == cl.A)return "A";  
 if (uiImageFormat == cl.RG)return "RG";  
 if (uiImageFormat == cl.RA)return "RA";  
 if (uiImageFormat == cl.RGB)return "RGB";
 if (uiImageFormat == cl.RGBA)return "RGBA";  
 if (uiImageFormat == cl.BGRA)return "BGRA";  
 if (uiImageFormat == cl.ARGB)return "ARGB";  
 if (uiImageFormat == cl.INTENSITY)return "INTENSITY";  
 if (uiImageFormat == cl.LUMINANCE)return "LUMINANCE";  

 // channel_type 
 if (uiImageFormat == cl.SNORM_INT8)return "SNORM_INT8";
 if (uiImageFormat == cl.SNORM_INT16)return "SNORM_INT16";
 if (uiImageFormat == cl.UNORM_INT8)return "UNORM_INT8";
 if (uiImageFormat == cl.UNORM_INT16)return "UNORM_INT16";
 if (uiImageFormat == cl.UNORM_SHORT_565)return "UNORM_SHORT_565";
 if (uiImageFormat == cl.UNORM_SHORT_555)return "UNORM_SHORT_555";
 if (uiImageFormat == cl.UNORM_INT_101010)return "UNORM_INT_101010";
 if (uiImageFormat == cl.SIGNED_INT8)return "SIGNED_INT8";
 if (uiImageFormat == cl.SIGNED_INT16)return "SIGNED_INT16";
 if (uiImageFormat == cl.SIGNED_INT32)return "SIGNED_INT32";
 if (uiImageFormat == cl.UNSIGNED_INT8)return "UNSIGNED_INT8";
 if (uiImageFormat == cl.UNSIGNED_INT16)return "UNSIGNED_INT16";
 if (uiImageFormat == cl.UNSIGNED_INT32)return "UNSIGNED_INT32";
 if (uiImageFormat == cl.HALF_FLOAT)return "HALF_FLOAT";
 if (uiImageFormat == cl.FLOAT)return "FLOAT";

 // unknown constant
 return "Unknown";
}
