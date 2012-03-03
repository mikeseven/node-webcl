/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "memoryobject.h"
#include <node_buffer.h>

#if defined (__APPLE__) || defined(MACOSX)
  #include <OpenGL/OpenGL.h>
#else
  #include <GL/gl.h>
  #include <GL/glx.h>
#endif

#include <iostream>
using namespace std;

using namespace v8;
using namespace node;

namespace webcl {

Persistent<FunctionTemplate> MemoryObject::constructor_template;

void MemoryObject::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(MemoryObject::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLMemoryObject"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getImageInfo", getImageInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createSubBuffer", createSubBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLTextureInfo", getGLTextureInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLObjectInfo", getGLObjectInfo);

  target->Set(String::NewSymbol("WebCLMemoryObject"), constructor_template->GetFunction());
}

MemoryObject::MemoryObject(Handle<Object> wrapper) : memory(0)
{
}

void MemoryObject::Destructor() {
  cout<<"  Destroying CL memory object"<<endl;
  if(memory) ::clReleaseMemObject(memory);
  memory=0;
}

JS_METHOD(MemoryObject::getInfo)
{
  HandleScope scope;

  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args.This());
  cl_mem_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_MEM_TYPE:
  case CL_MEM_FLAGS:
  case CL_MEM_REFERENCE_COUNT:
  case CL_MEM_MAP_COUNT: {
    cl_uint param_value=0;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  case CL_MEM_SIZE:
  case CL_MEM_OFFSET: {
    size_t param_value=0;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    return scope.Close(JS_INT(param_value));
  }
  case CL_MEM_ASSOCIATED_MEMOBJECT: {
    cl_mem param_value=NULL;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    return scope.Close(MemoryObject::New(param_value)->handle_);
  }
  case CL_MEM_HOST_PTR: {
    char *param_value=NULL;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(char*), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    size_t nbytes = *(size_t*)param_value;
    return scope.Close(Buffer::New(param_value, nbytes)->handle_);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

JS_METHOD(MemoryObject::getImageInfo)
{
  HandleScope scope;
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args.This());
  cl_mem_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_IMAGE_ELEMENT_SIZE:
  case CL_IMAGE_ROW_PITCH:
  case CL_IMAGE_SLICE_PITCH:
  case CL_IMAGE_WIDTH:
  case CL_IMAGE_HEIGHT:
  case CL_IMAGE_DEPTH: {
    size_t param_value=0;
    cl_int ret=::clGetImageInfo(mo->getMemory(),param_name,sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_IMAGE_FORMAT: {
    cl_image_format param_value;
    cl_int ret=::clGetImageInfo(mo->getMemory(),param_name,sizeof(cl_image_format), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    cl_channel_order channel_order = param_value.image_channel_order;
    cl_channel_type channel_type = param_value.image_channel_data_type;
    Local<Object> obj = Object::New();
    obj->Set(JS_STR("order"), JS_INT(channel_order));
    obj->Set(JS_STR("data_type"), JS_INT(channel_type));
    return scope.Close(obj);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

// CL 1.1
JS_METHOD(MemoryObject::createSubBuffer)
{
  HandleScope scope;
  MemoryObject *mo = UnwrapThis<MemoryObject>(args);
  cl_mem_flags flags = args[0]->Uint32Value();
  cl_buffer_create_type buffer_create_type = args[1]->Uint32Value();

  if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)
    return ThrowError("CL_INVALID_VALUE");

  cl_buffer_region region;
  Local<Object> obj = args[2]->ToObject();
  region.origin = obj->Get(JS_STR("origin"))->Uint32Value();
  region.size = obj->Get(JS_STR("size"))->Uint32Value();

  cl_int ret=CL_SUCCESS;
  cl_mem sub_buffer = ::clCreateSubBuffer(
      mo->getMemory(),
      flags,
      buffer_create_type,
      &region,
      &ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_BUFFER_SIZE);
    REQ_ERROR_THROW(CL_INVALID_HOST_PTR);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(MemoryObject::New(sub_buffer)->handle_);
}

JS_METHOD(MemoryObject::getGLTextureInfo)
{
  HandleScope scope;
  MemoryObject *memobj = UnwrapThis<MemoryObject>(args);
  cl_gl_texture_info param_name = args[0]->NumberValue();
  GLint param_value;

  // TODO no other value that GLenum/GLint returned in OpenCL 1.1
  int ret = ::clGetGLTextureInfo(memobj->getMemory(), param_name, sizeof(GLint), &param_value, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(JS_INT(param_value));
}

JS_METHOD(MemoryObject::getGLObjectInfo)
{
  HandleScope scope;
  MemoryObject *memobj = UnwrapThis<MemoryObject>(args);
  cl_gl_object_type gl_object_type = args[0]->IsNull() ? 0 : args[0]->NumberValue();
  cl_GLuint gl_object_name = args[1]->IsNull() ? 0 : args[1]->NumberValue();
  int ret = ::clGetGLObjectInfo(memobj->getMemory(),
                                gl_object_type==0 ? NULL : &gl_object_type,
                                gl_object_name==0 ? NULL : &gl_object_name);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  Local<Array> arr=Array::New();
  arr->Set(JS_STR("gl_object_type"), JS_INT(gl_object_type));
  arr->Set(JS_STR("gl_object_name"), JS_INT(gl_object_name));

  return scope.Close(arr);
}

JS_METHOD(MemoryObject::New)
{
  HandleScope scope;
  MemoryObject *mo = new MemoryObject(args.This());
  mo->Wrap(args.This());
  registerCLObj(mo);
  return scope.Close(args.This());
}

MemoryObject *MemoryObject::New(cl_mem mw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  MemoryObject *memobj = ObjectWrap::Unwrap<MemoryObject>(obj);
  memobj->memory = mw;

  return memobj;
}

}
