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

#include "commandqueue.h"
#include "platform.h"
#include "context.h"
#include "device.h"
#include "memoryobject.h"
#include "event.h"
#include "kernel.h"
#include <vector>
#include <node_buffer.h>
#include <cstring> // for memcpy

using namespace v8;
using namespace node;

namespace webcl {
#define MakeEventWaitList(arg) \
    cl_event *events_wait_list=NULL; \
    cl_uint num_events_wait_list=0; \
    if(!arg->IsUndefined() && !arg->IsNull()) { \
      Local<Array> arr = Local<Array>::Cast(arg); \
      num_events_wait_list=arr->Length(); \
      if(num_events_wait_list>0) {\
        events_wait_list=new cl_event[num_events_wait_list]; \
        for(cl_uint i=0;i<num_events_wait_list;i++) \
          events_wait_list[i]=ObjectWrap::Unwrap<Event>(arr->Get(i)->ToObject())->getEvent(); \
      }\
    }

Persistent<FunctionTemplate> CommandQueue::constructor_template;

void CommandQueueCB(Persistent<Value> value, void *param) {
#ifdef LOGGING
  String::AsciiValue str(value->ToObject()->GetConstructorName());
  printf("%s weak ref cb\n", *str);
#endif
  value.Dispose();
}

void CommandQueue::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(CommandQueue::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLCommandQueue"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueNDRangeKernel", enqueueNDRangeKernel);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueTask", enqueueTask);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueWriteBuffer", enqueueWriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueReadBuffer", enqueueReadBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueCopyBuffer", enqueueCopyBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueWriteBufferRect", enqueueWriteBufferRect);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueReadBufferRect", enqueueReadBufferRect);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueCopyBufferRect", enqueueCopyBufferRect);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueWriteImage", enqueueWriteImage);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueReadImage", enqueueReadImage);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueCopyImage", enqueueCopyImage);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueCopyImageToBuffer", enqueueCopyImageToBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueCopyBufferToImage", enqueueCopyBufferToImage);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueMapBuffer", enqueueMapBuffer);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueMapImage", enqueueMapImage);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueUnmapMemObject", enqueueUnmapMemObject);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueMarker", enqueueMarker);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueWaitForEvents", enqueueWaitForEvents);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueBarrier", enqueueBarrier);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_flush", flush);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_finish", finish);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueAcquireGLObjects", enqueueAcquireGLObjects);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_enqueueReleaseGLObjects", enqueueReleaseGLObjects);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLCommandQueue"), ctor->GetFunction());
  constructor_template.MakeWeak(NULL, CommandQueueCB);
}

CommandQueue::CommandQueue(Handle<Object> wrapper) : command_queue(0)
{
  _type=CLObjType::CommandQueue;
}

CommandQueue::~CommandQueue() {
#ifdef LOGGING
  printf("In ~CommandQueue\n");
#endif
  // Destructor();
}

void CommandQueue::Destructor() {
  if(command_queue) {
    cl_uint count;
    ::clGetCommandQueueInfo(command_queue,CL_QUEUE_REFERENCE_COUNT,sizeof(cl_uint),&count,NULL);
#ifdef LOGGING
    cout<<"  Destroying CommandQueue, CLrefCount is: "<<count<<endl;
#endif
    ::clReleaseCommandQueue(command_queue);
    if(count==1) {
      unregisterCLObj(this);
      command_queue=0;
    }
  }
}

NAN_METHOD(CommandQueue::release)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  
  // Flush first
  cl_int ret = ::clFlush(cq->getCommandQueue());

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }
  
  cq->Destructor();
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::getInfo)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  cl_command_queue_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_QUEUE_CONTEXT: {
    cl_context ctx=0;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_context), &ctx, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    if(ctx) {
      WebCLObject *obj=findCLObj((void*)ctx, CLObjType::Context);
      if(obj) 
        NanReturnValue(NanObjectWrapHandle(obj));
      else
        NanReturnValue(NanObjectWrapHandle(Context::New(ctx)));
    }
    NanReturnUndefined();
  }
  case CL_QUEUE_DEVICE: {
    cl_device_id dev=0;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_device_id), &dev, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    if(dev) {
      WebCLObject *obj=findCLObj((void*)dev, CLObjType::Device);

      if(obj) 
        NanReturnValue(NanObjectWrapHandle(obj));
      else
        NanReturnValue(NanObjectWrapHandle(Device::New(dev)));
    }
    NanReturnUndefined();
  }
  // case CL_QUEUE_REFERENCE_COUNT:
  case CL_QUEUE_PROPERTIES: {
    cl_command_queue_properties param_value;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_command_queue_properties), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(INVALID_VALUE);
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

NAN_METHOD(CommandQueue::enqueueNDRangeKernel)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  REQ_ARGS(5);

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args[0]->ToObject());
  int workDim = args[1]->Uint32Value();

  size_t *offsets=NULL;
  cl_uint num_offsets=0;
  if(!args[2]->IsUndefined() && !args[2]->IsNull()) {
    Local<Array> arr = Local<Array>::Cast(args[2]);
    num_offsets=arr->Length();
    if (num_offsets > 0) {
      offsets = new size_t[num_offsets];
      for (cl_uint i = 0; i < num_offsets; i++)
        offsets[i] = arr->Get(i)->Uint32Value();
    }
  }

  size_t *globals=NULL;
  cl_uint num_globals=0;
  if(!args[3]->IsUndefined() && !args[3]->IsNull()) {
    Local<Array> arr = Local<Array>::Cast(args[3]);
    num_globals=arr->Length();
    if(num_globals == 0)
      NanThrowError("# globals must be at least 1");
    globals=new size_t[num_globals];
    for(cl_uint i=0;i<num_globals;i++)
      globals[i]=arr->Get(i)->Uint32Value();
  }

  size_t *locals=NULL;
  cl_uint num_locals=0;
  if(!args[4]->IsUndefined() && !args[4]->IsNull()) {
    Local<Array> arr = Local<Array>::Cast(args[4]);
    num_locals=arr->Length();
    if(num_locals == 0)
      NanThrowError("# locals must be at least 1");
    locals=new size_t[num_locals];
    for(cl_uint i=0;i<num_locals;i++)
      locals[i]=arr->Get(i)->Uint32Value();
  }

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event=(args[6]->IsUndefined()  || args[6]->IsNull());

  cl_int ret=::clEnqueueNDRangeKernel(
      cq->getCommandQueue(), kernel->getKernel(),
      workDim, // work dimension
      offsets,
      globals,
      locals,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(offsets) delete[] offsets;
  if(globals) delete[] globals;
  if(locals) delete[] locals;
  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_KERNEL);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_KERNEL_ARGS);
    REQ_ERROR_THROW(INVALID_WORK_DIMENSION);
    REQ_ERROR_THROW(INVALID_GLOBAL_WORK_SIZE);
    REQ_ERROR_THROW(INVALID_GLOBAL_OFFSET);
    REQ_ERROR_THROW(INVALID_WORK_GROUP_SIZE);
    REQ_ERROR_THROW(INVALID_WORK_ITEM_SIZE);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueTask)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  REQ_ARGS(1);

  Kernel *k = ObjectWrap::Unwrap<Kernel>(args[0]->ToObject());

  MakeEventWaitList(args[1]);

  cl_event event;
  bool no_event = (args[2]->IsUndefined() || args[2]->IsNull());

  cl_int ret=::clEnqueueTask(
      cq->getCommandQueue(), k->getKernel(),
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_KERNEL);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_KERNEL_ARGS);
    REQ_ERROR_THROW(INVALID_WORK_GROUP_SIZE);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueWriteBuffer)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  uint32_t offset = args[2]->Uint32Value();
  uint32_t size = args[3]->Uint32Value();

  void *ptr=NULL;
  if(!args[4]->IsUndefined()) {
    if(args[4]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[4]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[4]->IsObject()) {
      Local<Object> obj=args[4]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData();
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueWriteBuffer(
                  cq->getCommandQueue(), mo->getMemory(), blocking_write, offset, size,
                  ptr,
                  num_events_wait_list,
                  events_wait_list,
                  no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueWriteBufferRect)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t buffer_origin[3] = {0,0,0};
  size_t host_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      buffer_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      host_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  uint32_t buffer_row_pitch=args[5]->Uint32Value();
  uint32_t buffer_slice_pitch=args[6]->Uint32Value();
  uint32_t host_row_pitch=args[7]->Uint32Value();
  uint32_t host_slice_pitch=args[8]->Uint32Value();

  void *ptr=NULL;
  if(!args[9]->IsUndefined()) {
    if(args[9]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[9]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[9]->IsObject()) {
      Local<Object> obj=args[9]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData();
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[10]);

  cl_event event;
  bool no_event = (args[11]->IsUndefined() || args[11]->IsNull());

  cl_int ret=::clEnqueueWriteBufferRect(
      cq->getCommandQueue(),
      mo->getMemory(),
      blocking_write,
      buffer_origin,
      host_origin,
      region,
      buffer_row_pitch,
      buffer_slice_pitch,
      host_row_pitch,
      host_slice_pitch,
      ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[11]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueReadBuffer)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  uint32_t offset=args[2]->Uint32Value();
  uint32_t size=args[3]->Uint32Value();

  void *ptr=NULL;
  if(!args[4]->IsUndefined()) {
    if(args[4]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[4]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[4]->IsObject()) {
      Local<Object> obj=args[4]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData();
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueReadBuffer(
      cq->getCommandQueue(), mo->getMemory(), blocking_read, offset, size,
      ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueReadBufferRect)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t buffer_origin[3] = {0,0,0};
  size_t host_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      buffer_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      host_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  uint32_t buffer_row_pitch=args[5]->Uint32Value();
  uint32_t buffer_slice_pitch=args[6]->Uint32Value();
  uint32_t host_row_pitch=args[7]->Uint32Value();
  uint32_t host_slice_pitch=args[8]->Uint32Value();

  void *ptr=NULL;
  if(!args[9]->IsUndefined()) {
    if(args[9]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[9]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[9]->IsObject()) {
      Local<Object> obj=args[9]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData(); 
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[10]);

  cl_event event;
  bool no_event = (args[11]->IsUndefined() || args[11]->IsNull());

  cl_int ret=::clEnqueueReadBufferRect(
      cq->getCommandQueue(),
      mo->getMemory(),
      blocking_read,
      buffer_origin,
      host_origin,
      region,
      buffer_row_pitch,
      buffer_slice_pitch,
      host_row_pitch,
      host_slice_pitch,
      ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[5]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueCopyBuffer)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_offset = args[2]->Uint32Value();
  size_t dst_offset = args[3]->Uint32Value();
  size_t size = args[4]->Uint32Value();

  MakeEventWaitList(args[5]);

  cl_event event=NULL;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueCopyBuffer(
      cq->getCommandQueue(), mo_src->getMemory(), mo_dst->getMemory(),
      src_offset, dst_offset, size,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueCopyBufferRect)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t dst_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t src_row_pitch = args[5]->Uint32Value();
  size_t src_slice_pitch = args[6]->Uint32Value();
  size_t dst_row_pitch = args[7]->Uint32Value();
  size_t dst_slice_pitch = args[8]->Uint32Value();

  MakeEventWaitList(args[9]);

  cl_event event=NULL;
  bool no_event = (args[10]->IsUndefined() || args[10]->IsNull());

  cl_int ret=::clEnqueueCopyBufferRect(
      cq->getCommandQueue(),
      mo_src->getMemory(),
      mo_dst->getMemory(),
      src_origin,
      dst_origin,
      region,
      src_row_pitch,
      src_slice_pitch,
      dst_row_pitch,
      dst_slice_pitch,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

 if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[10]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueWriteImage)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t origin[3]={0,0,0};
  size_t region[3]={1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      origin[i]=arr->Get(i)->Uint32Value();

  arr= Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t row_pitch = args[4]->Uint32Value();
  size_t slice_pitch = 0; //args[5]->Uint32Value(); // no slice_pitch in WebCL 1.0

  void *ptr=NULL;
  if(!args[5]->IsUndefined()) {
    if(args[5]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[5]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[5]->IsObject()) {
      Local<Object> obj=args[5]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData();
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[6]);

  cl_event event;
  bool no_event = (args[7]->IsUndefined() || args[7]->IsNull());

  cl_int ret=::clEnqueueWriteImage(
      cq->getCommandQueue(), mo->getMemory(), blocking_write,
      origin,
      region,
      row_pitch,
      slice_pitch,
      ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[7]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueReadImage)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t row_pitch = args[4]->Uint32Value();
  size_t slice_pitch = 0;

  void *ptr=NULL;
  if(!args[5]->IsUndefined()) {
    if(args[5]->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(args[5]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[5]->IsObject()) {
      Local<Object> obj=args[5]->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name))
        ptr=Buffer::Data(obj);
      else
        ptr = obj->GetIndexedPropertiesExternalArrayData();
    }
    else
      NanThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[6]);

  cl_event event;
  bool no_event = (args[7]->IsUndefined() || args[7]->IsNull());

  cl_int ret=::clEnqueueReadImage(
      cq->getCommandQueue(), mo->getMemory(), blocking_read,
      origin,
      region,
      row_pitch, slice_pitch, 
      ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[8]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueCopyImage)
{   
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t dst_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueCopyImage(
      cq->getCommandQueue(), mo_src->getMemory(), mo_dst->getMemory(),
      src_origin,
      dst_origin,
      region,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(IMAGE_FORMAT_MISMATCH);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueCopyImageToBuffer)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Local<Array>::Cast(args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[3]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t dst_offset = args[4]->Uint32Value();

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueCopyImageToBuffer(
      cq->getCommandQueue(), mo_src->getMemory(), mo_dst->getMemory(),
      (const size_t*) src_origin,
      (const size_t*) region,
      dst_offset,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueCopyBufferToImage)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_offset = args[2]->Uint32Value();
  size_t dst_origin[3]={0,0,0};
  size_t region[3]={1,1,1};

  Local<Array> arr=Local<Array>::Cast(args[3]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Local<Array>::Cast(args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  MakeEventWaitList(args[5]);

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=::clEnqueueCopyBufferToImage(
      cq->getCommandQueue(), mo_src->getMemory(), mo_dst->getMemory(),
      src_offset,
      dst_origin,
      region,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

void free_callback(char *data, void *hint) {
  //cout<<"enqueueMapBuffer free_callback called"<<endl;
}

NAN_METHOD(CommandQueue::enqueueMapBuffer)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  // TODO: arg checking
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  cl_bool blocking = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_map_flags flags = args[2]->Uint32Value();
  size_t offset = args[3]->Uint32Value();
  size_t size = args[4]->Uint32Value();

  MakeEventWaitList(args[5]);

  cl_int ret=CL_SUCCESS;
  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  void *result=::clEnqueueMapBuffer(
              cq->getCommandQueue(), mo->getMemory(),
              blocking, flags, offset, size,
              num_events_wait_list,
              events_wait_list,
              no_event ? NULL : &event, &ret);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(MAP_FAILURE);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  // cl_mem mem=mo->getMemory();
  // void *host_ptr=NULL;
  // ::clGetMemObjectInfo(mem,CL_MEM_HOST_PTR,sizeof(void*),host_ptr,NULL);
  // printf("\n[map] memobject %p, host_ptr %p",mem, host_ptr);

  // printf("\n[map] result = ");
  // for (size_t i = 0; i < size; ++i)
  // {
  //   printf("%d ",((uint8_t*)result)[i]);
  // }
  // printf("\n");

  // wrap OpenCL result buffer into a node Buffer
  // WARNING: make sure result is shared not copied, otherwise unmapBuffer won't work
  // node::Buffer *buf=node::Buffer::New((char*) result,size, free_callback, NULL);
  Local<Object> buf=NanNewBufferHandle((char*) result, size, free_callback, NULL);
  // printf("[map] result %p, wrapped data %p\n", result, node::Buffer::Data(buf));
  if(node::Buffer::Data(buf) != result) {
    printf("WARNING: data buffer has been copied\n");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }

  NanReturnValue(buf);
}

NAN_METHOD(CommandQueue::enqueueMapImage)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  // TODO: arg checking
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  cl_bool blocking = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_map_flags flags = args[2]->Uint32Value();

  size_t origin[3];
  size_t region[3];

  Local<Array> originArray = Local<Array>::Cast(args[3]);
  for (int i=0; i<3; i++) {
    size_t s = originArray->Get(i)->Uint32Value();
    origin[i] = s;
  }

  Local<Array> regionArray = Local<Array>::Cast(args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->Uint32Value();
    region[i] = s;
  }

  MakeEventWaitList(args[5]);

  size_t row_pitch;
  size_t slice_pitch;

  cl_event event;
  bool no_event = (args[6]->IsUndefined() || args[6]->IsNull());

  cl_int ret=CL_SUCCESS;
  void *result=::clEnqueueMapImage(
              cq->getCommandQueue(), mo->getMemory(),
              blocking, flags,
              origin,
              region,
              &row_pitch, &slice_pitch,
              num_events_wait_list,
              events_wait_list,
              no_event ? NULL : &event, &ret);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(MAP_FAILURE);
    REQ_ERROR_THROW(EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  // TODO: return image_row_pitch, image_slice_pitch?

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }

  size_t nbytes = region[0] * region[1] * region[2];
  NanReturnValue(NanNewBufferHandle((char*)result, nbytes, free_callback, NULL));
}

NAN_METHOD(CommandQueue::enqueueUnmapMemObject)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  // TODO: arg checking
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  Local<Object> buf(args[1]->ToObject());

  MakeEventWaitList(args[2]);

  cl_event event;
  bool no_event = (args[3]->IsUndefined() || args[3]->IsNull());

  // printf("[unmap] wrapped data %p, memobject %p\n", node::Buffer::Data(buf),mo->getMemory());
  // printf("[unmap] Before Unmap: ");
  char *data= (char*)node::Buffer::Data(buf);
  // for(int i=0;i<20;i++) {
  //   printf("%d ", data[i]);
  // }
  // printf("\n");

  cl_int ret=::clEnqueueUnmapMemObject(
      cq->getCommandQueue(), mo->getMemory(),
      data,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  // printf("[unmap] After Unmap: ");
  // for(int i=0;i<20;i++) {
  //   printf("%d ",data[i]);
  // }
  // printf("\n");

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[3]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueMarker)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  cl_event event;
  bool no_event = (args[0]->IsUndefined() || args[0]->IsNull());

  cl_int ret = ::clEnqueueMarker(cq->getCommandQueue(), no_event ? NULL : &event);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[0]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueWaitForEvents)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  MakeEventWaitList(args[0]);

  cl_int ret = ::clEnqueueWaitForEvents(
      cq->getCommandQueue(),
      num_events_wait_list,
      events_wait_list);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_EVENT);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueBarrier)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  MakeEventWaitList(args[0]);

  cl_event event=NULL;
  bool no_event = (args[1]->IsUndefined() || args[1]->IsNull());

  cl_int ret = ::clEnqueueBarrier(cq->getCommandQueue());

  if(events_wait_list && ret==CL_SUCCESS) {
    cl_int ret2 = ::clEnqueueWaitForEvents(
        cq->getCommandQueue(),
        num_events_wait_list,
        events_wait_list);

    if (ret2 != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(INVALID_CONTEXT);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_EVENT);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    delete[] events_wait_list;
  }

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event && event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[1]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

class FinishWorker : public NanAsyncWorker {
 public:
  FinishWorker(Baton *baton)
    : NanAsyncWorker(baton->callback), baton_(baton) {
    }

  ~FinishWorker() {
    if(baton_) {
      NanScope();
      // if (baton_->callback) delete baton_->callback;
      // if (!baton_->parent.IsEmpty()) NanDisposePersistent(baton_->parent);
      // if (!baton_->data.IsEmpty()) NanDisposePersistent(baton_->data);
      delete baton_;
    }
  }

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

     // // must return passed data
    Local<Value> argv[] = {
      // NanPersistentToLocal(baton_->parent),  // event
      JS_INT(baton_->error)
    };

    // printf("[async event] callback JS\n");
    callback->Call(1, argv);
  }

  private:
    Baton *baton_;
};


NAN_METHOD(CommandQueue::finish)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  if(!cq->command_queue) {
    cl_int ret=CL_INVALID_COMMAND_QUEUE;
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
  }

  cl_int ret = ::clFinish(cq->getCommandQueue());

  if(args.Length()>0 && args[0]->IsFunction()) {
      Baton *baton=new Baton();
      baton->error=ret;
      baton->callback=new NanCallback(args[0].As<Function>());
      NanAsyncQueueWorker(new FinishWorker(baton));
      NanReturnUndefined();
  }

  else if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::flush)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());
  cl_int ret = ::clFlush(cq->getCommandQueue());

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueAcquireGLObjects)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[0]->IsArray()) {
    Local<Array> mem_objects_arr= Local<Array>::Cast(args[0]);
    num_objects=mem_objects_arr->Length();
    mem_objects=new cl_mem[num_objects];
    for(int i=0;i<num_objects;i++) {
      MemoryObject *obj=ObjectWrap::Unwrap<MemoryObject>(mem_objects_arr->Get(i)->ToObject());
      mem_objects[i]=obj->getMemory();
    }
  }
  else if(args[0]->IsObject()) {
    num_objects=1;
    mem_objects=new cl_mem[1];
    mem_objects[0]=ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject())->getMemory();
  }

  MakeEventWaitList(args[1]);

  cl_event event;
  bool no_event = (args[2]->IsUndefined() || args[2]->IsNull());

  int ret = ::clEnqueueAcquireGLObjects(cq->getCommandQueue(),
      num_objects, mem_objects,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(mem_objects) delete[] mem_objects;
  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_GL_OBJECT);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::enqueueReleaseGLObjects)
{
  NanScope();
  CommandQueue *cq = ObjectWrap::Unwrap<CommandQueue>(args.This());

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[0]->IsArray()) {
    Local<Array> mem_objects_arr= Local<Array>::Cast(args[0]);
    num_objects=mem_objects_arr->Length();
    mem_objects=new cl_mem[num_objects];
    for(int i=0;i<num_objects;i++) {
      MemoryObject *obj=ObjectWrap::Unwrap<MemoryObject>(mem_objects_arr->Get(i)->ToObject());
      mem_objects[i]=obj->getMemory();
    }
  }
  else if(args[0]->IsObject()) {
    num_objects=1;
    mem_objects=new cl_mem[1];
    mem_objects[0]=ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject())->getMemory();
  }

  MakeEventWaitList(args[1]);

  cl_event event;
  bool no_event = (args[2]->IsUndefined() || args[2]->IsNull());

  int ret = ::clEnqueueReleaseGLObjects(cq->getCommandQueue(),
      num_objects, mem_objects,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(mem_objects) delete[] mem_objects;
  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(INVALID_CONTEXT);
    REQ_ERROR_THROW(INVALID_GL_OBJECT);
    REQ_ERROR_THROW(INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  NanReturnUndefined();
}

NAN_METHOD(CommandQueue::New)
{
  NanScope();
  CommandQueue *cq = new CommandQueue(args.This());
  cq->Wrap(args.This());
  NanReturnValue(args.This());
}

CommandQueue *CommandQueue::New(cl_command_queue cw, WebCLObject *parent)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  CommandQueue *commandqueue = ObjectWrap::Unwrap<CommandQueue>(obj);
  commandqueue->command_queue = cw;
  commandqueue->setParent(parent);
  registerCLObj(cw, commandqueue);

  return commandqueue;
}

}
