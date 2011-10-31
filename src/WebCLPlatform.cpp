/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "common.h"
#include "WebCLPlatform.h"
#include "WebCLDevice.h"

#include <iostream>
using namespace std;

using namespace v8;
using namespace webcl;
using namespace cl;

Persistent<FunctionTemplate> WebCLPlatform::constructor_template;

/* static  */
void WebCLPlatform::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLPlatform::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLPlatform"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getDevices", getDevices);

  target->Set(String::NewSymbol("WebCLPlatform"), constructor_template->GetFunction());
}

WebCLPlatform::WebCLPlatform(Handle<Object> wrapper) : platform(0)
{
}

WebCLPlatform::~WebCLPlatform()
{
  if (platform) delete platform;
}

/* static */
JS_METHOD(WebCLPlatform::getDevices)
{
  HandleScope scope;

  WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(args.This());
  cl_device_type device_type = args[0]->Uint32Value();

  VECTOR_CLASS<cl::Device> devices;
  cl_int ret=platform->getPlatform()->getDevices(device_type, &devices);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  Local<Array> deviceArray = Array::New(devices.size());
  for (int i=0; i<devices.size(); i++) {
    cl::Device *device=new cl::Device(devices[i]);
    deviceArray->Set(i, WebCLDevice::New(device)->handle_);
  }

  return scope.Close(deviceArray);
}

/* static */
JS_METHOD(WebCLPlatform::getInfo)
{
  HandleScope scope;
  WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(args.This());
  cl_platform_info param_name = args[0]->Uint32Value();
  STRING_CLASS param_value;
  cl_int ret=platform->getPlatform()->getInfo(param_name,&param_value);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(String::New(param_value.c_str(),param_value.length()));
}

/* static  */
JS_METHOD(WebCLPlatform::New)
{
  HandleScope scope;
  WebCLPlatform *cl = new WebCLPlatform(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLPlatform *WebCLPlatform::New(cl::Platform* pw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(obj);
  platform->platform = pw;

  return platform;
}
