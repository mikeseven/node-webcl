/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var cl = require("../build/Release/_webcl");

var webcl = new cl.WebCL();

exports.WebCL = webcl;
exports.WebCLCommandQueue = cl.WebCLCommandQueue;
exports.WebCLContext = cl.WebCLContext;
exports.WebCLDevice= cl.WebCLDevice;
exports.WebCLEvent = cl.WebCLEvent;
exports.WebCLKernel = cl.WebCLKernel;
exports.WebCLMemoryObject = cl.WebCLMemoryObject;
exports.WebCLPlatform = cl.WebCLPlatform;
exports.WebCLProgram = cl.WebCLProgram;
exports.WebCLSampler = cl.WebCLSampler;

// expose top level methods in WebCL object
exports.getPlatformIDs = function() { 
    return webcl.getPlatformIDs();
};

exports.createContext = function(a,b) { 
    return webcl.createContext(a,b);
};

exports.createContextFromType = function(a,b) { 
    return webcl.createContextFromType(a,b);
};

exports.waitForEvents = function() { 
    return webcl.waitForEvents(arguments);
};

exports.unloadCompiler = function() { 
    return webcl.unloadCompiler();
};

/* Error Codes */
exports.CL_SUCCESS                                  =0;
exports.CL_DEVICE_NOT_FOUND                         =-1;
exports.CL_DEVICE_NOT_AVAILABLE                     =-2;
exports.CL_COMPILER_NOT_AVAILABLE                   =-3;
exports.CL_MEM_OBJECT_ALLOCATION_FAILURE            =-4;
exports.CL_OUT_OF_RESOURCES                         =-5;
exports.CL_OUT_OF_HOST_MEMORY                       =-6;
exports.CL_PROFILING_INFO_NOT_AVAILABLE             =-7;
exports.CL_MEM_COPY_OVERLAP                         =-8;
exports.CL_IMAGE_FORMAT_MISMATCH                    =-9;
exports.CL_IMAGE_FORMAT_NOT_SUPPORTED               =-10;
exports.CL_BUILD_PROGRAM_FAILURE                    =-11;
exports.CL_MAP_FAILURE                              =-12;
exports.CL_MISALIGNED_SUB_BUFFER_OFFSET             -13;
exports.CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST =-14;

exports.CL_INVALID_VALUE                            =-30;
exports.CL_INVALID_DEVICE_TYPE                      =-31;
exports.CL_INVALID_PLATFORM                         =-32;
exports.CL_INVALID_DEVICE                           =-33;
exports.CL_INVALID_CONTEXT                          =-34;
exports.CL_INVALID_QUEUE_PROPERTIES                 =-35;
exports.CL_INVALID_COMMAND_QUEUE                    =-36;
exports.CL_INVALID_HOST_PTR                         =-37;
exports.CL_INVALID_MEM_OBJECT                       =-38;
exports.CL_INVALID_IMAGE_FORMAT_DESCRIPTOR          =-39;
exports.CL_INVALID_IMAGE_SIZE                       =-40;
exports.CL_INVALID_SAMPLER                          =-41;
exports.CL_INVALID_BINARY                           =-42;
exports.CL_INVALID_BUILD_OPTIONS                    =-43;
exports.CL_INVALID_PROGRAM                          =-44;
exports.CL_INVALID_PROGRAM_EXECUTABLE               =-45;
exports.CL_INVALID_KERNEL_NAME                      =-46;
exports.CL_INVALID_KERNEL_DEFINITION                =-47;
exports.CL_INVALID_KERNEL                           =-48;
exports.CL_INVALID_ARG_INDEX                        =-49;
exports.CL_INVALID_ARG_VALUE                        =-50;
exports.CL_INVALID_ARG_SIZE                         =-51;
exports.CL_INVALID_KERNEL_ARGS                      =-52;
exports.CL_INVALID_WORK_DIMENSION                   =-53;
exports.CL_INVALID_WORK_GROUP_SIZE                  =-54;
exports.CL_INVALID_WORK_ITEM_SIZE                   =-55;
exports.CL_INVALID_GLOBAL_OFFSET                    =-56;
exports.CL_INVALID_EVENT_WAIT_LIST                  =-57;
exports.CL_INVALID_EVENT                            =-58;
exports.CL_INVALID_OPERATION                        =-59;
exports.CL_INVALID_GL_OBJECT                        =-60;
exports.CL_INVALID_BUFFER_SIZE                      =-61;
exports.CL_INVALID_MIP_LEVEL                        =-62;
exports.CL_INVALID_GLOBAL_WORK_SIZE                 =-63;
exports.CL_INVALID_PROPERTY                         =-64;

/* OpenCL Version */
exports.CL_VERSION_1_0                              =1;
exports.CL_VERSION_1_1                              =1;

/* cl_bool */
exports.CL_FALSE                                    =0;
exports.CL_TRUE                                     =1;

/* cl_platform_info */
exports.CL_PLATFORM_PROFILE                         =0x0900;
exports.CL_PLATFORM_VERSION                         =0x0901;
exports.CL_PLATFORM_NAME                            =0x0902;
exports.CL_PLATFORM_VENDOR                          =0x0903;
exports.CL_PLATFORM_EXTENSIONS                      =0x0904;

/* cl_device_type - bitfield */
exports.CL_DEVICE_TYPE_DEFAULT                      =(1 << 0);
exports.CL_DEVICE_TYPE_CPU                          =(1 << 1);
exports.CL_DEVICE_TYPE_GPU                          =(1 << 2);
exports.CL_DEVICE_TYPE_ACCELERATOR                  =(1 << 3);
exports.CL_DEVICE_TYPE_ALL                          =0xFFFFFFFF;

/* cl_device_info */
exports.CL_DEVICE_TYPE                              =0x1000;
exports.CL_DEVICE_VENDOR_ID                         =0x1001;
exports.CL_DEVICE_MAX_COMPUTE_UNITS                 =0x1002;
exports.CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS          =0x1003;
exports.CL_DEVICE_MAX_WORK_GROUP_SIZE               =0x1004;
exports.CL_DEVICE_MAX_WORK_ITEM_SIZES               =0x1005;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR       =0x1006;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT      =0x1007;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT        =0x1008;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG       =0x1009;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT      =0x100A;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE     =0x100B;
exports.CL_DEVICE_MAX_CLOCK_FREQUENCY               =0x100C;
exports.CL_DEVICE_ADDRESS_BITS                      =0x100D;
exports.CL_DEVICE_MAX_READ_IMAGE_ARGS               =0x100E;
exports.CL_DEVICE_MAX_WRITE_IMAGE_ARGS              =0x100F;
exports.CL_DEVICE_MAX_MEM_ALLOC_SIZE                =0x1010;
exports.CL_DEVICE_IMAGE2D_MAX_WIDTH                 =0x1011;
exports.CL_DEVICE_IMAGE2D_MAX_HEIGHT                =0x1012;
exports.CL_DEVICE_IMAGE3D_MAX_WIDTH                 =0x1013;
exports.CL_DEVICE_IMAGE3D_MAX_HEIGHT                =0x1014;
exports.CL_DEVICE_IMAGE3D_MAX_DEPTH                 =0x1015;
exports.CL_DEVICE_IMAGE_SUPPORT                     =0x1016;
exports.CL_DEVICE_MAX_PARAMETER_SIZE                =0x1017;
exports.CL_DEVICE_MAX_SAMPLERS                      =0x1018;
exports.CL_DEVICE_MEM_BASE_ADDR_ALIGN               =0x1019;
exports.CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE          =0x101A;
exports.CL_DEVICE_SINGLE_FP_CONFIG                  =0x101B;
exports.CL_DEVICE_GLOBAL_MEM_CACHE_TYPE             =0x101C;
exports.CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE         =0x101D;
exports.CL_DEVICE_GLOBAL_MEM_CACHE_SIZE             =0x101E;
exports.CL_DEVICE_GLOBAL_MEM_SIZE                   =0x101F;
exports.CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE          =0x1020;
exports.CL_DEVICE_MAX_CONSTANT_ARGS                 =0x1021;
exports.CL_DEVICE_LOCAL_MEM_TYPE                    =0x1022;
exports.CL_DEVICE_LOCAL_MEM_SIZE                    =0x1023;
exports.CL_DEVICE_ERROR_CORRECTION_SUPPORT          =0x1024;
exports.CL_DEVICE_PROFILING_TIMER_RESOLUTION        =0x1025;
exports.CL_DEVICE_ENDIAN_LITTLE                     =0x1026;
exports.CL_DEVICE_AVAILABLE                         =0x1027;
exports.CL_DEVICE_COMPILER_AVAILABLE                =0x1028;
exports.CL_DEVICE_EXECUTION_CAPABILITIES            =0x1029;
exports.CL_DEVICE_QUEUE_PROPERTIES                  =0x102A;
exports.CL_DEVICE_NAME                              =0x102B;
exports.CL_DEVICE_VENDOR                            =0x102C;
exports.CL_DRIVER_VERSION                           =0x102D;
exports.CL_DEVICE_PROFILE                           =0x102E;
exports.CL_DEVICE_VERSION                           =0x102F;
exports.CL_DEVICE_EXTENSIONS                        =0x1030;
exports.CL_DEVICE_PLATFORM                          =0x1031;
exports.CL_DEVICE_DOUBLE_FP_CONFIG                  =0x1032;
exports.CL_DEVICE_HALF_FP_CONFIG                    =0x1033;
exports.CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF       =0x1034;
exports.CL_DEVICE_HOST_UNIFIED_MEMORY               =0x1035;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR          =0x1036;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT         =0x1037;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_INT           =0x1038;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG          =0x1039;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT         =0x103A;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE        =0x103B;
exports.CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF          =0x103C;
exports.CL_DEVICE_OPENCL_C_VERSION                  =0x103D;

/* cl_device_fp_config - bitfield */
exports.CL_FP_DENORM                                =(1 << 0);
exports.CL_FP_INF_NAN                               =(1 << 1);
exports.CL_FP_ROUND_TO_NEAREST                      =(1 << 2);
exports.CL_FP_ROUND_TO_ZERO                         =(1 << 3);
exports.CL_FP_ROUND_TO_INF                          =(1 << 4);
exports.CL_FP_FMA                                   =(1 << 5);
exports.CL_FP_SOFT_FLOAT                            =(1 << 6);

/* cl_device_mem_cache_type */
exports.CL_NONE                                     =0x0;
exports.CL_READ_ONLY_CACHE                          =0x1;
exports.CL_READ_WRITE_CACHE                         =0x2;

/* cl_device_local_mem_type */
exports.CL_LOCAL                                    =0x1;
exports.CL_GLOBAL                                   =0x2;

/* cl_device_exec_capabilities - bitfield */
exports.CL_EXEC_KERNEL                              =(1 << 0);
exports.CL_EXEC_NATIVE_KERNEL                       =(1 << 1);

/* cl_command_queue_properties - bitfield */
exports.CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE      =(1 << 0);
exports.CL_QUEUE_PROFILING_ENABLE                   =(1 << 1);

/* cl_context_info  */
exports.CL_CONTEXT_REFERENCE_COUNT                  =0x1080;
exports.CL_CONTEXT_DEVICES                          =0x1081;
exports.CL_CONTEXT_PROPERTIES                       =0x1082;
exports.CL_CONTEXT_NUM_DEVICES                      =0x1083;

/* cl_context_info + cl_context_properties */
exports.CL_CONTEXT_PLATFORM                         =0x1084;

/* cl_command_queue_info */
exports.CL_QUEUE_CONTEXT                            =0x1090;
exports.CL_QUEUE_DEVICE                             =0x1091;
exports.CL_QUEUE_REFERENCE_COUNT                    =0x1092;
exports.CL_QUEUE_PROPERTIES                         =0x1093;

/* cl_mem_flags - bitfield */
exports.CL_MEM_READ_WRITE                           =(1 << 0);
exports.CL_MEM_WRITE_ONLY                           =(1 << 1);
exports.CL_MEM_READ_ONLY                            =(1 << 2);
exports.CL_MEM_USE_HOST_PTR                         =(1 << 3);
exports.CL_MEM_ALLOC_HOST_PTR                       =(1 << 4);
exports.CL_MEM_COPY_HOST_PTR                        =(1 << 5);

/* cl_channel_order */
exports.CL_R                                        =0x10B0;
exports.CL_A                                        =0x10B1;
exports.CL_RG                                       =0x10B2;
exports.CL_RA                                       =0x10B3;
exports.CL_RGB                                      =0x10B4;
exports.CL_RGBA                                     =0x10B5;
exports.CL_BGRA                                     =0x10B6;
exports.CL_ARGB                                     =0x10B7;
exports.CL_INTENSITY                                =0x10B8;
exports.CL_LUMINANCE                                =0x10B9;
exports.CL_Rx                                       =0x10BA;
exports.CL_RGx                                      =0x10BB;
exports.CL_RGBx                                     =0x10BC;

/* cl_channel_type */
exports.CL_SNORM_INT8                               =0x10D0;
exports.CL_SNORM_INT16                              =0x10D1;
exports.CL_UNORM_INT8                               =0x10D2;
exports.CL_UNORM_INT16                              =0x10D3;
exports.CL_UNORM_SHORT_565                          =0x10D4;
exports.CL_UNORM_SHORT_555                          =0x10D5;
exports.CL_UNORM_INT_101010                         =0x10D6;
exports.CL_SIGNED_INT8                              =0x10D7;
exports.CL_SIGNED_INT16                             =0x10D8;
exports.CL_SIGNED_INT32                             =0x10D9;
exports.CL_UNSIGNED_INT8                            =0x10DA;
exports.CL_UNSIGNED_INT16                           =0x10DB;
exports.CL_UNSIGNED_INT32                           =0x10DC;
exports.CL_HALF_FLOAT                               =0x10DD;
exports.CL_FLOAT                                    =0x10DE;

/* cl_mem_object_type */
exports.CL_MEM_OBJECT_BUFFER                        =0x10F0;
exports.CL_MEM_OBJECT_IMAGE2D                       =0x10F1;
exports.CL_MEM_OBJECT_IMAGE3D                       =0x10F2;

/* cl_mem_info */
exports.CL_MEM_TYPE                                 =0x1100;
exports.CL_MEM_FLAGS                                =0x1101;
exports.CL_MEM_SIZE                                 =0x1102;
exports.CL_MEM_HOST_PTR                             =0x1103;
exports.CL_MEM_MAP_COUNT                            =0x1104;
exports.CL_MEM_REFERENCE_COUNT                      =0x1105;
exports.CL_MEM_CONTEXT                              =0x1106;
exports.CL_MEM_ASSOCIATED_MEMOBJECT                 =0x1107;
exports.CL_MEM_OFFSET                               =0x1108;

/* cl_image_info */
exports.CL_IMAGE_FORMAT                             =0x1110;
exports.CL_IMAGE_ELEMENT_SIZE                       =0x1111;
exports.CL_IMAGE_ROW_PITCH                          =0x1112;
exports.CL_IMAGE_SLICE_PITCH                        =0x1113;
exports.CL_IMAGE_WIDTH                              =0x1114;
exports.CL_IMAGE_HEIGHT                             =0x1115;
exports.CL_IMAGE_DEPTH                              =0x1116;

/* cl_addressing_mode */
exports.CL_ADDRESS_NONE                             =0x1130;
exports.CL_ADDRESS_CLAMP_TO_EDGE                    =0x1131;
exports.CL_ADDRESS_CLAMP                            =0x1132;
exports.CL_ADDRESS_REPEAT                           =0x1133;
exports.CL_ADDRESS_MIRRORED_REPEAT                  =0x1134;

/* cl_filter_mode */
exports.CL_FILTER_NEAREST                           =0x1140;
exports.CL_FILTER_LINEAR                            =0x1141;

/* cl_sampler_info */
exports.CL_SAMPLER_REFERENCE_COUNT                  =0x1150;
exports.CL_SAMPLER_CONTEXT                          =0x1151;
exports.CL_SAMPLER_NORMALIZED_COORDS                =0x1152;
exports.CL_SAMPLER_ADDRESSING_MODE                  =0x1153;
exports.CL_SAMPLER_FILTER_MODE                      =0x1154;

/* cl_map_flags - bitfield */
exports.CL_MAP_READ                                 =(1 << 0);
exports.CL_MAP_WRITE                                =(1 << 1);

/* cl_program_info */
exports.CL_PROGRAM_REFERENCE_COUNT                  =0x1160;
exports.CL_PROGRAM_CONTEXT                          =0x1161;
exports.CL_PROGRAM_NUM_DEVICES                      =0x1162;
exports.CL_PROGRAM_DEVICES                          =0x1163;
exports.CL_PROGRAM_SOURCE                           =0x1164;
exports.CL_PROGRAM_BINARY_SIZES                     =0x1165;
exports.CL_PROGRAM_BINARIES                         =0x1166;

/* cl_program_build_info */
exports.CL_PROGRAM_BUILD_STATUS                     =0x1181;
exports.CL_PROGRAM_BUILD_OPTIONS                    =0x1182;
exports.CL_PROGRAM_BUILD_LOG                        =0x1183;

/* cl_build_status */
exports.CL_BUILD_SUCCESS                             =0;
exports.CL_BUILD_NONE                                =1;
exports.CL_BUILD_ERROR                              =-2;
exports.CL_BUILD_IN_PROGRESS                        =-3;

/* cl_kernel_info */
exports.CL_KERNEL_FUNCTION_NAME                     =0x1190;
exports.CL_KERNEL_NUM_ARGS                          =0x1191;
exports.CL_KERNEL_REFERENCE_COUNT                   =0x1192;
exports.CL_KERNEL_CONTEXT                           =0x1193;
exports.CL_KERNEL_PROGRAM                           =0x1194;

/* cl_kernel_work_group_info */
exports.CL_KERNEL_WORK_GROUP_SIZE                   =0x11B0;
exports.CL_KERNEL_COMPILE_WORK_GROUP_SIZE           =0x11B1;
exports.CL_KERNEL_LOCAL_MEM_SIZE                    =0x11B2;
exports.CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE =0x11B3;
exports.CL_KERNEL_PRIVATE_MEM_SIZE                  =0x11B4;

/* cl_event_info  */
exports.CL_EVENT_COMMAND_QUEUE                      =0x11D0;
exports.CL_EVENT_COMMAND_TYPE                       =0x11D1;
exports.CL_EVENT_REFERENCE_COUNT                    =0x11D2;
exports.CL_EVENT_COMMAND_EXECUTION_STATUS           =0x11D3;
exports.CL_EVENT_CONTEXT                            =0x11D4;

/* cl_command_type */
exports.CL_COMMAND_NDRANGE_KERNEL                   =0x11F0;
exports.CL_COMMAND_TASK                             =0x11F1;
exports.CL_COMMAND_NATIVE_KERNEL                    =0x11F2;
exports.CL_COMMAND_READ_BUFFER                      =0x11F3;
exports.CL_COMMAND_WRITE_BUFFER                     =0x11F4;
exports.CL_COMMAND_COPY_BUFFER                      =0x11F5;
exports.CL_COMMAND_READ_IMAGE                       =0x11F6;
exports.CL_COMMAND_WRITE_IMAGE                      =0x11F7;
exports.CL_COMMAND_COPY_IMAGE                       =0x11F8;
exports.CL_COMMAND_COPY_IMAGE_TO_BUFFER             =0x11F9;
exports.CL_COMMAND_COPY_BUFFER_TO_IMAGE             =0x11FA;
exports.CL_COMMAND_MAP_BUFFER                       =0x11FB;
exports.CL_COMMAND_MAP_IMAGE                        =0x11FC;
exports.CL_COMMAND_UNMAP_MEM_OBJECT                 =0x11FD;
exports.CL_COMMAND_MARKER                           =0x11FE;
exports.CL_COMMAND_ACQUIRE_GL_OBJECTS               =0x11FF;
exports.CL_COMMAND_RELEASE_GL_OBJECTS               =0x1200;
exports.CL_COMMAND_READ_BUFFER_RECT                 =0x1201;
exports.CL_COMMAND_WRITE_BUFFER_RECT                =0x1202;
exports.CL_COMMAND_COPY_BUFFER_RECT                 =0x1203;
exports.CL_COMMAND_USER                             =0x1204;

/* command execution status */
exports.CL_COMPLETE                                 =0x0;
exports.CL_RUNNING                                  =0x1;
exports.CL_SUBMITTED                                =0x2;
exports.CL_QUEUED                                   =0x3;
  
/* cl_buffer_create_type  */
exports.CL_BUFFER_CREATE_TYPE_REGION                =0x1220;

/* cl_profiling_info  */
exports.CL_PROFILING_COMMAND_QUEUED                 =0x1280;
exports.CL_PROFILING_COMMAND_SUBMIT                 =0x1281;
exports.CL_PROFILING_COMMAND_START                  =0x1282;
exports.CL_PROFILING_COMMAND_END                    =0x1283;

/*
 * cl_ext.h
 */
/* cl_khr_fp64 extension - no extension exports.since it has no functions  */
exports.CL_DEVICE_DOUBLE_FP_CONFIG                  =0x1032;

/* cl_khr_fp16 extension - no extension exports.since it has no functions  */
exports.CL_DEVICE_HALF_FP_CONFIG                    =0x1033;

/************************ 
* cl_khr_icd extension *                                                  
************************/
/* cl_platform_info                                                        */
exports.CL_PLATFORM_ICD_SUFFIX_KHR                  =0x0920;

/* Additional Error Codes                                                  */
exports.CL_PLATFORM_NOT_FOUND_KHR                   =-1001;

/******************************************
* cl_nv_device_attribute_query extension *
******************************************/
/* cl_nv_device_attribute_query extension - no extension exports.since it has no functions */
exports.CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV       =0x4000;
exports.CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV       =0x4001;
exports.CL_DEVICE_REGISTERS_PER_BLOCK_NV            =0x4002;
exports.CL_DEVICE_WARP_SIZE_NV                      =0x4003;
exports.CL_DEVICE_GPU_OVERLAP_NV                    =0x4004;
exports.CL_DEVICE_KERNEL_EXEC_TIMEOUT_NV            =0x4005;
exports.CL_DEVICE_INTEGRATED_MEMORY_NV              =0x4006;

/*********************************
* cl_amd_device_attribute_query *
*********************************/
exports.CL_DEVICE_PROFILING_TIMER_OFFSET_AMD        =0x4036;

/* cl_device_partition_property_ext */
exports.CL_DEVICE_PARTITION_EQUALLY_EXT             =0x4050;
exports.CL_DEVICE_PARTITION_BY_COUNTS_EXT           =0x4051;
exports.CL_DEVICE_PARTITION_BY_NAMES_EXT            =0x4052;
exports.CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN_EXT  =0x4053;

/* clDeviceGetInfo selectors */
exports.CL_DEVICE_PARENT_DEVICE_EXT                 =0x4054;
exports.CL_DEVICE_PARTITION_TYPES_EXT               =0x4055;
exports.CL_DEVICE_AFFINITY_DOMAINS_EXT              =0x4056;
exports.CL_DEVICE_REFERENCE_COUNT_EXT               =0x4057;
exports.CL_DEVICE_PARTITION_STYLE_EXT               =0x4058;

/* error codes */
exports.CL_DEVICE_PARTITION_FAILED_EXT              =-1057;
exports.CL_INVALID_PARTITION_COUNT_EXT              =-1058;
exports.CL_INVALID_PARTITION_NAME_EXT               =-1059;

/* CL_AFFINITY_DOMAINs */
exports.CL_AFFINITY_DOMAIN_L1_CACHE_EXT             =0x1;
exports.CL_AFFINITY_DOMAIN_L2_CACHE_EXT             =0x2;
exports.CL_AFFINITY_DOMAIN_L3_CACHE_EXT             =0x3;
exports.CL_AFFINITY_DOMAIN_L4_CACHE_EXT             =0x4;
exports.CL_AFFINITY_DOMAIN_NUMA_EXT                 =0x10;
exports.CL_AFFINITY_DOMAIN_NEXT_FISSIONABLE_EXT     =0x100;

/* cl_device_partition_property_ext list terminators */
exports.CL_PROPERTIES_LIST_END_EXT                  =0;
exports.CL_PARTITION_BY_COUNTS_LIST_END_EXT         =0;
exports.CL_PARTITION_BY_NAMES_LIST_END_EXT          =-1;

/*********************************
* cl_amd_device_attribute_query *
*********************************/
/* cl_gl_object_type */
exports.CL_GL_OBJECT_BUFFER             =0x2000;
exports.CL_GL_OBJECT_TEXTURE2D          =0x2001;
exports.CL_GL_OBJECT_TEXTURE3D          =0x2002;
exports.CL_GL_OBJECT_RENDERBUFFER       =0x2003;

/* cl_gl_texture_info */
exports.CL_GL_TEXTURE_TARGET            =0x2004;
exports.CL_GL_MIPMAP_LEVEL              =0x2005;

/* Additional Error Codes  */
exports.CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR  =-1000;

/* cl_gl_context_info  */
exports.CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR    =0x2006;
exports.CL_DEVICES_FOR_GL_CONTEXT_KHR           =0x2007;

/* Additional cl_context_properties  */
exports.CL_GL_CONTEXT_KHR                       =0x2008;
exports.CL_EGL_DISPLAY_KHR                      =0x2009;
exports.CL_GLX_DISPLAY_KHR                      =0x200A;
exports.CL_WGL_HDC_KHR                          =0x200B;
exports.CL_CGL_SHAREGROUP_KHR                   =0x200C;

/* 
 *  cl_khr_gl_event  extension
 *  See section 9.9 in the OpenCL 1.1 spec for more information
 */
exports.CL_COMMAND_GL_FENCE_SYNC_OBJECT_KHR     =0x200D;

exports.types = {
};

var num = 0;
exports.types.UNKNOWN = num++;

exports.types.BYTE = num++;
exports.types.CHAR = num++;
exports.types.UCHAR = num++;
exports.types.SHORT = num++;
exports.types.USHORT = num++;

    // Basic types
exports.types.INT = num++;                        // cl_int
exports.types.UINT = num++;                       // cl_uint
exports.types.LONG = num++;                       // cl_long
exports.types.ULONG = num++;                      // cl_ulong
exports.types.BOOL = num++;                       // cl_bool = cl_uint                    //10
exports.types.SIZE_T = num++;                     // size_t
exports.types.HALF = num++;                       // cl_half
exports.types.FLOAT = num++;                      // cl_float
exports.types.DOUBLE = num++;                     // cl_double

// String types
exports.types.STRING = num++;                     // char*

    // Class types
exports.types.PLATFORM = num++;                   // cl_platform_id
exports.types.DEVICE = num++;                     // cl_device_id
exports.types.CONTEXT = num++;                    // cl_context
exports.types.COMMAND_QUEUE = num++;              // cl_command_queue
exports.types.MEMORY_OBJECT = num++;              // cl_mem                               //20
exports.types.MEM = exports.types.MEMORY_OBJECT;
exports.types.PROGRAM = num++;                    // cl_program
exports.types.KERNEL = num++;                     // cl_kernel
exports.types.EVENT = num++;                      // cl_event
exports.types.SAMPLER = num++;                    // cl_sampler

exports.types.IMAGE_FORMAT = num++;

    // Special types
exports.types.ADRESSING_MODE = num++;             // cl_addressing_mode
exports.types.BUILD_STATUS = num++;               // cl_build_status
exports.types.CHANNEL_ORDER = num++;              // cl_channel_order
exports.types.CHANNEL_TYPE = num++;               // cl_channel_type
exports.types.COMMAND_QUEUE_PROPERTIES = num++;   // cl_command_queue_properties          //30
exports.types.COMMAND_TYPE = num++;               // cl_command_type
exports.types.CONTEXT_PROPERTIES = num++;         // cl_context_properties

exports.types.DEVICE_EXEC_CAPABILITIES = num++;   // cl_device_exec_capabilities
exports.types.DEVICE_FP_CONFIG = num++;           // cl_device_fp_config
exports.types.DEVICE_LOCAL_MEM_TYPE = num++;      // cl_device_local_mem_type
exports.types.DEVICE_MEM_CACHE_TYPE = num++;      // cl_device_mem_cache_type
exports.types.DEVICE_TYPE = num++;                // cl_device_type
exports.types.FILTER_MODE = num++;                // cl_filter_mode
exports.types.WGL_OBJECT_TYPE = num++;            // cl_gl_object_type
exports.types.MAP_FLAGS = num++;                  // cl_map_flags                         //40
exports.types.MEM_FENCE_FLAGS = num++;            // cl_mem_fence_flags
exports.types.MEM_FLAGS = num++;                  // cl_mem_flags
exports.types.MEM_OBJECT_TYPE = num++;            // cl_mem_object_type

    // Vector types
exports.types.BYTE_V = num++;
exports.types.CHAR_V = num++;
exports.types.UCHAR_V = num++;
exports.types.SHORT_V = num++;
exports.types.USHORT_V = num++;
exports.types.INT_V = num++;                      // cl_int*
exports.types.UINT_V = num++;                     // cl_uint*                             //50
exports.types.LONG_V = num++;                     // cl_long*
exports.types.ULONG_V = num++;                    // cl_ulong*
exports.types.BOOL_V = num++;                     // cl_bool*
exports.types.SIZE_T_V = num++;                   // size_t*
exports.types.HALF_V = num++;
exports.types.FLOAT_V = num++;
exports.types.DOUBLE_V = num++;
exports.types.STRING_V = num++;                   // char**

exports.types.PLATFORM_V = num++;
exports.types.DEVICE_V = num++;                                                           //60
exports.types.CONTEXT_V = num++;
exports.types.COMMAND_QUEUE_V = num++;
exports.types.MEMORY_OBJECT_V = num++;
exports.types.PROGRAM_V = num++;
exports.types.KERNEL_V = num++;
exports.types.EVENT_V = num++;
exports.types.SAMPLER_V = num++;

exports.types.LAST = num;
