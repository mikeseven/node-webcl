/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLPlatform.h"
#include "WebCLDevice.h"

#include <iostream>
using namespace std;

using namespace v8;
using namespace webcl;
using namespace cl;

Persistent<FunctionTemplate> WebCLDevice::constructor_template;

/* static  */
void WebCLDevice::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLDevice::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLDevice"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);

  target->Set(String::NewSymbol("WebCLDevice"), constructor_template->GetFunction());
}

WebCLDevice::WebCLDevice(Handle<Object> wrapper) : device(0)
{
}

WebCLDevice::~WebCLDevice()
{
  if (device) delete device;
}

/* static */
JS_METHOD(WebCLDevice::getInfo)
{
  HandleScope scope;
  WebCLDevice *device = ObjectWrap::Unwrap<WebCLDevice>(args.This());
  cl_device_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_DEVICE_NAME:
  case CL_DEVICE_VENDOR:
  case CL_DRIVER_VERSION:
  case CL_DEVICE_PROFILE:
  case CL_DEVICE_VERSION:
  case CL_DEVICE_OPENCL_C_VERSION:
  case CL_DEVICE_EXTENSIONS: {
    STRING_CLASS param_value;
    cl_int ret=device->getDevice()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(JS_STR(param_value.c_str(),param_value.length()));
  }
  break;
  case CL_DEVICE_PLATFORM: {
    cl_platform_id param_value=NULL;
    cl_int ret=device->getDevice()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT((ulong)param_value));
  }
  break;
  case CL_DEVICE_TYPE: {
    cl_device_id param_value=NULL;
    cl_int ret=device->getDevice()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT((ulong)param_value));
  }
  break;
  case CL_DEVICE_MAX_WORK_ITEM_SIZES: {
      VECTOR_CLASS<std::size_t> workitem_size;
      cl_int ret=device->getDevice()->getInfo(param_name,&workitem_size);
      if (ret != CL_SUCCESS) {
        REQ_ERROR_THROW(CL_INVALID_PLATFORM);
        REQ_ERROR_THROW(CL_INVALID_VALUE);
        REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
        return JS_EXCEPTION("UNKNOWN ERROR");
      }

      int n=workitem_size.size();
      Local<Array> arr = Array::New(n);
      for(int i=0;i<n;i++)
        arr->Set(i,JS_INT(workitem_size[i]));

      return scope.Close(arr);
  }
  break;
  default: {
    ::size_t param_value=0;
    cl_int ret=device->getDevice()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  }
}

/* static  */
JS_METHOD(WebCLDevice::New)
{
  HandleScope scope;
  WebCLDevice *cl = new WebCLDevice(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLDevice *WebCLDevice::New(cl::Device* dw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLDevice *device = ObjectWrap::Unwrap<WebCLDevice>(obj);
  device->device = dw;

  return device;
}
