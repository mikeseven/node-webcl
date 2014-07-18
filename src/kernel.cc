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
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Kernel::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLKernel"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getArgInfo", getArgInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getWorkGroupInfo", getWorkGroupInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_setArg", setArg);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLKernel"), ctor->GetFunction());
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

NAN_METHOD(Kernel::release)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  
  DESTROY_WEBCL_OBJECT(kernel);
  
  NanReturnUndefined();
}

NAN_METHOD(Kernel::getInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
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
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_STR(param_value,(int)param_value_size_ret));
  }
  case CL_KERNEL_CONTEXT: {
    cl_context param_value=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_context), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(NanObjectWrapHandle(Context::New(param_value)));
  }
  case CL_KERNEL_PROGRAM: {
    cl_program param_value=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_program), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_KERNEL);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(NanObjectWrapHandle(Program::New(param_value)));
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
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT(param_value));
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

NAN_METHOD(Kernel::getArgInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  int index = args[0]->Uint32Value();
  char name[256], typeName[256];
  int addressQualifier, accessQualifier, typeQualifier;

  cl_int ret = ::clGetKernelArgInfo(kernel->getKernel(), index, 
                                    CL_KERNEL_ARG_ADDRESS_QUALIFIER, 
                                    sizeof(cl_kernel_arg_address_qualifier), &addressQualifier, NULL);

  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_ACCESS_QUALIFIER, 
                              sizeof(cl_kernel_arg_access_qualifier), &accessQualifier, NULL);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_TYPE_QUALIFIER, 
                              sizeof(cl_kernel_arg_type_qualifier), &typeQualifier, NULL);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_TYPE_NAME, 
                              sizeof(typeName), typeName, NULL);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_NAME, 
                              sizeof(name), name, NULL);

  if(ret!=CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_ARG_INDEX);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_KERNEL_ARG_INFO_NOT_AVAILABLE);
    REQ_ERROR_THROW(CL_INVALID_KERNEL);
    return NanThrowError("Unknown Error");
  }

  // TODO create WebCLKernelArgInfo dictionary
  Local<Object> kArgInfo = Object::New();
  kArgInfo->Set(JS_STR("name"), JS_STR(name));
  kArgInfo->Set(JS_STR("typeName"), JS_STR(typeName));
  kArgInfo->Set(JS_STR("addressQualifier"), JS_INT(addressQualifier));
  kArgInfo->Set(JS_STR("accessQualifier"), JS_INT(accessQualifier));
  kArgInfo->Set(JS_STR("typeQualifier"), JS_INT(typeQualifier));

  NanReturnValue(kArgInfo);
}

NAN_METHOD(Kernel::getWorkGroupInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
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
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT((int32_t)param_value));
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
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_NUM((double)param_value));
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
      return NanThrowError("UNKNOWN ERROR");
    }

    Local<Array> sizeArray = Array::New(3);
    for (int i=0; i<3; i++) {
      sizeArray->Set(i, JS_INT((int32_t)param_value[i]));
    }
    NanReturnValue(sizeArray);
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

// TODO: setArg is incomplete!!!!
NAN_METHOD(Kernel::setArg)
{
  NanScope();

  if (!args[0]->IsUint32())
    return NanThrowError("CL_INVALID_ARG_INDEX");

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  cl_uint arg_index = args[0]->Uint32Value();
  cl_int ret=CL_SUCCESS;

  if(args[1]->IsObject()) {
    String::AsciiValue str(args[1]->ToObject()->GetConstructorName());
    if(!strcmp("WebCLSampler",*str)) {
      // WebCLSampler
      cl_sampler sampler;
      Sampler *s = ObjectWrap::Unwrap<Sampler>(args[1]->ToObject());
      sampler = s->getSampler();

      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_sampler), &sampler);
    }
    else if(!strcmp(*str, "WebCLBuffer") || !strcmp(*str, "WebCLImage")) {
      // WebCLBuffer and WebCLImage
      // printf("[SetArg] mem object\n");
      MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());
      cl_mem mem = mo->getMemory();
      ret = ::clSetKernelArg(kernel->getKernel(), arg_index, sizeof(cl_mem), &mem);
    }
    else if(!args[1]->IsArray()) {
      // ArrayBufferView
      Handle<Object> obj=args[1]->ToObject();
      char *host_ptr= (char*) (obj->GetIndexedPropertiesExternalArrayData());
      int len=obj->GetIndexedPropertiesExternalArrayDataLength(); // number of elements
      int bytes=obj->Get(JS_STR("byteLength"))->Uint32Value();
      // int byteOffset=obj->Get(JS_STR("byteOffset"))->Uint32Value();

      if(len == 1) {
        // handle __local params
        // printf("[setArg] index %d has 1 value\n",arg_index);
        cl_kernel_arg_address_qualifier addr=0;
        ret = ::clGetKernelArgInfo(kernel->getKernel(), arg_index, CL_KERNEL_ARG_ADDRESS_QUALIFIER, 
                              sizeof(cl_kernel_arg_address_qualifier), &addr, NULL);
        if(addr == CL_KERNEL_ARG_ADDRESS_LOCAL) {
          // printf("  index %d size: %d\n",arg_index,*((cl_int*) host_ptr));          
          ret = ::clSetKernelArg(kernel->getKernel(), arg_index, *((cl_int*) host_ptr), NULL);
          // printf("[setArg __local] ret = %d\n",ret);
        }
        else
          ret = ::clSetKernelArg(kernel->getKernel(), arg_index, bytes, host_ptr);
      }
      else
        ret = ::clSetKernelArg(kernel->getKernel(), arg_index, bytes, host_ptr);
   }
    else 
      return NanThrowTypeError("Invalid object for arg 1");
  }
  else 
    return NanThrowTypeError("Invalid object for arg 1");
 
  if (ret != CL_SUCCESS) {
    printf("[setArg] ret = %d\n",ret);
    REQ_ERROR_THROW(CL_INVALID_KERNEL);
    REQ_ERROR_THROW(CL_INVALID_ARG_INDEX);
    REQ_ERROR_THROW(CL_INVALID_ARG_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_SAMPLER);
    REQ_ERROR_THROW(CL_INVALID_ARG_SIZE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

NAN_METHOD(Kernel::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();
  Kernel *k = new Kernel(args.This());
  k->Wrap(args.This());
  registerCLObj(k);
  NanReturnValue(args.This());
}

Kernel *Kernel::New(cl_kernel kw)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(obj);
  kernel->kernel = kw;

  return kernel;
}

}
