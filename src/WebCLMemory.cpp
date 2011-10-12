/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLMemory.h"

using namespace v8;
using namespace webcl;

Persistent<FunctionTemplate> WebCLMemory::constructor_template;

/* static  */
void WebCLMemory::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLMemory::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLMemoryObject"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getImageInfo", getImageInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createSubBuffer", createSubBuffer);

  target->Set(String::NewSymbol("WebCLMemoryObject"), constructor_template->GetFunction());
}

WebCLMemory::WebCLMemory(Handle<Object> wrapper) : memory(0)
{
}

WebCLMemory::~WebCLMemory()
{
  if (memory) delete memory;
}

/* static  */
JS_METHOD(WebCLMemory::getInfo)
{
  HandleScope scope;

  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args.This());
  cl_mem_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_MEM_TYPE:
  case CL_MEM_FLAGS:
  case CL_MEM_REFERENCE_COUNT:
  case CL_MEM_MAP_COUNT: {
    cl_uint param_value=0;
    cl_int ret=mo->getMemory()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }

    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  case CL_MEM_SIZE:
  case CL_MEM_OFFSET: {
    std:size_t param_value=0;
    cl_int ret=mo->getMemory()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }

    return scope.Close(JS_INT(param_value));
  }
  case CL_MEM_ASSOCIATED_MEMOBJECT: {
    cl::Memory *param_value=new cl::Memory();
    cl_int ret=mo->getMemory()->getInfo(param_name,param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }

    return scope.Close(WebCLMemory::New(param_value)->handle_);
  }
  /* TODO case CL_MEM_HOST_PTR: {
    char *ptr = *((char**)param_value);
    param_name = CL_MEM_SIZE;
    ret = MemoryObjectWrapper::memoryObjectInfoHelper(mo->getMemoryObject(),
        param_name,
        sizeof(param_value),
        param_value,
        &param_value_size_ret);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    size_t nbytes = *(size_t*)param_value;
    return scope.Close(node::Buffer::New(ptr, nbytes)->handle_);
  }*/
  default:
    return ThrowException(Exception::Error(String::New("UNKNOWN param_name")));
  }
}

/* static  */
JS_METHOD(WebCLMemory::getImageInfo)
{
  HandleScope scope;
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args.This());
  cl_mem_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_IMAGE_ELEMENT_SIZE:
  case CL_IMAGE_ROW_PITCH:
  case CL_IMAGE_SLICE_PITCH:
  case CL_IMAGE_WIDTH:
  case CL_IMAGE_HEIGHT:
  case CL_IMAGE_DEPTH: {
    std::size_t param_value=0;
    cl_int ret=((cl::Image*) mo->getMemory())->getImageInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_IMAGE_FORMAT: {
    cl::ImageFormat param_value;
    cl_int ret=((cl::Image*) mo->getMemory())->getImageInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
      WEBCL_COND_RETURN_THROW(CL_INVALID_MEM_OBJECT);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
      WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
    }
    cl_channel_order channel_order = param_value.image_channel_order;
    cl_channel_type channel_type = param_value.image_channel_data_type;
    Local<Object> obj = Object::New();
    obj->Set(JS_STR("order"), JS_INT(channel_order));
    obj->Set(JS_STR("data_type"), JS_INT(channel_type));
    return scope.Close(obj);
  }
  default:
    return ThrowException(Exception::Error(String::New("UNKNOWN param_name")));
  }
}

/* static  */
JS_METHOD(WebCLMemory::createSubBuffer)
{
  HandleScope scope;
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args.This());
  cl_mem_flags flags = args[0]->Uint32Value();
  cl_buffer_create_type buffer_create_type = args[1]->Uint32Value();

  if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)
    return ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));

  cl_buffer_region region;
  Local<Object> obj = args[1]->ToObject();
  region.origin = obj->Get(JS_STR("origin"))->Uint32Value();
  region.size = obj->Get(JS_STR("size"))->Uint32Value();

  cl_int ret=CL_SUCCESS;
  cl::Buffer sub_buffer=((cl::Buffer*) mo->getMemory())->createSubBuffer(flags,buffer_create_type,&region,&ret);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_BUFFER_SIZE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_HOST_PTR);
    WEBCL_COND_RETURN_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLMemory::New(new cl::Buffer(sub_buffer))->handle_);
}

/* static  */
JS_METHOD(WebCLMemory::New)
{
  HandleScope scope;
  WebCLMemory *cl = new WebCLMemory(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLMemory *WebCLMemory::New(cl::Memory* mw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLMemory *memobj = ObjectWrap::Unwrap<WebCLMemory>(obj);
  memobj->memory = mw;

  return memobj;
}
