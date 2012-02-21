/*
 * platform.cc
 *
 *  Created on: Dec 18, 2011
 *      Author: ngk437
 */

#include "platform.h"
#include "device.h"

#include <iostream>
#include <cstring>

using namespace v8;
using namespace std;

namespace webcl {

Persistent<FunctionTemplate> Platform::constructor_template;

void Platform::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Platform::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLPlatform"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getDevices", getDevices);

  target->Set(String::NewSymbol("WebCLPlatform"), constructor_template->GetFunction());
}

Platform::Platform(Handle<Object> wrapper) : platform_id(0)
{
}

JS_METHOD(Platform::getDevices)
{
  HandleScope scope;

  Platform *platform = UnwrapThis<Platform>(args);
  cl_device_type type = args[0]->Uint32Value();

  cl_uint n = 0;
  cl_int ret = ::clGetDeviceIDs(platform->platform_id, type, 0, NULL, &n);
  cout<<"Found "<<n<<" devices"<<endl;
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  cl_device_id* ids = new cl_device_id[n];
  ret = ::clGetDeviceIDs(platform->platform_id, type, n, ids, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  Local<Array> deviceArray = Array::New(n);
  for (int i=0; i<n; i++) {
    //cout<<"Found device: "<<ids[i]<<endl;
    deviceArray->Set(i, Device::New(ids[i])->handle_);
  }

  delete[] ids;

  return scope.Close(deviceArray);
}

JS_METHOD(Platform::getInfo)
{
  HandleScope scope;
  Platform *platform = UnwrapThis<Platform>(args);
  cl_platform_info param_name = args[0]->Uint32Value();

  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=clGetPlatformInfo(platform->platform_id, param_name, 1024, param_value, &param_value_size_ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(JS_STR(param_value,param_value_size_ret));
}

JS_METHOD(Platform::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  Platform *cl = new Platform(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

Platform *Platform::New(cl_platform_id pid)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Platform *platform = ObjectWrap::Unwrap<Platform>(obj);
  platform->platform_id = pid;

  return platform;
}

JS_METHOD(Platform::getExtension) {
  HandleScope scope;

  Platform *platform = UnwrapThis<Platform>(args);
  Local<String> vstr = args[0]->ToString();
  String::AsciiValue astr(vstr);
  char *str= *astr;
  for(int i=0;i<astr.length();i++)
    str[i]=tolower(str[i]);

  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=::clGetPlatformInfo(platform->platform_id, CL_PLATFORM_EXTENSIONS, 1024, param_value, &param_value_size_ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  char *p= ::strstr(param_value,str);
  if(!p)
    return ThrowError("UNKNOWN EXTENSION");

  return Undefined();
}

} // namespace webcl

