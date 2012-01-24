/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "ext_gl.h"
#include "commandqueue.h"
#include "context.h"
#include "memoryobject.h"
#include "event.h"

#include <GL/gl.h>
#include <CL/cl_gl.h>

#include <iostream>
using namespace std;

using namespace node;
using namespace v8;

namespace webcl {

#define MakeEventWaitList(arg) \
    cl_event *events_wait_list=NULL; \
    cl_uint num_events_wait_list=0; \
    if(!arg->IsUndefined() && !arg->IsNull()) { \
      Local<Array> arr = Array::Cast(*arg); \
      num_events_wait_list=arr->Length(); \
      events_wait_list=new cl_event[num_events_wait_list]; \
      for(int i=0;i<num_events_wait_list;i++) \
        events_wait_list[i]=ObjectWrap::Unwrap<Event>(arr->Get(i)->ToObject())->getEvent(); \
    }

Persistent<FunctionTemplate> EXTGL::constructor_template;

void EXTGL::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(EXTGL::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLEXTGL"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createFromGLBuffer", createFromGLBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createFromGLTexture2D", createFromGLTexture2D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createFromGLTexture3D", createFromGLTexture3D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_createFromGLRenderbuffer", createFromGLRenderbuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLObjectInfo", getGLObjectInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getGLTextureInfo", getGLTextureInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueAcquireGLObjects", enqueueAcquireGLObjects);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueReleaseGLObjects", enqueueReleaseGLObjects);

  target->Set(String::NewSymbol("WebCLEXTGL"), constructor_template->GetFunction());
}

EXTGL::EXTGL(Handle<Object> wrapper)
{
}

JS_METHOD(EXTGL::createFromGLBuffer)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  Context *context = ObjectWrap::Unwrap<Context>(args[0]->ToObject());
  cl_mem_flags flags = args[1]->NumberValue();
  cl_GLuint bufobj = args[2]->NumberValue();
  int ret;
  cl_mem clmem = ::clCreateFromGLBuffer(context->getContext(),flags,bufobj,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(MemoryObject::New(clmem)->handle_);
}

JS_METHOD(EXTGL::createFromGLTexture2D)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  Context *context = ObjectWrap::Unwrap<Context>(args[0]->ToObject());
  cl_mem_flags flags = args[1]->NumberValue();
  cl_GLenum target = args[2]->NumberValue();
  cl_GLint miplevel = args[3]->NumberValue();
  cl_GLuint texture = args[4]->NumberValue();
  int ret;
  cl_mem clmem = ::clCreateFromGLTexture2D(context->getContext(),flags,target,miplevel,texture,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(MemoryObject::New(clmem)->handle_);
}

JS_METHOD(EXTGL::createFromGLTexture3D)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  Context *context = ObjectWrap::Unwrap<Context>(args[0]->ToObject());
  cl_mem_flags flags = args[1]->NumberValue();
  cl_GLenum target = args[2]->NumberValue();
  cl_GLint miplevel = args[3]->NumberValue();
  cl_GLuint texture = args[4]->NumberValue();
  int ret;
  cl_mem clmem = ::clCreateFromGLTexture3D(context->getContext(),flags,target,miplevel,texture,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(MemoryObject::New(clmem)->handle_);
}

JS_METHOD(EXTGL::createFromGLRenderbuffer)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  Context *context = ObjectWrap::Unwrap<Context>(args[0]->ToObject());
  cl_mem_flags flags = args[1]->NumberValue();
  cl_GLuint renderbuffer = args[2]->NumberValue();
  int ret;
  cl_mem clmem = ::clCreateFromGLRenderbuffer(context->getContext(),flags,renderbuffer, &ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(MemoryObject::New(clmem)->handle_);
}

JS_METHOD(EXTGL::getGLObjectInfo)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  MemoryObject *memobj = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  cl_gl_object_type gl_object_type = args[1]->IsNull() ? 0 : args[1]->NumberValue();
  cl_GLuint gl_object_name = args[2]->IsNull() ? 0 : args[2]->NumberValue();
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

JS_METHOD(EXTGL::getGLTextureInfo)
{
  HandleScope scope;
  EXTGL *extgl = UnwrapThis<EXTGL>(args);
  MemoryObject *memobj = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  cl_gl_texture_info param_name = args[1]->NumberValue();
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

JS_METHOD(EXTGL::enqueueAcquireGLObjects)
{
  HandleScope scope;
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args[0]->ToObject());

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[1]->IsArray()) {
    Local<Array> mem_objects_arr= Array::Cast(*args[1]);
    num_objects=mem_objects_arr->Length();
    mem_objects=new cl_mem[num_objects];
    for(int i=0;i<num_objects;i++) {
      MemoryObject *obj=ObjectWrap::Unwrap<MemoryObject>(mem_objects_arr->Get(i)->ToObject());
      mem_objects[i]=obj->getMemory();
    }
  }
  else if(args[1]->IsObject()) {
    num_objects=1;
    mem_objects=new cl_mem[1];
    mem_objects[0]=ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject())->getMemory();
  }

  MakeEventWaitList(args[2]);

  cl_event event=NULL;
  bool generate_event = !args[3]->IsUndefined() && args[3]->BooleanValue();

  int ret = ::clEnqueueAcquireGLObjects(cq->getCommandQueue(),
      num_objects, mem_objects,
      num_events_wait_list,
      events_wait_list,
      generate_event ? &event : NULL);

  if(mem_objects) delete[] mem_objects;
  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(event) return scope.Close(Event::New(event)->handle_);
  return Undefined();
}

JS_METHOD(EXTGL::enqueueReleaseGLObjects)
{
  HandleScope scope;
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args[0]->ToObject());

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[1]->IsArray()) {
    Local<Array> mem_objects_arr= Array::Cast(*args[1]);
    num_objects=mem_objects_arr->Length();
    mem_objects=new cl_mem[num_objects];
    for(int i=0;i<num_objects;i++) {
      MemoryObject *obj=ObjectWrap::Unwrap<MemoryObject>(mem_objects_arr->Get(i)->ToObject());
      mem_objects[i]=obj->getMemory();
    }
  }
  else if(args[1]->IsObject()) {
    num_objects=1;
    mem_objects=new cl_mem[1];
    mem_objects[0]=ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject())->getMemory();
  }

  MakeEventWaitList(args[2]);

  cl_event event=NULL;
  bool generate_event = !args[3]->IsUndefined() && args[3]->BooleanValue();

  int ret = ::clEnqueueReleaseGLObjects(cq->getCommandQueue(),
      num_objects, mem_objects,
      num_events_wait_list,
      events_wait_list,
      generate_event ? &event : NULL);

  if(mem_objects) delete[] mem_objects;
  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_GL_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(event) {
    //cout<<"[enqueueReleaseGLObjects] event "<<event<<endl;
    return scope.Close(Event::New(event)->handle_);
  }
  return Undefined();
}

JS_METHOD(EXTGL::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  EXTGL *gl = new EXTGL(args.This());
  gl->Wrap(args.This());
  return scope.Close(args.This());
}

EXTGL *EXTGL::New()
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  EXTGL *gl = ObjectWrap::Unwrap<EXTGL>(obj);

  return gl;
}

}
