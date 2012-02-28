/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "program.h"
#include "device.h"
#include "kernel.h"
#include "context.h"

#include <iostream>
#include <vector>
#include <cstring>
using namespace std;

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Program::constructor_template;

void Program::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Program::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLProgram"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getBuildInfo", getBuildInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_build", build);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createKernel", createKernel);

  target->Set(String::NewSymbol("WebCLProgram"), constructor_template->GetFunction());
}

Program::Program(Handle<Object> wrapper) : program(0)
{
}

void Program::Destructor() {
  cout<<"  Destroying CL program"<<endl;
  if(program) ::clReleaseProgram(program);
  program=0;
}

JS_METHOD(Program::getInfo)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);
  cl_program_info param_name = args[1]->NumberValue();

  switch (param_name) {
  case CL_PROGRAM_REFERENCE_COUNT:
  case CL_PROGRAM_NUM_DEVICES: {
    cl_uint value=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(cl_uint), &value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Integer::NewFromUnsigned(value));
  }
  case CL_PROGRAM_CONTEXT: {
    cl_context value=NULL;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(cl_context), &value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Context::New(value)->handle_);
  }
  case CL_PROGRAM_DEVICES: {
    cl_device_id devices[1024];
    size_t param_value_size_ret=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(cl_device_id)*1024, devices, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    size_t num_devices=param_value_size_ret;
    Local<Array> deviceArray = Array::New(num_devices);
    for (std::size_t i=0; i<num_devices; i++) {
      cl_device_id d = devices[i];
      deviceArray->Set(i, Device::New(d)->handle_);
    }
    return scope.Close(deviceArray);
  }
  case CL_PROGRAM_SOURCE: {
    char source[1024];
    size_t param_value_size_ret=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(char)*1024, source, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_STR(source,param_value_size_ret));
  }
  case CL_PROGRAM_BINARY_SIZES:
    return ThrowError("CL_PROGRAM_BINARY_SIZES unimplemented");
  case CL_PROGRAM_BINARIES:
    return ThrowError("CL_PROGRAM_BINARIES unimplemented");
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

JS_METHOD(Program::getBuildInfo)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);
  Device *dev = ObjectWrap::Unwrap<Device>(args[0]->ToObject());
  cl_program_info param_name = args[1]->Uint32Value();

  switch (param_name) {
  case CL_PROGRAM_BUILD_STATUS: {
    cl_build_status param_value;
    cl_int ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, sizeof(cl_build_status), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  default: {
    size_t param_value_size_ret=0;
    cl_int ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, 0,
        NULL, &param_value_size_ret);
    char *param_value=new char[param_value_size_ret];
    ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, param_value_size_ret,
        param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_PROGRAM);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    Local<Value> obj = scope.Close(JS_STR(param_value,param_value_size_ret));
    delete[] param_value;
    return obj;
  }
  }
}

JS_METHOD(Program::build)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);

  vector<cl_device_id> devices;
  if(args[0]->IsArray()) {
    Local<Array> deviceArray = Array::Cast(*args[0]);
    //cout<<"Building program for "<<deviceArray->Length()<<" devices"<<endl;
    for (int i=0; i<deviceArray->Length(); i++) {
      Local<Object> obj = deviceArray->Get(i)->ToObject();
      Device *d = ObjectWrap::Unwrap<Device>(obj);
      //cout<<"Device "<<i<<": "<<d->getDevice()<<endl;
      devices.push_back( d->getDevice() );
    }
  }
  else if(args[0]->IsObject()) {
    Local<Object> obj = args[0]->ToObject();
    Device *d = ObjectWrap::Unwrap<Device>(obj);
    devices.push_back( d->getDevice() );
  }

  char *options=NULL;
  if(!args[1]->IsUndefined() && args[1]->IsString()) {
    Local<String> str = args[1]->ToString();
    //cout<<"str length: "<<str->Length()<<endl;
    if(str->Length()>0) {
      String::AsciiValue _options(str);
      options = ::strdup(*_options);
    }
  }

  cl_int ret = ::clBuildProgram(prog->getProgram(), devices.size(), devices.size() ? &devices.front() : NULL,
      options,
      NULL /*notifyFptr*/,
      NULL /*data*/);
  if(options) free(options);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PROGRAM);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_DEVICE);
    REQ_ERROR_THROW(CL_INVALID_BINARY);
    REQ_ERROR_THROW(CL_INVALID_BUILD_OPTIONS);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_COMPILER_NOT_AVAILABLE);
    REQ_ERROR_THROW(CL_BUILD_PROGRAM_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

JS_METHOD(Program::createKernel)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);

  Local<String> str = args[0]->ToString();
  String::AsciiValue astr(str);

  cl_int ret = CL_SUCCESS;
  cl_kernel kw = ::clCreateKernel(prog->getProgram(), (const char*) *astr, &ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PROGRAM);
    REQ_ERROR_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(CL_INVALID_KERNEL_NAME);
    REQ_ERROR_THROW(CL_INVALID_KERNEL_DEFINITION);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(Kernel::New(kw)->handle_);
}

JS_METHOD(Program::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  /*Context *context = ObjectWrap::Unwrap<Context>(args[0]->ToObject());

  Local<String> str = args[1]->ToString();
  String::AsciiValue astr(str);
  cl::Program::Sources sources;
  std::pair<const char*, ::size_t> source(*astr,astr.length());
  sources.push_back(source);

  cl_int ret = CL_SUCCESS;
  cl::Program *pw = new cl::Program(*context->getContext(),sources,&ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }*/

  Program *p = new Program(args.This());
  //p->program=pw;
  p->Wrap(args.This());
  registerCLObj(p);
  return scope.Close(args.This());
}

Program *Program::New(cl_program pw)
{
  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Program *progobj = ObjectWrap::Unwrap<Program>(obj);
  progobj->program = pw;

  return progobj;
}

} // namespace
