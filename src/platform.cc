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

#include "platform.h"
#include "device.h"

#include <cstring>

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Platform::constructor_template;

void Platform::Init(Handle<Object> target)
{
  NanScope();

  Local<FunctionTemplate> t = FunctionTemplate::New(Platform::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(NanSymbol("WebCLPlatform"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getDevices", getDevices);

  target->Set(NanSymbol("WebCLPlatform"), constructor_template->GetFunction());
}

Platform::Platform(Handle<Object> wrapper) : platform_id(0)
{
}

NAN_METHOD(Platform::getDevices)
{
  NanScope();

  Platform *platform = UnwrapThis<Platform>(args);
  cl_device_type type = args[0]->Uint32Value();

  cl_uint n = 0;
  cl_int ret = ::clGetDeviceIDs(platform->platform_id, type, 0, NULL, &n);
  #ifdef LOGGING
  cout<<"Found "<<n<<" devices"<<endl;
  #endif
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
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
    return NanThrowError("UNKNOWN ERROR");
  }

  Local<Array> deviceArray = Array::New(n);
  for (uint32_t i=0; i<n; i++) {
    #ifdef LOGGING
    cout<<"Found device: "<<ids[i]<<endl;
    #endif
    deviceArray->Set(i, Device::New(ids[i])->handle_);
  }

  delete[] ids;

  NanReturnValue(deviceArray);
}

NAN_METHOD(Platform::getInfo)
{
  NanScope();
  Platform *platform = UnwrapThis<Platform>(args);
  cl_platform_info param_name = args[0]->Uint32Value();

  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=clGetPlatformInfo(platform->platform_id, param_name, 1024, param_value, &param_value_size_ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  // NOTE: Adjust length because API returns NULL terminated string
  NanReturnValue(JS_STR(param_value,(int)param_value_size_ret - 1));
}

NAN_METHOD(Platform::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();
  Platform *cl = new Platform(args.This());
  cl->Wrap(args.This());
  NanReturnValue(args.This());
}

Platform *Platform::New(cl_platform_id pid)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Platform *platform = ObjectWrap::Unwrap<Platform>(obj);
  platform->platform_id = pid;

  return platform;
}

NAN_METHOD(Platform::getExtension) {
  NanScope();

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
    return NanThrowError("UNKNOWN ERROR");
  }

  char *p= ::strstr(param_value,str);
  if(!p)
    return NanThrowError("UNKNOWN EXTENSION");

  NanReturnUndefined();
}

} // namespace webcl

