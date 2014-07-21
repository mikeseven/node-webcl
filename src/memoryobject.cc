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

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(MemoryObject::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLMemoryObject"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getGLObjectInfo", getGLObjectInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLMemoryObject"), ctor->GetFunction());
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

    NanReturnValue(NanObjectWrapHandle(MemoryObject::New(param_value)));
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
    NanReturnValue(NanNewBufferHandle(param_value, nbytes));
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

NAN_METHOD(MemoryObject::getGLObjectInfo)
{
  NanScope();
  MemoryObject *memobj = ObjectWrap::Unwrap<MemoryObject>(args.This());
  cl_gl_object_type gl_object_type = 0;
  cl_GLuint gl_object_name = 0;
  int ret = ::clGetGLObjectInfo(memobj->getMemory(), &gl_object_type, &gl_object_name);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  Local<Array> arr=Array::New();
  arr->Set(JS_STR("glObject"), JS_INT(gl_object_name));
  arr->Set(JS_STR("type"), JS_INT(gl_object_type));
  if(gl_object_type==CL_GL_OBJECT_TEXTURE2D || gl_object_type==CL_GL_OBJECT_TEXTURE3D) {
    int textureTarget=0, mipmapLevel=0;
    ::clGetGLTextureInfo(memobj->getMemory(),CL_GL_TEXTURE_TARGET,sizeof(GLenum),&textureTarget,NULL);
    ::clGetGLTextureInfo(memobj->getMemory(),CL_GL_MIPMAP_LEVEL,sizeof(GLint),&mipmapLevel,NULL);
    arr->Set(JS_STR("textureTarget"), JS_INT(textureTarget));
    arr->Set(JS_STR("mipmapLevel"), JS_INT(mipmapLevel));
  }

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
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

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

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(WebCLBuffer::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLBuffer"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_createSubBuffer", createSubBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLBuffer"), ctor->GetFunction());
}

WebCLBuffer::WebCLBuffer(Handle<Object> wrapper) : MemoryObject(wrapper)
{
}

NAN_METHOD(WebCLBuffer::getInfo)
{
  return MemoryObject::getInfo(args);
}

NAN_METHOD(WebCLBuffer::getGLObjectInfo)
{
  return MemoryObject::getGLObjectInfo(args);
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
  WebCLBuffer *mo = ObjectWrap::Unwrap<WebCLBuffer>(args.This());
  cl_mem_flags flags = args[0]->Uint32Value();

  cl_buffer_region region;
  region.origin = args[1]->Uint32Value();
  region.size = args[2]->Uint32Value();

  cl_int ret=CL_SUCCESS;
  cl_mem sub_buffer = ::clCreateSubBuffer(
      mo->getMemory(),
      flags,
      CL_BUFFER_CREATE_TYPE_REGION,
      &region,
      &ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_BUFFER_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(NanObjectWrapHandle(WebCLBuffer::New(sub_buffer)));
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
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

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

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(WebCLImage::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLImage"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getGLTextureInfo", getGLTextureInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLImage"), ctor->GetFunction());
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
  WebCLImage *mo = ObjectWrap::Unwrap<WebCLImage>(args.This());;

  Local<Object> obj = Object::New();
  cl_image_format param_value;
  cl_int ret=::clGetImageInfo(mo->getMemory(),CL_IMAGE_FORMAT,sizeof(cl_image_format), &param_value, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  obj->Set(JS_STR("channelOrder"), JS_INT(param_value.image_channel_order));
  obj->Set(JS_STR("channelType"), JS_INT(param_value.image_channel_data_type));

  size_t value=0;
  ret |= ::clGetImageInfo(mo->getMemory(),CL_IMAGE_WIDTH,sizeof(size_t), &value, NULL);
  obj->Set(JS_STR("width"), JS_INT(value));
  ret |= ::clGetImageInfo(mo->getMemory(),CL_IMAGE_HEIGHT,sizeof(size_t), &value, NULL);
  obj->Set(JS_STR("height"), JS_INT(value));
  ret |= ::clGetImageInfo(mo->getMemory(),CL_IMAGE_DEPTH,sizeof(size_t), &value, NULL);
  obj->Set(JS_STR("depth"), JS_INT(value));
  ret |= ::clGetImageInfo(mo->getMemory(),CL_IMAGE_ROW_PITCH,sizeof(size_t), &value, NULL);
  obj->Set(JS_STR("rowPitch"), JS_INT(value));
  ret |= ::clGetImageInfo(mo->getMemory(),CL_IMAGE_SLICE_PITCH,sizeof(size_t), &value, NULL);
  obj->Set(JS_STR("slicePitch"), JS_INT(value));

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(obj);
}

NAN_METHOD(WebCLImage::getGLObjectInfo)
{
  return MemoryObject::getGLObjectInfo(args);
}

NAN_METHOD(WebCLImage::getGLTextureInfo)
{
  NanScope();
  WebCLImage *memobj = ObjectWrap::Unwrap<WebCLImage>(args.This());;
  cl_gl_texture_info param_name = args[0]->Uint32Value();
  GLint param_value;

  // TODO no other value that GLenum/GLint returned in OpenCL 1.1
  int ret = ::clGetGLTextureInfo(memobj->getMemory(), param_name, sizeof(GLint), &param_value, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
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
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);
 
  WebCLImage *memobj = ObjectWrap::Unwrap<WebCLImage>(obj);
  memobj->memory = mw;

  return memobj;
}

}
