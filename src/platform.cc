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
using namespace std;

namespace webcl {

Persistent<FunctionTemplate> Platform::constructor_template;

void Platform::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Platform::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLPlatform"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getDevices", getDevices);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getSupportedExtensions", getSupportedExtensions);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enableExtension", enableExtension);

  target->Set(NanSymbol("WebCLPlatform"), ctor->GetFunction());
}

Platform::Platform(Handle<Object> wrapper) : platform_id(0), enableExtensions(NONE), availableExtensions(NONE)
{
  _type=CLObjType::Platform;
}

NAN_METHOD(Platform::getDevices)
{
  NanScope();

  Platform *platform = ObjectWrap::Unwrap<Platform>(args.This());
  cl_device_type type = CL_DEVICE_TYPE_ALL;
  if(!args[0]->IsUndefined() && !args[0]->IsNull())
    type=args[0]->Uint32Value();

  cl_uint n = 0;
  cl_int ret = ::clGetDeviceIDs(platform->platform_id, type, 0, NULL, &n);
  #ifdef LOGGING
  cout<<"Found "<<n<<" devices"<<endl;
  #endif
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PLATFORM);
    REQ_ERROR_THROW(INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  cl_device_id* ids = new cl_device_id[n];
  ret = ::clGetDeviceIDs(platform->platform_id, type, n, ids, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PLATFORM);
    REQ_ERROR_THROW(INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  Local<Array> deviceArray = Array::New(n);
  for (uint32_t i=0; i<n; i++) {
    #ifdef LOGGING
    char name[256];
    ::clGetDeviceInfo(ids[i],CL_DEVICE_NAME,sizeof(name),name,NULL);
    cout<<"Found device: "<<ids[i]<<" "<<name<<endl;
    #endif
    WebCLObject *obj=findCLObj((void*)ids[i], CLObjType::Device);
    if(obj)
      deviceArray->Set(i, NanObjectWrapHandle(obj));
    else
      deviceArray->Set(i, NanObjectWrapHandle(Device::New(ids[i])));
  }

  delete[] ids;

  NanReturnValue(deviceArray);
}

NAN_METHOD(Platform::getInfo)
{
  NanScope();
  Platform *platform = ObjectWrap::Unwrap<Platform>(args.This());
  cl_platform_info param_name = args[0]->Uint32Value();

  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=clGetPlatformInfo(platform->platform_id, param_name, 1024, param_value, &param_value_size_ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PLATFORM);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  // NOTE: Adjust length because API returns NULL terminated string
  NanReturnValue(JS_STR(param_value,(int)param_value_size_ret - 1));
}

NAN_METHOD(Platform::getSupportedExtensions)
{
  NanScope();
  Platform *platform = ObjectWrap::Unwrap<Platform>(args.This());
  char param_value[1024];
  size_t param_value_size_ret=0;

  cl_int ret=clGetPlatformInfo(platform->platform_id, CL_PLATFORM_EXTENSIONS, 1024, param_value, &param_value_size_ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PLATFORM);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

 NanReturnValue(JS_STR(param_value));
}

NAN_METHOD(Platform::enableExtension)
{
  NanScope();
  Platform *platform = ObjectWrap::Unwrap<Platform>(args.This());

  if(!args[0]->IsString()) {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_PLATFORM);
    NanReturnValue(JS_BOOL(false));
  }

  if(platform->availableExtensions==0x00) {
    char param_value[1024];
    size_t param_value_size_ret=0;

    cl_int ret=clGetPlatformInfo(platform->platform_id, CL_PLATFORM_EXTENSIONS, 1024, param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_PLATFORM);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    if(strcasestr(param_value,"gl_sharing")) platform->availableExtensions |= GL_SHARING;
    if(strcasestr(param_value,"fp16"))       platform->availableExtensions |= FP16;
    if(strcasestr(param_value,"fp64"))       platform->availableExtensions |= FP64;
  }

  Local<String> name=args[0]->ToString();
  String::AsciiValue astr(name);
  bool ret=false;
  if(strcasestr(*astr,"gl_sharing") && (platform->availableExtensions & GL_SHARING)) { platform->enableExtensions |= GL_SHARING; ret = true; }
  else if(strcasestr(*astr,"fp16") && (platform->availableExtensions & FP16))        { platform->enableExtensions |= FP16; ret = true; }
  else if(strcasestr(*astr,"fp64") && (platform->availableExtensions & FP64))        { platform->enableExtensions |= FP64; ret = true; }

  NanReturnValue(JS_BOOL(ret));
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
  // Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Platform *platform = ObjectWrap::Unwrap<Platform>(obj);
  platform->platform_id = pid;
  registerCLObj(pid, platform);

  return platform;
}

} // namespace webcl

