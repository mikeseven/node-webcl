/*
 * device.cc
 *
 *  Created on: Dec 19, 2011
 *      Author: ngk437
 */

#include "device.h"

#include <iostream>
using namespace std;

using namespace v8;
using namespace webcl;

namespace webcl {
Persistent<FunctionTemplate> Device::constructor_template;

void Device::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Device::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLDevice"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getExtension", getExtension);

  target->Set(String::NewSymbol("WebCLDevice"), constructor_template->GetFunction());
}

Device::Device(Handle<Object> wrapper) : device_id(0)
{
}

JS_METHOD(Device::getInfo)
{
  HandleScope scope;
  Device *device = UnwrapThis<Device>(args);
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
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_STR(param_value,param_value_size_ret));
  }
  break;
  case CL_DEVICE_PLATFORM: {
    cl_platform_id param_value;
    size_t param_value_size_ret=0;

    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_platform_id), &param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT((unsigned long)param_value));
  }
  break;
  case CL_DEVICE_TYPE: {
    cl_device_id param_value;
    size_t param_value_size_ret=0;
    cout<<"Device "<<device->device_id<<endl;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(cl_device_id), &param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT((unsigned long)param_value));
  }
  break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
    size_t param_value_size_ret=0;
    cl_int param_value[1024];
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, 1024*sizeof(cl_int), param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_PLATFORM);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    int n=param_value_size_ret;
    Local<Array> arr = Array::New(n);
    for(int i=0;i<n;i++)
      arr->Set(i,JS_INT(param_value[i]));

    return scope.Close(arr);
  }
  break;
  default: {
    size_t param_value=0;
    cl_int ret=::clGetDeviceInfo(device->device_id, param_name, sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  }
  return Undefined();
}

JS_METHOD(Device::getExtension)
{
  HandleScope scope;
  cl_device_info param_name = args[0]->Uint32Value();

  return Undefined();
}

JS_METHOD(Device::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  Device *cl = new Device(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
Device *Device::New(cl_device_id dw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Device *device = ObjectWrap::Unwrap<Device>(obj);
  device->device_id = dw;

  return device;
}

}
