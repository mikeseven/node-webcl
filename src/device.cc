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

#include "device.h"
#include "platform.h"

#include <cstring>

using namespace v8;
using namespace std;
using namespace webcl;

namespace webcl {
Persistent<FunctionTemplate> Device::constructor_template;

void Device::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Device::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLDevice"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getSupportedExtensions", getSupportedExtensions);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enableExtension", enableExtension);

  target->Set(NanSymbol("WebCLDevice"), ctor->GetFunction());
}

Device::Device(Handle<Object> wrapper) : device_id(0), enableExtensions(NONE), availableExtensions(NONE)
{
  _type=CLObjType::Device;
}

NAN_METHOD(Device::getInfo)
{
  NanScope();
  Device *device = ObjectWrap::Unwrap<Device>(args.This());
  cl_device_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_DEVICE_NAME:
  case CL_DEVICE_VENDOR:
  case CL_DRIVER_VERSION:
  case CL_DEVICE_PROFILE:
  case CL_DEVICE_VERSION:
  case CL_DEVICE_OPENCL_C_VERSION:
  case CL_DEVICE_EXTENSIONS: {
    char param_value[1024];
    size_t param_value_size_ret=0;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(char)*1024, param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    // NOTE: Adjust length because API returns NULL terminated string
    NanReturnValue(JS_STR(param_value,(int)param_value_size_ret - 1));
  }
  break;
  case CL_DEVICE_PLATFORM: {
    cl_platform_id param_value;

    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_platform_id), &param_value, NULL);

    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    if(param_value) {
      WebCLObject *obj=findCLObj((void*)param_value, CLObjType::Platform);
      if(obj) {
        NanReturnValue(NanObjectWrapHandle(obj));
      }
      else
        NanReturnValue(NanObjectWrapHandle(Platform::New(param_value)));
    }
    NanReturnUndefined();
  }
  break;
  case CL_DEVICE_TYPE: {
    cl_device_type param_value;
    #ifdef LOGGING
    cout<<"Device "<<device->device_id<<endl;
    #endif
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_type), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned((unsigned long)param_value));
  }
  break;
  case CL_DEVICE_LOCAL_MEM_TYPE: {
    cl_device_local_mem_type param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_local_mem_type), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::New(param_value));
  }
  break;
  case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: {
    cl_device_mem_cache_type param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_mem_cache_type), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::New(param_value));
  }
  break;
  case CL_DEVICE_EXECUTION_CAPABILITIES: {
    cl_device_exec_capabilities param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_exec_capabilities), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned((int)param_value));
  }
  break;
  case CL_DEVICE_QUEUE_PROPERTIES: {
    cl_command_queue_properties param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_command_queue_properties), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned((int)param_value));
  }
  break;
  case CL_DEVICE_HALF_FP_CONFIG:
  case CL_DEVICE_SINGLE_FP_CONFIG:
  case CL_DEVICE_DOUBLE_FP_CONFIG: {
    cl_device_fp_config param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_fp_config), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned((int)param_value));
  }
  break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
    cl_int ret;

    // get CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS param
    cl_uint max_work_item_dimensions;
    ret=::clGetDeviceInfo(device->device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(size_t), &max_work_item_dimensions, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_PLATFORM);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    // get CL_DEVICE_MAX_WORK_ITEM_SIZES array param
    size_t *param_value=new size_t[max_work_item_dimensions];
    ret=::clGetDeviceInfo(device->device_id, param_name, max_work_item_dimensions*sizeof(size_t), param_value, NULL);
    if (ret != CL_SUCCESS) {
		delete[] param_value;
      REQ_ERROR_THROW(INVALID_PLATFORM);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    Local<Array> arr = Array::New(max_work_item_dimensions);
    for(cl_uint i=0;i<max_work_item_dimensions;i++)
      arr->Set(i,JS_INT(param_value[i]));

	delete[] param_value;
	NanReturnValue(arr);
  }
  break;
  // cl_bool params
  case CL_DEVICE_AVAILABLE:
  case CL_DEVICE_COMPILER_AVAILABLE:
  case CL_DEVICE_ENDIAN_LITTLE:
  case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
  case CL_DEVICE_HOST_UNIFIED_MEMORY:
  case CL_DEVICE_IMAGE_SUPPORT:
  {
    cl_bool param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_bool), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    // keeping as Integer vs Boolean so comparisons with cl.TRUE/cl.FALSE work
    NanReturnValue(JS_BOOL((int)param_value));
  }
  break;
  // cl_uint params
  case CL_DEVICE_ADDRESS_BITS:
  case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
  case CL_DEVICE_MAX_CLOCK_FREQUENCY:
  case CL_DEVICE_MAX_COMPUTE_UNITS:
  case CL_DEVICE_MAX_CONSTANT_ARGS:
  case CL_DEVICE_MAX_READ_IMAGE_ARGS:
  case CL_DEVICE_MAX_SAMPLERS:
  case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
  case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
  case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
  case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
  case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
  case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
  case CL_DEVICE_VENDOR_ID: 
  
  // OpenCL 1.2 constants
  //case CL_DEVICE_REFERENCE_COUNT:
  //case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
  {
    cl_uint param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned(param_value));
  }
  break;
  // cl_ulong params
  case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
  case CL_DEVICE_GLOBAL_MEM_SIZE:
  case CL_DEVICE_LOCAL_MEM_SIZE:
  case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
  case CL_DEVICE_MAX_MEM_ALLOC_SIZE: {
    cl_ulong param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_ulong), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    // FIXME: handle uint64 somehow
    // JS only supports doubles, v8 has ints, CL params can be uint64
    // the memory params can certainly overflow uint32 size
    NanReturnValue(Integer::NewFromUnsigned((unsigned int)param_value));
  }
  break;
  // size_t params
  case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
  case CL_DEVICE_IMAGE2D_MAX_WIDTH:
  case CL_DEVICE_IMAGE3D_MAX_DEPTH:
  case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
  case CL_DEVICE_IMAGE3D_MAX_WIDTH:
  case CL_DEVICE_MAX_PARAMETER_SIZE:
  case CL_DEVICE_MAX_WORK_GROUP_SIZE:
  case CL_DEVICE_PROFILING_TIMER_RESOLUTION: 
  
  // OpenCL 1.2 constants
  //case CL_DEVICE_IMAGE_MAX_BUFFER_SIZE:
  //case CL_DEVICE_IMAGE_MAX_ARRAY_SIZE:
  {
    size_t param_value;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    // FIXME: handle 64 bit size_t somehow
    // assume for these params it will fit in an int
    NanReturnValue(Integer::New((int)param_value));
  }
  break;
  default: {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }
  }
  NanReturnUndefined();
}

NAN_METHOD(Device::enableExtension)
{
  NanScope();
  Device *device = ObjectWrap::Unwrap<Device>(args.This());
  if(!args[0]->IsString())
    return NanThrowTypeError("invalid extension name");

  if(device->availableExtensions==NONE) {
    char param_value[1024];
    size_t param_value_size_ret=0;

    cl_int ret=clGetDeviceInfo(device->device_id, CL_DEVICE_EXTENSIONS, 1024, param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    if(strcasestr(param_value,"gl_sharing")) device->availableExtensions |= GL_SHARING;
    if(strcasestr(param_value,"fp16"))       device->availableExtensions |= FP16;
    if(strcasestr(param_value,"fp64"))       device->availableExtensions |= FP64;
  }

  Local<String> name=args[0]->ToString();
  String::AsciiValue astr(name);
  bool ret=false;
  if(strcasestr(*astr,"gl_sharing") && (device->availableExtensions & GL_SHARING)) { device->enableExtensions |= GL_SHARING; ret=true; }
  else if(strcasestr(*astr,"fp16") && (device->availableExtensions & FP16))        { device->enableExtensions |= FP16;; ret=true; }
  else if(strcasestr(*astr,"fp64") && (device->availableExtensions & FP64))        { device->enableExtensions |= FP64;; ret=true; }

  NanReturnValue(JS_BOOL(ret));
}

NAN_METHOD(Device::getSupportedExtensions)
{
  NanScope();
  Device *device = ObjectWrap::Unwrap<Device>(args.This());
  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=clGetDeviceInfo(device->device_id, CL_DEVICE_EXTENSIONS, 1024, param_value, &param_value_size_ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_DEVICE);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(strstr(param_value,"gl_sharing")) device->availableExtensions |= GL_SHARING;
  if(strstr(param_value,"fp16"))       device->availableExtensions |= FP16;
  if(strstr(param_value,"fp64"))       device->availableExtensions |= FP64;

  NanReturnValue(JS_STR(param_value));
}

NAN_METHOD(Device::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();
  Device *cl = new Device(args.This());
  cl->Wrap(args.This());
  NanReturnValue(args.This());
}

/* static  */
Device *Device::New(cl_device_id dw)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Device *device = ObjectWrap::Unwrap<Device>(obj);
  device->device_id = dw;
  registerCLObj(dw, device);

  return device;
}

}
