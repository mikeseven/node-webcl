/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
** 
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

var ctypes={
    //bool,
    char,
    uchar,
    short,
    int,
    uint,
    long,
    ulong,
    float,
    double,
    //half,
    //size,
    
    // openCL specific
    local,
    mem,
    sampler,
    image2d,
    image3d,
    //event,
    
    // vector types
    char2, uchar2, short2, ushort2, int2, uint2, long2, ulong2, float2, // xy
    char3, uchar3, short3, ushort3, int3, uint3, long3, ulong3, float3, // xyz
    char4, uchar4, short4, ushort4, int4, uint4, long4, ulong4, float4, // xyzw
    double2, half2,
    double3, half3,
    double4, half4,
};


module.exports = ctypes;