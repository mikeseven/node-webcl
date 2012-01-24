/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "kernel.h"
#include "program.h"
#include "memoryobject.h"
#include "context.h"
#include "device.h"
#include "platform.h"
#include "sampler.h"

#include <iostream>
using namespace std;

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Kernel::constructor_template;

void Kernel::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Kernel::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLKernel"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getKernelInfo", getKernelInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getWorkGroupInfo", getWorkGroupInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_setArg", setArg);

  target->Set(String::NewSymbol("WebCLKernel"), constructor_template->GetFunction());
}

Kernel::Kernel(Handle<Object> wrapper) : kernel(0)
{
}

void Kernel::Destructor() {
  cout<<"  Destroying CL kernel"<<endl;
  if(kernel) ::clReleaseKernel(kernel);
  kernel=0;
}

JS_METHOD(Kernel::getKernelInfo)
{
  HandleScope scope;
  Kernel *kernel = UnwrapThis<Kernel>(args);
  cl_kernel_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_KERNEL_FUNCTION_NAME: {
    char param_value[1024];
    size_t param_value_size_ret=0;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(char)*1024, &param_value, &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_STR(param_value,param_value_size_ret));
  }
  case CL_KERNEL_CONTEXT: {
    cl_context param_value=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_context), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Context::New(param_value)->handle_);
  }
  case CL_KERNEL_PROGRAM: {
    cl_program param_value=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_program), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Program::New(param_value)->handle_);
  }
  case CL_KERNEL_NUM_ARGS:
  case CL_KERNEL_REFERENCE_COUNT: {
    cl_uint param_value=0;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

JS_METHOD(Kernel::getWorkGroupInfo)
{
  HandleScope scope;
  Kernel *kernel = UnwrapThis<Kernel>(args);
  Device *device = ObjectWrap::Unwrap<Device>(args[0]->ToObject());
  cl_kernel_work_group_info param_name = args[1]->NumberValue();

  switch (param_name) {
  case CL_KERNEL_WORK_GROUP_SIZE:
  case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: {
    size_t param_value=0;
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_KERNEL_LOCAL_MEM_SIZE:
  case CL_KERNEL_PRIVATE_MEM_SIZE: {
    cl_ulong param_value=0;
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(cl_ulong), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
    ::size_t param_value[]={0,0,0};
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(size_t)*3, &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    Local<Array> sizeArray = Array::New(3);
    for (std::size_t i=0; i<3; i++) {
      sizeArray->Set(i, JS_INT(param_value[i]));
    }
    return scope.Close(sizeArray);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

JS_METHOD(Kernel::setArg)
{
  HandleScope scope;

  if (!args[0]->IsUint32())
    return ThrowError("CL_INVALID_ARG_INDEX");

  Kernel *kernel = UnwrapThis<Kernel>(args);
  cl_uint arg_index = args[0]->Uint32Value();
  std::size_t arg_size = 0;
  cl_uint type = 0;
  void *arg_value = 0;
  cl_int ret;

  type = args[2]->Uint32Value();

  // TODO check types other than MEMORY_OBJECT
  if(type & types::LOCAL) { // TODO is it true for all __local args?
    cl_float arg = args[1]->NumberValue();
    ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(float), &arg);
  }
  else if(type & types::MEM) {
    cl_mem mem;
    if (args[1]->IsUint32()) {
      cl_uint ptr = args[1]->Uint32Value();
      if (ptr)
        return ThrowError("ARG is not of specified type");
      mem = 0;
    } else {
      MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());
      mem = mo->getMemory();
    }
    ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_mem), &mem);
  }
  else if(type & types::SAMPLER) {
    cl_sampler sampler;
    if (args[1]->IsUint32()) {
      cl_uint ptr = args[1]->Uint32Value();
      if (ptr)
        return ThrowError("ARG is not of specified type");
      sampler = 0;
    } else {
      Sampler *s = ObjectWrap::Unwrap<Sampler>(args[1]->ToObject());
      sampler = s->getSampler();
    }
    ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_sampler), &sampler);
  }
  else if(type & types::INT) {
    if (((type & types::UNSIGNED) && !args[1]->IsUint32()) || !args[1]->IsInt32())
      return ThrowError("ARG is not of specified type");
    if(type & types::UNSIGNED) {
      cl_uint arg=args[1]->Uint32Value();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_uint), &arg);
    }
    else {
      cl_int arg=args[1]->Int32Value();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_int), &arg);
    }
  }
  else if(type & types::LONG) {
    if (!args[1]->IsNumber())
      return ThrowError("ARG is not of specified type");
    if(type & types::UNSIGNED) {
      cl_ulong arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_ulong), &arg);
    }
    else {
      cl_long arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_long), &arg);
    }
  }
  else if(type && types::FLOAT) {
    if (!args[1]->IsNumber())
      return ThrowError("ARG is not of specified type");
    cl_float arg = args[1]->NumberValue();
    ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_float), &arg);
  }
  else if(type & types::HALF_FLOAT) {
    if (!args[1]->IsNumber())
      return ThrowError("ARG is not of specified type");
    cl_half arg = args[1]->NumberValue();
    ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_half), &arg);
  }
  else if(type & types::SHORT) {
    if (!args[1]->IsNumber())
      return ThrowError("ARG is not of specified type");
    if(type & types::UNSIGNED) {
      cl_ushort arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_ushort), &arg);
    }
    else {
      cl_short arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_short), &arg);
    }
  }
  else if(type & types::CHAR) {
    if (!args[1]->IsNumber())
      return ThrowError("ARG is not of specified type");
    if(type & types::UNSIGNED) {
      cl_uchar arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_uchar), &arg);
    }
    else {
      cl_char arg = args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_char), &arg);
    }
  }
  else {
    return ThrowError("UNKNOWN TYPE");
  }

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_KERNEL);
    REQ_ERROR_THROW(CL_INVALID_ARG_INDEX);
    REQ_ERROR_THROW(CL_INVALID_ARG_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_SAMPLER);
    REQ_ERROR_THROW(CL_INVALID_ARG_SIZE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

JS_METHOD(Kernel::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  Kernel *k = new Kernel(args.This());
  k->Wrap(args.This());
  registerCLObj(k);
  return scope.Close(args.This());
}

Kernel *Kernel::New(cl_kernel kw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(obj);
  kernel->kernel = kw;

  return kernel;
}

}
