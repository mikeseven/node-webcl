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

#include "kernel.h"
#include "program.h"
#include "memoryobject.h"
#include "context.h"
#include "device.h"
#include "platform.h"
#include "sampler.h"

#include <cstring>

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

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getWorkGroupInfo", getWorkGroupInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_setArg", setArg);
  // Patch
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(String::NewSymbol("WebCLKernel"), constructor_template->GetFunction());
}

Kernel::Kernel(Handle<Object> wrapper) : kernel(0)
{
}

void Kernel::Destructor() {
  #ifdef LOGGING
  cout<<"  Destroying CL kernel"<<endl;
  #endif
  if(kernel) ::clReleaseKernel(kernel);
  kernel=0;
}

JS_METHOD(Kernel::release)
{
  HandleScope scope;
  Kernel *kernel = UnwrapThis<Kernel>(args);
  
  kernel->Destructor();
  
  return Undefined();
}

JS_METHOD(Kernel::getInfo)
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
    return scope.Close(JS_STR(param_value,(int)param_value_size_ret));
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
  cl_kernel_work_group_info param_name = args[1]->Uint32Value();

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
    return scope.Close(JS_INT((int32_t)param_value));
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
    return scope.Close(JS_NUM((double)param_value));
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
    for (int i=0; i<3; i++) {
      sizeArray->Set(i, JS_INT((int32_t)param_value[i]));
    }
    return scope.Close(sizeArray);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

// TODO: setArg is incomplete!!!!
JS_METHOD(Kernel::setArg)
{
  HandleScope scope;

  if (!args[0]->IsUint32())
    return ThrowError("CL_INVALID_ARG_INDEX");

  Kernel *kernel = UnwrapThis<Kernel>(args);
  cl_uint arg_index = args[0]->Uint32Value();
  cl_int ret;

  if(!args[1]->IsArray() && args[1]->IsObject()) {
    String::AsciiValue str(args[1]->ToObject()->GetPrototype()->ToDetailString());
    if(strcmp("WebCLSampler",*str)) {
      MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());
      cl_mem mem = mo->getMemory();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_mem), &mem);
    }
    else {
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
  }
  else {
    cl_uint type = args[2]->Uint32Value();
    cl_uint vec_type = type & 0xFFFF0000;
    type &= 0xFFFF;

    // TODO support for vectors
    // TODO check types LOCAL
    switch (type) {
    case types::LOCAL_MEMORY_SIZE: {
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(float), NULL);
      break;
    }
    case types::INT: {
      if (!args[1]->IsInt32())
        return ThrowError("ARG is not of specified type");
      cl_int arg = args[1]->Int32Value();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_int), &arg);
      break;
    }
    case types::UINT: {
      if (!args[1]->IsUint32())
        return ThrowError("ARG is not of specified type");
      cl_uint arg = args[1]->Uint32Value();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_uint), &arg);
      break;
    }
    case types::LONG: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_long arg = (cl_long)args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_long), &arg);
      break;
    }
    case types::ULONG: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_ulong arg = (cl_ulong)args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_ulong), &arg);
      break;
    }
    case types::FLOAT: {
      if (vec_type & types::VEC4) {
        float *arg = NULL;
        if (args[1]->IsArray()) {
          Local<Array> arr = Array::Cast(*args[1]);
          if (arr->GetIndexedPropertiesExternalArrayDataLength() < 0) {
            // pure JS array, no native backend
            float _arg[4];
            _arg[0] = (float) arr->Get(0)->NumberValue();
            _arg[1] = (float) arr->Get(1)->NumberValue();
            _arg[2] = (float) arr->Get(2)->NumberValue();
            _arg[3] = (float) arr->Get(3)->NumberValue();
            ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_float) * 4, _arg);
          } else {
            arg = (float*) arr->GetIndexedPropertiesExternalArrayData();
            ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_float) * 4, arg);
          }
        } else if (args[1]->IsObject()) {
          arg = (float*) args[1]->ToObject()->GetIndexedPropertiesExternalArrayData();
          ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_float) * 4, arg);
        }
      } else if (!args[1]->IsNumber()) {
        return ThrowError("ARG is not of specified type");
      }
      else {
        cl_float arg = (float) args[1]->NumberValue();
        ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_float), &arg);
      }
      break;
    }
    case types::DOUBLE: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_double arg = (cl_double)args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_double), &arg);
      break;
    }
    case types::HALF: { // TODO HALF may not be mapped correctly!
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_half arg = (cl_half) args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_half), &arg);
      break;
    }
    case types::SHORT: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_short arg = (cl_short) args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_short), &arg);
      break;
    }
    case types::USHORT: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_ushort arg = (cl_ushort) args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_ushort), &arg);
      break;
    }
    case types::CHAR: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_char arg =(cl_char)  args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_char), &arg);
      break;
    }
    case types::UCHAR: {
      if (!args[1]->IsNumber())
        return ThrowError("ARG is not of specified type");
      cl_uchar arg = (cl_uchar) args[1]->NumberValue();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_uchar), &arg);
      break;
    }
    default: {
      return ThrowError("UNKNOWN TYPE");
    }

    }
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
