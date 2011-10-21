/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLProgram.h"
#include "WebCLDevice.h"
#include "WebCLKernel.h"
#include "WebCLContext.h"

#include <iostream>

using namespace std;
using namespace v8;
using namespace webcl;
using namespace cl;

Persistent<FunctionTemplate> WebCLProgram::constructor_template;

/* static  */
void WebCLProgram::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLProgram::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLProgram"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getBuildInfo", getBuildInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "build", build);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createKernel", createKernel);

  target->Set(String::NewSymbol("WebCLProgram"), constructor_template->GetFunction());
}

WebCLProgram::WebCLProgram(Handle<Object> wrapper) : program(0)
{
}

WebCLProgram::~WebCLProgram()
{
  if (program) delete program;
}

JS_METHOD(WebCLProgram::getInfo)
{
  HandleScope scope;
  WebCLProgram *prog = node::ObjectWrap::Unwrap<WebCLProgram>(args.This());
  cl_program_info param_name = args[1]->NumberValue();

  switch (param_name) {
  case CL_PROGRAM_REFERENCE_COUNT:
  case CL_PROGRAM_NUM_DEVICES: {
    cl_uint value=0;
    cl_int ret=prog->getProgram()->getInfo(param_name, &value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(Integer::NewFromUnsigned(value));
  }
  case CL_PROGRAM_CONTEXT: {
    cl::Context value;;
    cl_int ret=prog->getProgram()->getInfo(param_name, &value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    cl::Context *cw = new cl::Context(value);
    return scope.Close(WebCLContext::New(cw)->handle_);
  }
  case CL_PROGRAM_DEVICES: {
    VECTOR_CLASS<cl_device_id> devices;
    cl_int ret=prog->getProgram()->getInfo(param_name, &devices);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    std::size_t num_devices=devices.size();
    Local<Array> deviceArray = Array::New(num_devices);
    for (std::size_t i=0; i<num_devices; i++) {
      cl_device_id d = devices[i];
      cl::Device *dw = new cl::Device(d);
      deviceArray->Set(i, WebCLDevice::New(dw)->handle_);
    }
    return scope.Close(deviceArray);
  }
  case CL_PROGRAM_SOURCE: {
    STRING_CLASS source;
    cl_int ret=prog->getProgram()->getInfo(param_name, &source);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(String::New(source.c_str(),source.length()));
  }
  case CL_PROGRAM_BINARY_SIZES:
    return ThrowException(Exception::Error(String::New("CL_PROGRAM_BINARY_SIZES unimplemented")));
  case CL_PROGRAM_BINARIES:
    return ThrowException(Exception::Error(String::New("CL_PROGRAM_BINARIES unimplemented")));
  default:
    return ThrowException(Exception::Error(String::New("UNKNOWN param_name")));
  }
}

JS_METHOD(WebCLProgram::getBuildInfo)
{
  HandleScope scope;
  WebCLProgram *prog = node::ObjectWrap::Unwrap<WebCLProgram>(args.This());
  WebCLDevice *dev = ObjectWrap::Unwrap<WebCLDevice>(args[0]->ToObject());
  cl_program_info param_name = args[1]->Uint32Value();

  switch (param_name) {
  case CL_PROGRAM_BUILD_STATUS: {
    cl_build_status param_value;
    cl_int ret=prog->getProgram()->getBuildInfo(*(dev->getDevice()), param_name, &param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  default: {
    STRING_CLASS param_value;
    cl_int ret=prog->getProgram()->getBuildInfo(*(dev->getDevice()), param_name, &param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(JS_STR(param_value.c_str(),param_value.length()));
  }
  }
}

static char STR_EMPTY[]="";

JS_METHOD(WebCLProgram::build)
{
  HandleScope scope;
  WebCLProgram *prog = node::ObjectWrap::Unwrap<WebCLProgram>(args.This());

  if (!args[0]->IsArray())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));
  if (!args[1]->IsString())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));

  Local<Array> deviceArray = Array::Cast(*args[0]);
  VECTOR_CLASS<cl::Device> devices;
  for (int i=0; i<deviceArray->Length(); i++) {
    Local<Object> obj = deviceArray->Get(i)->ToObject();
    WebCLDevice *d = ObjectWrap::Unwrap<WebCLDevice>(obj);
    devices.push_back( *d->getDevice() );
  }

  char *options=STR_EMPTY;
  if(!args[0]->IsUndefined()) {
    Local<Value> optionString = args[1];
    Local<String> str = args[1]->ToString();
    String::AsciiValue _options(str);
    options = *_options;
  }

  cl_int ret = CL_SUCCESS;
  prog->getProgram()->build(devices,options,NULL,NULL);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_BINARY);
    WEBCL_COND_RETURN_THROW(CL_INVALID_BUILD_OPTIONS);
    WEBCL_COND_RETURN_THROW(CL_INVALID_OPERATION);
    WEBCL_COND_RETURN_THROW(CL_COMPILER_NOT_AVAILABLE);
    WEBCL_COND_RETURN_THROW(CL_BUILD_PROGRAM_FAILURE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return Undefined();
}

JS_METHOD(WebCLProgram::createKernel)
{
  HandleScope scope;
  WebCLProgram *prog = node::ObjectWrap::Unwrap<WebCLProgram>(args.This());

  Local<String> str = args[0]->ToString();
  String::AsciiValue astr(str);

  cl_int ret = CL_SUCCESS;
  cl::Kernel *kw=new cl::Kernel(*prog->getProgram(),(const char*) *astr,&ret);

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM);
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL_NAME);
    WEBCL_COND_RETURN_THROW(CL_INVALID_KERNEL_DEFINITION);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLKernel::New(kw)->handle_);
}

/* static  */
JS_METHOD(WebCLProgram::New)
{
  HandleScope scope;
  WebCLContext *context = ObjectWrap::Unwrap<WebCLContext>(args[0]->ToObject());

  Local<String> str = args[1]->ToString();
  String::AsciiValue astr(str);
  cl::Program::Sources sources;
  std::pair<const char*, ::size_t> source(*astr,astr.length());
  sources.push_back(source);

  cl_int ret = CL_SUCCESS;
  cl::Program *pw = new cl::Program(*context->getContext(),sources,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  WebCLProgram *cl = new WebCLProgram(args.This());
  cl->program=pw;
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLProgram *WebCLProgram::New(cl::Program* pw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLProgram *progobj = ObjectWrap::Unwrap<WebCLProgram>(obj);
  progobj->program = pw;

  return progobj;
}
