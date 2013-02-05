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

#include "program.h"
#include "device.h"
#include "kernel.h"
#include "context.h"

#include <vector>
#include <cstdlib>
#include <cstring>

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
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(String::NewSymbol("WebCLProgram"), constructor_template->GetFunction());
}

Program::Program(Handle<Object> wrapper) : program(0)
{
}

void Program::Destructor() {
  #ifdef LOGGING
  cout<<"  Destroying CL program"<<endl;
  #endif
  if(program) ::clReleaseProgram(program);
  program=0;
}

JS_METHOD(Program::release)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);
  
  prog->Destructor();
  
  return Undefined();
}

JS_METHOD(Program::getInfo)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);
  cl_program_info param_name = args[1]->Uint32Value();

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
    int num_devices=(int)param_value_size_ret;
    Local<Array> deviceArray = Array::New(num_devices);
    for (int i=0; i<num_devices; i++) {
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
    return scope.Close(JS_STR(source,(int)param_value_size_ret));
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
    Local<Value> obj = scope.Close(JS_STR(param_value,(int)param_value_size_ret));
    delete[] param_value;
    return obj;
  }
  }
}

void Program::callback (cl_program program, void *user_data)
{
  //cout<<"[Program::driver_callback] thread "<<pthread_self()<<endl<<flush;
  Baton *baton = static_cast<Baton*>(user_data);
  //cout<<"  baton: "<<hex<<baton<<dec<<endl<<flush;
  baton->error=0;

  int num_devices=0;
  ::clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(int), &num_devices, NULL);
  if(num_devices>0) {
    cl_device_id *devices=new cl_device_id[num_devices];
    ::clGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*num_devices, devices, NULL);
    for(int i=0;i<num_devices;i++) {
      int err;
      ::clGetProgramBuildInfo(program, devices[i], CL_PROGRAM_BUILD_STATUS, sizeof(int), &err, NULL);
      baton->error |= err;
    }
    delete[] devices;
  }

  //cout<<"  async init"<<endl<<flush;
  uv_async_init(uv_default_loop(), &baton->async, After_cb);
  uv_async_send(&baton->async);
}

void
Program::After_cb(uv_async_t* handle, int status) {
  HandleScope scope;

  Baton *baton = static_cast<Baton*>(handle->data);
  //cout<<"[Program::After_cb] baton= "<<hex<<baton<<dec<<endl<<flush;
  uv_close((uv_handle_t*) &baton->async,NULL);

  Handle<Value> argv[]={
      JS_INT(baton->error),
      baton->data
  };

  TryCatch try_catch;

  baton->callback->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
      node::FatalException(try_catch);

  baton->callback.Dispose();
  baton->data.Dispose();
  baton->parent.Dispose();
  delete baton;
}

JS_METHOD(Program::build)
{
  HandleScope scope;
  Program *prog = UnwrapThis<Program>(args);

  cl_device_id *devices=NULL;
  int num=0;
  if(args[0]->IsArray()) {
    Local<Array> deviceArray = Array::Cast(*args[0]);
    //cout<<"Building program for "<<deviceArray->Length()<<" devices"<<endl;
    num=deviceArray->Length();
    devices=new cl_device_id[num];
    for (int i=0; i<num; i++) {
      Local<Object> obj = deviceArray->Get(i)->ToObject();
      Device *d = ObjectWrap::Unwrap<Device>(obj);
      //cout<<"Device "<<i<<": "<<d->getDevice()<<endl;
      devices[i] = d->getDevice();
    }
  }
  else if(args[0]->IsObject()) {
    Local<Object> obj = args[0]->ToObject();
    Device *d = ObjectWrap::Unwrap<Device>(obj);
    num=1;
    devices=new cl_device_id;
    *devices= d->getDevice();
  }
  //cout<<"[Program::build] #devices: "<<num<<" ptr="<<hex<<devices<<dec<<endl<<flush;

  char *options=NULL;
  if(!args[1]->IsUndefined() && !args[1]->IsNull() && args[1]->IsString()) {
    String::AsciiValue str(args[1]);
    //cout<<"str length: "<<str.length()<<endl;
    if(str.length()>0) {
      options = ::strdup(*str);
    }
  }

  Baton *baton=NULL;
  if(args.Length()==4 && !args[3]->IsUndefined() && args[3]->IsFunction()) {

    baton=new Baton();
    //cout<<"[Program::build] Creating baton "<<hex<<baton<<dec<<" on thread "<<hex<<pthread_self()<<dec<<endl<<flush;

    Local<Function> fct=Local<Function>::Cast(args[3]);
    baton->callback=Persistent<Function>::New(fct);

    if(!args[2].IsEmpty() && !args[2]->IsUndefined() && !args[2]->IsNull()) {
      Local<Value> data=args[2];
      baton->data=Persistent<Value>::New(data);
    }

	//uv_async_init(uv_default_loop(), &baton->async, After_cb);
    baton->async.data=baton;
  }
  //cout<<"[Program::build] Calling clBuildProgram with baton: "<<hex<<baton<<dec<<endl<<flush;

  cl_int ret = ::clBuildProgram(prog->getProgram(), num, devices,
      options,
      baton ? Program::callback : NULL,
      baton);
  //cout<<"[Program::build] cleaning up options and devices lists"<<endl<<flush;
  if(options) free(options);
  if(devices) delete[] devices;

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

  //cout<<"[Program::build] end"<<endl<<flush;
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
