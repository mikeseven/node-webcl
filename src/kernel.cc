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

Persistent<FunctionTemplate> Kernel::constructor;

void Kernel::Init(Handle<Object> exports)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Kernel::New);
  NanAssignPersistent(FunctionTemplate, constructor, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLKernel"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getArgInfo", getArgInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getWorkGroupInfo", getWorkGroupInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_setArg", setArg);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  exports->Set(NanSymbol("WebCLKernel"), ctor->GetFunction());
}

Kernel::Kernel(Handle<Object> wrapper) : kernel(0)
{
  _type=CLObjType::Kernel;
}

Kernel::~Kernel() {
#ifdef LOGGING
  printf("In ~Kernel\n");
#endif
  // Destructor();
}

void Kernel::Destructor() {
  if(kernel) {
    cl_uint count;
    ::clGetKernelInfo(kernel,CL_KERNEL_REFERENCE_COUNT,sizeof(cl_uint),&count,NULL);
#ifdef LOGGING
    cout<<"  Destroying Kernel, CLrefCount is: "<<count<<endl;
#endif
    ::clReleaseKernel(kernel);
    if(count==1) {
      unregisterCLObj(this);
      kernel=0;
    }
  }
}

NAN_METHOD(Kernel::release)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  
  kernel->Destructor();
  
  NanReturnUndefined();
}

NAN_METHOD(Kernel::getInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  cl_kernel_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_KERNEL_FUNCTION_NAME: {
    size_t param_value_size_ret=0;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, 0, NULL, &param_value_size_ret);
    if(ret==CL_SUCCESS && param_value_size_ret) {
      char *param_value=new char[param_value_size_ret];
      ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(char)*param_value_size_ret, param_value, NULL);
      if (ret != CL_SUCCESS) {
        REQ_ERROR_THROW(INVALID_VALUE);
        REQ_ERROR_THROW(INVALID_KERNEL);
        REQ_ERROR_THROW(OUT_OF_RESOURCES);
        REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
        return NanThrowError("UNKNOWN ERROR");
      }
      Local<String> str=JS_STR(param_value,(int)param_value_size_ret-1);
      delete[] param_value;
      NanReturnValue(str);
    }
    NanReturnUndefined();
  }
  case CL_KERNEL_CONTEXT: {
    cl_context param_value=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_context), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    if(param_value) {
      WebCLObject *obj=findCLObj((void*)param_value, CLObjType::Context);
      if(obj) 
        NanReturnValue(NanObjectWrapHandle(obj));
      else {
        printf("Creating new context\n");
        NanReturnValue(NanObjectWrapHandle(Context::New(param_value)));
      }
    }
    NanReturnUndefined();
  }
  case CL_KERNEL_PROGRAM: {
    cl_program p=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_program), &p, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    if(p) {
      WebCLObject *obj=findCLObj((void*)p, CLObjType::Program);
      if(obj) 
        NanReturnValue(NanObjectWrapHandle(obj));
      else {
        printf("Creating new program\n");
        NanReturnValue(NanObjectWrapHandle(Program::New(p, NULL)));
      }
    }
    NanReturnUndefined();
  }
  case CL_KERNEL_NUM_ARGS:
  case CL_KERNEL_REFERENCE_COUNT: {
    cl_uint param_value=0;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), param_name, sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT(param_value));
  }
  default: {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }
  }
}

const char *address_qualifiers[]={ "global", "local", "constant", "private" };
const char *access_qualifiers[]={ "read_only", "write_only", "read_write", "none" };

NAN_METHOD(Kernel::getArgInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  int index = args[0]->Uint32Value();
  cl_kernel_arg_address_qualifier addressQualifier;
  cl_kernel_arg_access_qualifier accessQualifier;
  cl_kernel_arg_type_qualifier typeQualifier;

  cl_int ret = ::clGetKernelArgInfo(kernel->getKernel(), index, 
                                    CL_KERNEL_ARG_ADDRESS_QUALIFIER, 
                                    sizeof(cl_kernel_arg_address_qualifier), &addressQualifier, NULL);

  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_ACCESS_QUALIFIER, 
                              sizeof(cl_kernel_arg_access_qualifier), &accessQualifier, NULL);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, 
                              CL_KERNEL_ARG_TYPE_QUALIFIER, 
                              sizeof(cl_kernel_arg_type_qualifier), &typeQualifier, NULL);

  char name[256], typeName[256];
  memset(name,0,256);
  memset(typeName,0,256);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, CL_KERNEL_ARG_TYPE_NAME, 256, typeName, NULL);
  ret |= ::clGetKernelArgInfo(kernel->getKernel(), index, CL_KERNEL_ARG_NAME, 256, name, NULL);

  if(ret!=CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_ARG_INDEX);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(KERNEL_ARG_INFO_NOT_AVAILABLE);
    REQ_ERROR_THROW(INVALID_KERNEL);
    return NanThrowError("Unknown Error");
  }

  // printf("name %s, typeName %s, addressQualifier %x, accessQualifier %x\n", name, typeName, addressQualifier,accessQualifier);
  Local<Object> kArgInfo = Object::New();
  kArgInfo->Set(JS_STR("name"), JS_STR(name));
  kArgInfo->Set(JS_STR("typeName"), JS_STR(typeName));
  kArgInfo->Set(JS_STR("addressQualifier"), JS_STR(address_qualifiers[addressQualifier-CL_KERNEL_ARG_ADDRESS_GLOBAL]));
  kArgInfo->Set(JS_STR("accessQualifier"), JS_STR(access_qualifiers[accessQualifier-CL_KERNEL_ARG_ACCESS_READ_ONLY]));
  string str;
  if(typeQualifier & CL_KERNEL_ARG_TYPE_NONE)
    str+="none ";
  if(typeQualifier & CL_KERNEL_ARG_TYPE_CONST)
    str+="const ";
  if(typeQualifier & CL_KERNEL_ARG_TYPE_RESTRICT)
    str+="restrict ";
  if(typeQualifier & CL_KERNEL_ARG_TYPE_VOLATILE)
    str+="volatile ";

  kArgInfo->Set(JS_STR("typeQualifier"), JS_STR(str.c_str()));

  NanReturnValue(kArgInfo);
}

NAN_METHOD(Kernel::getWorkGroupInfo)
{
  NanScope();
  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  Device *device = NULL;

  if(!args[0]->IsNull())
    device=ObjectWrap::Unwrap<Device>(args[0]->ToObject());
  else {
    // get 1st device associated with the program this kernel belongs to
    cl_program p=NULL;
    cl_int ret=::clGetKernelInfo(kernel->getKernel(), CL_KERNEL_PROGRAM, sizeof(cl_program), &p, NULL);
    size_t num_devices=0;
    ret=::clGetProgramInfo(p, CL_PROGRAM_DEVICES, 0, NULL, &num_devices);
    num_devices /= sizeof(cl_device_id);
    if(num_devices>1) {
      ret = CL_INVALID_DEVICE;
      REQ_ERROR_THROW(INVALID_DEVICE);
      NanReturnUndefined();      
    }
    cl_device_id d=NULL;
    ret |= ::clGetProgramInfo(p, CL_PROGRAM_DEVICES, sizeof(cl_device_id), &d, NULL);  
    if(ret == CL_SUCCESS) {
      device=static_cast<Device*>(findCLObj((void*)d, CLObjType::Device)); 
    }
    else {
      ret = CL_INVALID_DEVICE;
      REQ_ERROR_THROW(INVALID_DEVICE);
      NanReturnUndefined();
    }
  }

  cl_kernel_work_group_info param_name = args[1]->Uint32Value();

  switch (param_name) {
  case CL_KERNEL_WORK_GROUP_SIZE:
  case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE: {
    size_t param_value=0;
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT((int32_t)param_value));
  }
  case CL_KERNEL_LOCAL_MEM_SIZE:
  case CL_KERNEL_PRIVATE_MEM_SIZE: {
    cl_ulong param_value=0;
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(cl_ulong), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_NUM((double)param_value));
  }
  case CL_KERNEL_COMPILE_WORK_GROUP_SIZE: {
    ::size_t param_value[]={0,0,0};
    cl_int ret=::clGetKernelWorkGroupInfo(kernel->getKernel(), device->getDevice(), param_name, sizeof(size_t)*3, &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_KERNEL);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    Local<Array> sizeArray = Array::New(3);
    for (int i=0; i<3; i++) {
      sizeArray->Set(i, JS_INT((int32_t)param_value[i]));
    }
    NanReturnValue(sizeArray);
  }
  default: {
    cl_int ret = CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }
  }
}

// static const char* types[]={"char","uchar","short","ushort","int","uint","long","ulong","float","double","half"};

struct TypeInfo {
  const char *name; // type name
  const size_t lname;  // type name's length (# of chars)
  const size_t size;   // type size
};
static const TypeInfo types[]={
  { "char", 4, 1 }, { "uchar", 5, 1 },
  { "short", 5, 2 }, { "ushort", 6, 2 },
  { "int", 3, 4 }, { "uint", 4, 4 },
  { "long", 4, 4 }, { "ulong", 5, 4 },
  { "float", 5, 4 }, { "double", 6, 8 }, 
  { "half", 4, 2 },
};
static const int nTypes=sizeof(types)/sizeof(TypeInfo);

NAN_METHOD(Kernel::setArg)
{
  NanScope();

  if (!args[0]->IsUint32())
    return NanThrowError("INVALID_ARG_INDEX");

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args.This());
  cl_kernel k = kernel->getKernel();
  cl_uint arg_index = args[0]->Uint32Value();
  cl_int ret=CL_SUCCESS;

  if(!k) {
    cl_int ret=CL_INVALID_KERNEL;
    REQ_ERROR_THROW(INVALID_KERNEL);
  }

  if(args[1]->IsObject()) {
    String::AsciiValue str(args[1]->ToObject()->GetConstructorName());
    if(!strcmp("WebCLSampler",*str)) {
      // WebCLSampler
      Sampler *s = ObjectWrap::Unwrap<Sampler>(args[1]->ToObject());
      cl_sampler sampler = s->getSampler();

      if(sampler == 0) {
        ret=CL_INVALID_SAMPLER;
        REQ_ERROR_THROW(INVALID_SAMPLER); // bug in OSX that allows null sampler without throwing exception
      }

      ret = ::clSetKernelArg(k, arg_index, sizeof(cl_sampler), &sampler);
    }
    else if(!strcmp(*str, "WebCLBuffer") || !strcmp(*str, "WebCLImage")) {
      // WebCLBuffer and WebCLImage
      // printf("[SetArg] mem object\n");
      MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());
      cl_mem mem = mo->getMemory();
      ret = ::clSetKernelArg(k, arg_index, sizeof(cl_mem), &mem);
    }
    else if(!args[1]->IsArray()) {
      Local<Object> obj=args[1]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      char *host_ptr=NULL;
      int len=0;
      int bytes=0;

      if(!strcmp("Buffer",*name)) {
        host_ptr = node::Buffer::Data(obj);
        bytes = node::Buffer::Length(obj);
      }
      else {
        // ArrayBufferView
        host_ptr= (char*) (obj->GetIndexedPropertiesExternalArrayData());
        len=obj->GetIndexedPropertiesExternalArrayDataLength(); // number of elements
        bytes=obj->Get(JS_STR("byteLength"))->Uint32Value();
        // int byteOffset=obj->Get(JS_STR("byteOffset"))->Uint32Value();
      }
      // printf("TypedArray: len %d, bytes %d, byteOffset %d\n",len,bytes,byteOffset);

      char typeName[16];
      ret = ::clGetKernelArgInfo(k, arg_index, CL_KERNEL_ARG_TYPE_NAME, sizeof(typeName), typeName, NULL);

      if(len>1) {
        for(int i=0;i<nTypes;i++) {
          if(strstr(typeName,types[i].name)==typeName) {
            if(strlen(typeName) > types[i].lname) {
              int vecSize = atoi(typeName + types[i].lname);
              bytes*=vecSize;
            }
            if(len)
              bytes/=len;

            break;
          }
        }
      }
    
      if(len == 1) {
        // handle __local params
        // printf("[setArg] index %d has 1 value\n",arg_index);
        cl_kernel_arg_address_qualifier addr=0;
        ret = ::clGetKernelArgInfo(k, arg_index, CL_KERNEL_ARG_ADDRESS_QUALIFIER, 
                              sizeof(cl_kernel_arg_address_qualifier), &addr, NULL);
        if(addr == CL_KERNEL_ARG_ADDRESS_LOCAL) {
          // printf("  index %d size: %d\n",arg_index,*((cl_int*) host_ptr));          
          ret = ::clSetKernelArg(k, arg_index, *((cl_int*) host_ptr), NULL);
          // printf("[setArg __local] ret = %d\n",ret);
        }
        else {
          ret = ::clSetKernelArg(k, arg_index, bytes, host_ptr);
          // printf("ret1= %d\n",ret);
        }
      }
      else {
        ret = ::clSetKernelArg(k, arg_index, bytes, host_ptr);
        // printf("ret2= %d\n",ret);
      }
   }
    else 
      return NanThrowTypeError("Invalid object for arg 1");
  }
  else 
    return NanThrowTypeError("Invalid object for arg 1");
 
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_KERNEL);
    REQ_ERROR_THROW(INVALID_ARG_INDEX);
    REQ_ERROR_THROW(INVALID_ARG_VALUE);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_SAMPLER);
    REQ_ERROR_THROW(INVALID_ARG_SIZE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
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
  NanReturnValue(args.This());
}

Kernel *Kernel::New(cl_kernel kw, WebCLObject *parent)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(obj);
  kernel->kernel = kw;
  kernel->setParent(parent);
  registerCLObj(kw, kernel);

  return kernel;
}

}
