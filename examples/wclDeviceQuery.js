/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var webcl=require("../lib/webcl"),
sys=require('util'),
clu=require('../lib/clUtils.js');

var WebCLPlatform=new webcl.WebCLPlatform();
var log=console.log;

//list of platforms
log("OpenCL SW Info:\n");
var platforms=webcl.getPlatformIDs();
console.log('Found '+platforms.length+' plaforms');
for(var i in platforms) {
  var p=platforms[i];
  //log("  Plaftorm ID: 0x"+p.getID().toString(16));
  log("  CL_PLATFORM_NAME: \t"+p.getInfo(webcl.CL_PLATFORM_NAME));
  log("  CL_PLATFORM_PROFILE: \t"+p.getInfo(webcl.CL_PLATFORM_PROFILE));
  log("  CL_PLATFORM_VERSION: \t"+p.getInfo(webcl.CL_PLATFORM_VERSION));
  log("  CL_PLATFORM_VENDOR: \t"+p.getInfo(webcl.CL_PLATFORM_VENDOR));
  log("  CL_PLATFORM_EXTENSIONS: \t"+p.getInfo(webcl.CL_PLATFORM_EXTENSIONS));

  //list for devices for a platform
  log("OpenCL Device Info:\n");
  var devices=p.getDevices(webcl.CL_DEVICE_TYPE_ALL);
  log('Found '+devices.length+' devices');
  for(i=0;i<devices.length;i++) {
    var device=devices[i];
    log(" ---------------------------------");
    log(' Device: '+device.getInfo(webcl.CL_DEVICE_NAME));
    log(" ---------------------------------");
    printDeviceInfo(device);
  }
  
  //Create a context for the devices
  var cxGPUContext = webcl.createContext(webcl.CL_DEVICE_TYPE_GPU, [webcl.CL_CONTEXT_PLATFORM, p]);
  var ctxDevices=cxGPUContext.getInfo(webcl.CL_CONTEXT_DEVICES);
  log("Found "+ctxDevices.length+" devices in context");
  for(var i=0;i<ctxDevices.length;i++)
    log("CL_CONTEXT_DEVICES\t\t"+ctxDevices[i]);
  
  
  // Determine and show image format support 
  // 2D
  ImageFormats=cxGPUContext.getSupportedImageFormats(webcl.CL_MEM_READ_ONLY, webcl.CL_MEM_OBJECT_IMAGE2D);
  uiNumSupportedFormats=ImageFormats.length;
  
  log("  ---------------------------------");
  log("  2D Image Formats Supported", uiNumSupportedFormats); 
  log("  ---------------------------------");
  log(clu.sprintf("%-6s%-16s%-22s\n", "#", "Channel Order", "Channel Type"));
  for(var i = 0; i < uiNumSupportedFormats; i++) 
  {  
    log(clu.sprintf("  %-6u%-16s%-22s", (i + 1),
        oclImageFormatString(ImageFormats[i].order), 
        oclImageFormatString(ImageFormats[i].data_type)));
  }
  log("");
  
  // 3D
  ImageFormats=cxGPUContext.getSupportedImageFormats(webcl.CL_MEM_READ_ONLY, webcl.CL_MEM_OBJECT_IMAGE3D);
  uiNumSupportedFormats=ImageFormats.length;
  
  log("  ---------------------------------");
  log("  3D Image Formats Supported", uiNumSupportedFormats); 
  log("  ---------------------------------");
  log(clu.sprintf("%-6s%-16s%-22s\n", "#", "Channel Order", "Channel Type"));
  for(var i = 0; i < uiNumSupportedFormats; i++) 
  {  
    log(clu.sprintf("  %-6u%-16s%-22s", (i + 1),
        oclImageFormatString(ImageFormats[i].order), 
        oclImageFormatString(ImageFormats[i].data_type)));
  }
  log(""); 
}

function printDeviceInfo(device)
{
  //log("  Device ID: \t\t\t0x"+device.getID().toString(16));
  log("  CL_DEVICE_NAME: \t\t\t"+device.getInfo(webcl.CL_DEVICE_NAME));
  log("  CL_DEVICE_VENDOR: \t\t\t"+device.getInfo(webcl.CL_DEVICE_VENDOR));
  log("  CL_DRIVER_VERSION: \t\t\t"+device.getInfo(webcl.CL_DRIVER_VERSION));
  log("  CL_DEVICE_VERSION: \t\t\t"+device.getInfo(webcl.CL_DEVICE_VERSION));
  log("  CL_DEVICE_PROFILE: \t\t\t"+device.getInfo(webcl.CL_DEVICE_PROFILE));
  log("  CL_DEVICE_PLATFORM: \t\t\t0x"+device.getInfo(webcl.CL_DEVICE_PLATFORM).toString(16));
  log("  CL_DEVICE_OPENCL_C_VERSION: \t\t"+device.getInfo(webcl.CL_DEVICE_OPENCL_C_VERSION));
  var type=parseInt(device.getInfo(webcl.CL_DEVICE_TYPE));
  if( type & webcl.CL_DEVICE_TYPE_CPU )
    log("  CL_DEVICE_TYPE:\t\t\tCL_DEVICE_TYPE_CPU");
  if( type & webcl.CL_DEVICE_TYPE_GPU )
    log("  CL_DEVICE_TYPE:\t\t\tCL_DEVICE_TYPE_GPU");
  if( type & webcl.CL_DEVICE_TYPE_ACCELERATOR )
    log("  CL_DEVICE_TYPE:\t\t\tCL_DEVICE_TYPE_ACCELERATOR");
  if( type & webcl.CL_DEVICE_TYPE_DEFAULT )
    log("  CL_DEVICE_TYPE:\t\t\tCL_DEVICE_TYPE_DEFAULT");

  var compute_units=0;
  log("  CL_DEVICE_MAX_COMPUTE_UNITS:\t\t"+(compute_units=device.getInfo(webcl.CL_DEVICE_MAX_COMPUTE_UNITS)));
  log("  CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:\t"+device.getInfo(webcl.CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS));

  var workitem_size=device.getInfo(webcl.CL_DEVICE_MAX_WORK_ITEM_SIZES);
  log("  CL_DEVICE_MAX_WORK_ITEM_SIZES:\t"+workitem_size[0]+" / "+workitem_size[1]+" / "+workitem_size[2]);

  log("  CL_DEVICE_MAX_WORK_GROUP_SIZE:\t"+device.getInfo(webcl.CL_DEVICE_MAX_WORK_GROUP_SIZE));
  log("  CL_DEVICE_MAX_CLOCK_FREQUENCY:\t"+device.getInfo(webcl.CL_DEVICE_MAX_CLOCK_FREQUENCY)+" MHz");
  log("  CL_DEVICE_ADDRESS_BITS:\t\t"+device.getInfo(webcl.CL_DEVICE_ADDRESS_BITS));
  log("  CL_DEVICE_MAX_MEM_ALLOC_SIZE:\t\t"+(device.getInfo(webcl.CL_DEVICE_MAX_MEM_ALLOC_SIZE) / (1024 * 1024))+" MBytes");
  log("  CL_DEVICE_GLOBAL_MEM_SIZE:\t\t"+(device.getInfo(webcl.CL_DEVICE_GLOBAL_MEM_SIZE) / (1024 * 1024))+" MBytes");
  log("  CL_DEVICE_ERROR_CORRECTION_SUPPORT:\t"+(device.getInfo(webcl.CL_DEVICE_ERROR_CORRECTION_SUPPORT) == webcl.CL_TRUE ? "yes" : "no"));
  log("  CL_DEVICE_LOCAL_MEM_TYPE:\t\t"+(device.getInfo(webcl.CL_DEVICE_LOCAL_MEM_TYPE) == 1 ? "local" : "global"));
  log("  CL_DEVICE_LOCAL_MEM_SIZE:\t\t"+(device.getInfo(webcl.CL_DEVICE_LOCAL_MEM_SIZE) / 1024)+" KBytes");
  log("  CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:\t"+(device.getInfo(webcl.CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE) / 1024)+" KBytes");

  // CL_DEVICE_QUEUE_PROPERTIES
  var queue_properties=device.getInfo(webcl.CL_DEVICE_QUEUE_PROPERTIES);
  if( queue_properties & webcl.CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE )
    log("  CL_DEVICE_QUEUE_PROPERTIES:\t\tCL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE");
  if( queue_properties & webcl.CL_QUEUE_PROFILING_ENABLE )
    log("  CL_DEVICE_QUEUE_PROPERTIES:\t\tCL_QUEUE_PROFILING_ENABLE");

  // image support
  log("  CL_DEVICE_IMAGE_SUPPORT:\t\t"+device.getInfo(webcl.CL_DEVICE_IMAGE_SUPPORT));
  log("  CL_DEVICE_MAX_READ_IMAGE_ARGS:\t"+device.getInfo(webcl.CL_DEVICE_MAX_READ_IMAGE_ARGS));
  log("  CL_DEVICE_MAX_WRITE_IMAGE_ARGS:\t"+device.getInfo(webcl.CL_DEVICE_MAX_WRITE_IMAGE_ARGS));

  // CL_DEVICE_SINGLE_FP_CONFIG
  var fp_config=device.getInfo(webcl.CL_DEVICE_SINGLE_FP_CONFIG);
  log("  CL_DEVICE_SINGLE_FP_CONFIG:\t\t"+
      (fp_config & webcl.CL_FP_DENORM ? "denorms " : "")+
      (fp_config & webcl.CL_FP_INF_NAN ? "INF-quietNaNs " : "")+
      (fp_config & webcl.CL_FP_ROUND_TO_NEAREST ? "round-to-nearest " : "")+
      (fp_config & webcl.CL_FP_ROUND_TO_ZERO ? "round-to-zero " : "")+
      (fp_config & webcl.CL_FP_ROUND_TO_INF ? "round-to-inf " : "")+
      (fp_config & webcl.CL_FP_FMA ? "fma " : ""));

  log("\n  CL_DEVICE_IMAGE <dim>"); 
  log("\t\t\t\t\t2D_MAX_WIDTH\t "+device.getInfo(webcl.CL_DEVICE_IMAGE2D_MAX_WIDTH));
  log("\t\t\t\t\t2D_MAX_HEIGHT\t "+device.getInfo(webcl.CL_DEVICE_IMAGE2D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_WIDTH\t "+device.getInfo(webcl.CL_DEVICE_IMAGE3D_MAX_WIDTH));
  log("\t\t\t\t\t3D_MAX_HEIGHT\t "+device.getInfo(webcl.CL_DEVICE_IMAGE3D_MAX_HEIGHT));
  log("\t\t\t\t\t3D_MAX_DEPTH\t "+device.getInfo(webcl.CL_DEVICE_IMAGE3D_MAX_DEPTH));

  // CL_DEVICE_EXTENSIONS: get device extensions, and if any then parse & log the string onto separate lines
  log("\n  CL_DEVICE_EXTENSIONS:");
  var device_string=device.getInfo(webcl.CL_DEVICE_EXTENSIONS);
  var nv_device_attibute_query=false;
  if (device_string.length > 0) 
  {
    var extensions=device_string.split(' ');
    for(var i=0;i<extensions.length;i++) {
      if("cl_nv_device_attribute_query"===extensions[i])
        nv_device_attibute_query=true;
      log("\t\t\t\t\t"+extensions[i]);
    }
  }
  else 
    log("  CL_DEVICE_EXTENSIONS: None");

  if(nv_device_attibute_query) 
  {
    var compute_capability_major=device.getInfo(webcl.CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV);
    var compute_capability_minor=device.getInfo(webcl.CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV);
    log("\n  CL_DEVICE_COMPUTE_CAPABILITY_NV:\t"+compute_capability_major+'.'+compute_capability_minor);
    log("  NUMBER OF MULTIPROCESSORS:\t\t"+compute_units); // this is the same value reported by CL_DEVICE_MAX_COMPUTE_UNITS
    log("  NUMBER OF CUDA CORES:\t\t\t"+NV_ConvertSMVer2Cores(compute_capability_major, compute_capability_minor) * compute_units);
    log("  CL_DEVICE_REGISTERS_PER_BLOCK_NV:\t"+device.getInfo(webcl.CL_DEVICE_REGISTERS_PER_BLOCK_NV));
    log("  CL_DEVICE_WARP_SIZE_NV:\t\t"+device.getInfo(webcl.CL_DEVICE_WARP_SIZE_NV));
    log("  CL_DEVICE_GPU_OVERLAP_NV:\t\t"+(device.getInfo(webcl.CL_DEVICE_GPU_OVERLAP_NV) == webcl.CL_TRUE ? "CL_TRUE" : "CL_FALSE"));
    log("  CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV:\t"+(device.getInfo(webcl.CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV) == webcl.CL_TRUE ? "CL_TRUE" : "CL_FALSE"));
    log("  CL_DEVICE_INTEGRATED_MEMORY_NV:\t"+(device.getInfo(webcl.CL_DEVICE_INTEGRATED_MEMORY_NV) == webcl.CL_TRUE ? "CL_TRUE" : "CL_FALSE"));
  }

  // CL_DEVICE_PREFERRED_VECTOR_WIDTH_<type>
  log("  CL_DEVICE_PREFERRED_VECTOR_WIDTH_<t>"); 
  var vec_width= [
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR),
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT),
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT),
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG),
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT),
                  device.getInfo(webcl.CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE),
                  ];
  log('\tCHAR '+vec_width[0]+', SHORT '+vec_width[1]+', INT '+vec_width[2]+', LONG '+vec_width[3]+
      ', FLOAT '+vec_width[4]+', DOUBLE '+vec_width[5]);

  log("  CL_DEVICE_NATIVE_VECTOR_WIDTH_<t>"); 
  var vec_native_width= [
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR),
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT),
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_INT),
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG),
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT),
                         device.getInfo(webcl.CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE),
                         ];
  log('\tCHAR '+vec_native_width[0]+', SHORT '+vec_native_width[1]+', INT '+vec_native_width[2]+
      ', LONG '+vec_native_width[3]+', FLOAT '+vec_native_width[4]+', DOUBLE '+vec_native_width[5]);
}

function NV_ConvertSMVer2Cores(major, minor)
{
  // Defines for GPU Architecture types (using the SM version to determine the # of cores per SM
  //0xMm (hexidecimal notation), M = SM Major version, and m = SM minor version
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
 // cl_channel_order 
 if (uiImageFormat == webcl.CL_R)return "CL_R";  
 if (uiImageFormat == webcl.CL_A)return "CL_A";  
 if (uiImageFormat == webcl.CL_RG)return "CL_RG";  
 if (uiImageFormat == webcl.CL_RA)return "CL_RA";  
 if (uiImageFormat == webcl.CL_RGB)return "CL_RGB";
 if (uiImageFormat == webcl.CL_RGBA)return "CL_RGBA";  
 if (uiImageFormat == webcl.CL_BGRA)return "CL_BGRA";  
 if (uiImageFormat == webcl.CL_ARGB)return "CL_ARGB";  
 if (uiImageFormat == webcl.CL_INTENSITY)return "CL_INTENSITY";  
 if (uiImageFormat == webcl.CL_LUMINANCE)return "CL_LUMINANCE";  

 // cl_channel_type 
 if (uiImageFormat == webcl.CL_SNORM_INT8)return "CL_SNORM_INT8";
 if (uiImageFormat == webcl.CL_SNORM_INT16)return "CL_SNORM_INT16";
 if (uiImageFormat == webcl.CL_UNORM_INT8)return "CL_UNORM_INT8";
 if (uiImageFormat == webcl.CL_UNORM_INT16)return "CL_UNORM_INT16";
 if (uiImageFormat == webcl.CL_UNORM_SHORT_565)return "CL_UNORM_SHORT_565";
 if (uiImageFormat == webcl.CL_UNORM_SHORT_555)return "CL_UNORM_SHORT_555";
 if (uiImageFormat == webcl.CL_UNORM_INT_101010)return "CL_UNORM_INT_101010";
 if (uiImageFormat == webcl.CL_SIGNED_INT8)return "CL_SIGNED_INT8";
 if (uiImageFormat == webcl.CL_SIGNED_INT16)return "CL_SIGNED_INT16";
 if (uiImageFormat == webcl.CL_SIGNED_INT32)return "CL_SIGNED_INT32";
 if (uiImageFormat == webcl.CL_UNSIGNED_INT8)return "CL_UNSIGNED_INT8";
 if (uiImageFormat == webcl.CL_UNSIGNED_INT16)return "CL_UNSIGNED_INT16";
 if (uiImageFormat == webcl.CL_UNSIGNED_INT32)return "CL_UNSIGNED_INT32";
 if (uiImageFormat == webcl.CL_HALF_FLOAT)return "CL_HALF_FLOAT";
 if (uiImageFormat == webcl.CL_FLOAT)return "CL_FLOAT";

 // unknown constant
 return "Unknown";
}
