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

#include "memoryobject.h"
#include <node_buffer.h>

using namespace v8;
using namespace node;

namespace webcl {

Persistent<FunctionTemplate> MemoryObject::constructor_template;

void MemoryObject::Init(Handle<Object> target)
{
  NanScope();

  Local<FunctionTemplate> t = FunctionTemplate::New(MemoryObject::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(NanSymbol("WebCLMemoryObject"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLObjectInfo", getGLObjectInfo);
  // Patch
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(NanSymbol("WebCLMemoryObject"), constructor_template->GetFunction());
}

MemoryObject::MemoryObject(Handle<Object> wrapper) : memory(0)
{
}

void MemoryObject::Destructor() {
  #ifdef LOGGING
  cout<<"  Destroying CL memory object"<<endl;
  #endif
  if(memory) ::clReleaseMemObject(memory);
  memory=0;
}

NAN_METHOD(MemoryObject::release)
{
  NanScope();

  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args.This());
  DESTROY_WEBCL_OBJECT(mo);
  
  NanReturnUndefined();
}

NAN_METHOD(MemoryObject::getInfo)
{
  NanScope();

  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args.This());
  cl_mem_info param_name = args[0]->Uint32Value();

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
      return NanThrowError("UNKNOWN ERROR");
    }

    NanReturnValue(Integer::NewFromUnsigned(param_value));
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
      return NanThrowError("UNKNOWN ERROR");
    }

    NanReturnValue(JS_INT((int32_t)param_value));
  }
  case CL_MEM_ASSOCIATED_MEMOBJECT: {
    cl_mem param_value=NULL;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(size_t), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    NanReturnValue(MemoryObject::New(param_value)->handle_);
  }
  case CL_MEM_HOST_PTR: {
    char *param_value=NULL;
    cl_int ret=::clGetMemObjectInfo(mo->getMemory(),param_name,sizeof(char*), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    size_t nbytes = *(size_t*)param_value;
    NanReturnValue(node::Buffer::New(param_value, nbytes)->handle_);
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

NAN_METHOD(MemoryObject::getGLObjectInfo)
{
  NanScope();
  MemoryObject *memobj = UnwrapThis<MemoryObject>(args);
  cl_gl_object_type gl_object_type = args[0]->IsNull() ? 0 : args[0]->Uint32Value();
  cl_GLuint gl_object_name = args[1]->IsNull() ? 0 : args[1]->Uint32Value();
  int ret = ::clGetGLObjectInfo(memobj->getMemory(),
                                gl_object_type==0 ? NULL : &gl_object_type,
                                gl_object_name==0 ? NULL : &gl_object_name);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  Local<Array> arr=Array::New();
  arr->Set(JS_STR("gl_object_type"), JS_INT(gl_object_type));
  arr->Set(JS_STR("gl_object_name"), JS_INT(gl_object_name));

  NanReturnValue(arr);
}

NAN_METHOD(MemoryObject::New)
{
  NanScope();
  MemoryObject *mo = new MemoryObject(args.This());
  mo->Wrap(args.This());
  registerCLObj(mo);
  NanReturnValue(args.This());
}

MemoryObject *MemoryObject::New(cl_mem mw)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  MemoryObject *memobj = ObjectWrap::Unwrap<MemoryObject>(obj);
  memobj->memory = mw;

  return memobj;
}

///////////////////////////////////////////////////////////////////////////////
// WebCLBuffer
///////////////////////////////////////////////////////////////////////////////

Persistent<FunctionTemplate> WebCLBuffer::constructor_template;

void WebCLBuffer::Init(Handle<Object> target)
{
  NanScope();

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLBuffer::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(NanSymbol("WebCLBuffer"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createSubBuffer", createSubBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(NanSymbol("WebCLBuffer"), constructor_template->GetFunction());
}

WebCLBuffer::WebCLBuffer(Handle<Object> wrapper) : MemoryObject(wrapper)
{
}

NAN_METHOD(WebCLBuffer::release)
{
  NanScope();

  MemoryObject *mo = (MemoryObject*) ObjectWrap::Unwrap<WebCLBuffer>(args.This());
  DESTROY_WEBCL_OBJECT(mo);
  
  NanReturnUndefined();
}

// CL 1.1
NAN_METHOD(WebCLBuffer::createSubBuffer)
{
  NanScope();
  WebCLBuffer *mo = UnwrapThis<WebCLBuffer>(args);
  cl_mem_flags flags = args[0]->Uint32Value();
  cl_buffer_create_type buffer_create_type = args[1]->Uint32Value();

  if (buffer_create_type != CL_BUFFER_CREATE_TYPE_REGION)
    return NanThrowError("CL_INVALID_VALUE");

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
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(WebCLBuffer::New(sub_buffer)->handle_);
}

NAN_METHOD(WebCLBuffer::New)
{
  NanScope();
  WebCLBuffer *mo = new WebCLBuffer(args.This());
  mo->Wrap(args.This());
  registerCLObj(mo);
  NanReturnValue(args.This());
}

WebCLBuffer *WebCLBuffer::New(cl_mem mw)
{
  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLBuffer *memobj = ObjectWrap::Unwrap<WebCLBuffer>(obj);
  memobj->memory = mw;

  return memobj;
}

///////////////////////////////////////////////////////////////////////////////
// WebCLImage
///////////////////////////////////////////////////////////////////////////////

Persistent<FunctionTemplate> WebCLImage::constructor_template;

void WebCLImage::Init(Handle<Object> target)
{
  NanScope();

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLImage::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(NanSymbol("WebCLImage"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLTextureInfo", getGLTextureInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(NanSymbol("WebCLImage"), constructor_template->GetFunction());
}

WebCLImage::WebCLImage(Handle<Object> wrapper) : MemoryObject(wrapper)
{
}

NAN_METHOD(WebCLImage::release)
{
  NanScope();

  MemoryObject *mo = (MemoryObject*) ObjectWrap::Unwrap<WebCLImage>(args.This());
  DESTROY_WEBCL_OBJECT(mo);
  
  NanReturnUndefined();
}

NAN_METHOD(WebCLImage::getInfo)
{
  NanScope();
  WebCLImage *mo = UnwrapThis<WebCLImage>(args);
  cl_mem_info param_name = args[0]->Uint32Value();

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
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT((int32_t)param_value));
  }
  case CL_IMAGE_FORMAT: {
    cl_image_format param_value;
    cl_int ret=::clGetImageInfo(mo->getMemory(),param_name,sizeof(cl_image_format), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    cl_channel_order channel_order = param_value.image_channel_order;
    cl_channel_type channel_type = param_value.image_channel_data_type;
    Local<Object> obj = Object::New();
    obj->Set(JS_STR("order"), JS_INT(channel_order));
    obj->Set(JS_STR("data_type"), JS_INT(channel_type));
    NanReturnValue(obj);
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

NAN_METHOD(WebCLImage::getGLTextureInfo)
{
  NanScope();
  WebCLImage *memobj = UnwrapThis<WebCLImage>(args);
  cl_gl_texture_info param_name = args[0]->Uint32Value();
  GLint param_value;

  // TODO no other value that GLenum/GLint returned in OpenCL 1.1
  int ret = ::clGetGLTextureInfo(memobj->getMemory(), param_name, sizeof(GLint), &param_value, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(JS_INT(param_value));
}

NAN_METHOD(WebCLImage::New)
{
  NanScope();
  WebCLImage *mo = new WebCLImage(args.This());
  mo->Wrap(args.This());
  registerCLObj(mo);
  NanReturnValue(args.This());
}

WebCLImage *WebCLImage::New(cl_mem mw)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLImage *memobj = ObjectWrap::Unwrap<WebCLImage>(obj);
  memobj->memory = mw;

  return memobj;
}

}
