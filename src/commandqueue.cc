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
      Local<Array> arr = Array::Cast(*arg); \
      num_events_wait_list=arr->Length(); \
      events_wait_list=new cl_event[num_events_wait_list]; \
      for(cl_uint i=0;i<num_events_wait_list;i++) \
        events_wait_list[i]=ObjectWrap::Unwrap<Event>(arr->Get(i)->ToObject())->getEvent(); \
    }

Persistent<FunctionTemplate> CommandQueue::constructor_template;

void CommandQueue::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(CommandQueue::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLCommandQueue"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueNDRangeKernel", enqueueNDRangeKernel);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueTask", enqueueTask);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueWriteBuffer", enqueueWriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueReadBuffer", enqueueReadBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueCopyBuffer", enqueueCopyBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueWriteBufferRect", enqueueWriteBufferRect);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueReadBufferRect", enqueueReadBufferRect);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueCopyBufferRect", enqueueCopyBufferRect);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueWriteImage", enqueueWriteImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueReadImage", enqueueReadImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueCopyImage", enqueueCopyImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueCopyImageToBuffer", enqueueCopyImageToBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueCopyBufferToImage", enqueueCopyBufferToImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueMapBuffer", enqueueMapBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueMapImage", enqueueMapImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueUnmapMemObject", enqueueUnmapMemObject);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueMarker", enqueueMarker);
  //NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueWaitForEvents", enqueueWaitForEvents);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueBarrier", enqueueBarrier);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_flush", flush);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_finish", finish);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueAcquireGLObjects", enqueueAcquireGLObjects);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_enqueueReleaseGLObjects", enqueueReleaseGLObjects);
  // Patch
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_release", release);

  target->Set(String::NewSymbol("WebCLCommandQueue"), constructor_template->GetFunction());
}

CommandQueue::CommandQueue(Handle<Object> wrapper) : command_queue(0)
{
}

void CommandQueue::Destructor() {
  #ifdef LOGGING
  cout<<"  Destroying CL command queue"<<endl;
  #endif
  if(command_queue) {
    cl_uint count;
    ::clGetCommandQueueInfo(command_queue,CL_QUEUE_REFERENCE_COUNT,sizeof(cl_uint),&count,0);
#ifdef LOGGING
    cout<<"CommandQueue ref count is: "<<count<<endl;
#endif
    ::clReleaseCommandQueue(command_queue);
#ifdef LOGGING
    cout<<"CommandQueue released"<<endl;
#endif
    }
  command_queue=0;
}

JS_METHOD(CommandQueue::release)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  
  // Flush first
  cl_int ret = ::clFlush(cq->getCommandQueue());

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }
  
  DESTROY_WEBCL_OBJECT(cq);
  
  return Undefined();
}

JS_METHOD(CommandQueue::getInfo)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  cl_command_queue_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_QUEUE_CONTEXT: {
    cl_context ctx=0;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_context), &ctx, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Context::New(ctx)->handle_);
  }
  case CL_QUEUE_DEVICE: {
    cl_device_id dev=0;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_device_id), &dev, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Device::New(dev)->handle_);
  }
  case CL_QUEUE_REFERENCE_COUNT:
  case CL_QUEUE_PROPERTIES: {
    cl_uint param_value;
    cl_int ret=::clGetCommandQueueInfo(cq->getCommandQueue(), param_name, sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  default:
    return ThrowError("CL_INVALID_VALUE");
  }
}

JS_METHOD(CommandQueue::enqueueNDRangeKernel)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  REQ_ARGS(4);

  Kernel *kernel = ObjectWrap::Unwrap<Kernel>(args[0]->ToObject());

  size_t *offsets=NULL;
  cl_uint num_offsets=0;
  if(!args[1]->IsUndefined() && !args[1]->IsNull()) {
    Local<Array> arr = Array::Cast(*args[1]);
    num_offsets=arr->Length();
    if (num_offsets > 0) {
      offsets = new size_t[num_offsets];
      for (cl_uint i = 0; i < num_offsets; i++)
        offsets[i] = arr->Get(i)->Uint32Value();
    }
  }

  size_t *globals=NULL;
  cl_uint num_globals=0;
  if(!args[2]->IsUndefined() && !args[2]->IsNull()) {
    Local<Array> arr = Array::Cast(*args[2]);
    num_globals=arr->Length();
    if(num_globals == 0)
      ThrowError("# globals must be at least 1");
    globals=new size_t[num_globals];
    for(cl_uint i=0;i<num_globals;i++)
      globals[i]=arr->Get(i)->Uint32Value();
  }

  size_t *locals=NULL;
  cl_uint num_locals=0;
  if(!args[3]->IsUndefined() && !args[3]->IsNull()) {
    Local<Array> arr = Array::Cast(*args[3]);
    num_locals=arr->Length();
    if(num_locals == 0)
      ThrowError("# locals must be at least 1");
    locals=new size_t[num_locals];
    for(cl_uint i=0;i<num_locals;i++)
      locals[i]=arr->Get(i)->Uint32Value();
  }

  MakeEventWaitList(args[4]);

  cl_event event;
  bool no_event=(args[5]->IsUndefined()  || args[5]->IsNull());

  cl_int ret=::clEnqueueNDRangeKernel(
      cq->getCommandQueue(), kernel->getKernel(),
      num_globals, // work dimension
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
    REQ_ERROR_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_KERNEL);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_KERNEL_ARGS);
    REQ_ERROR_THROW(CL_INVALID_WORK_DIMENSION);
    REQ_ERROR_THROW(CL_INVALID_GLOBAL_OFFSET);
    REQ_ERROR_THROW(CL_INVALID_WORK_GROUP_SIZE);
    REQ_ERROR_THROW(CL_INVALID_WORK_ITEM_SIZE);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[5]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueTask)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

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
    REQ_ERROR_THROW(CL_INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_KERNEL);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_KERNEL_ARGS);
    REQ_ERROR_THROW(CL_INVALID_WORK_GROUP_SIZE);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueWriteBuffer)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  uint32_t offset = args[2]->Uint32Value();
  uint32_t size = args[3]->Uint32Value();

  void *ptr=NULL;
  if(!args[4]->IsUndefined()) {
    if(args[4]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[4]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[4]->IsObject())
      ptr = args[4]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueWriteBufferRect)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t buffer_origin[3] = {0,0,0};
  size_t host_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      buffer_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      host_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  uint32_t buffer_row_pitch=args[5]->Uint32Value();
  uint32_t buffer_slice_pitch=args[6]->Uint32Value();
  uint32_t host_row_pitch=args[7]->Uint32Value();
  uint32_t host_slice_pitch=args[8]->Uint32Value();

  void *ptr=NULL;
  if(!args[9]->IsUndefined()) {
    if(args[9]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[9]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[9]->IsObject())
      ptr = args[9]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[11]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueReadBuffer)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  uint32_t offset=args[2]->Uint32Value();
  uint32_t size=args[3]->Uint32Value();

  void *ptr=NULL;
  if(!args[4]->IsUndefined()) {
    if(args[4]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[4]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[4]->IsObject())
      ptr = args[4]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueReadBufferRect)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t buffer_origin[3] = {0,0,0};
  size_t host_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      buffer_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      host_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[4]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  uint32_t buffer_row_pitch=args[5]->Uint32Value();
  uint32_t buffer_slice_pitch=args[6]->Uint32Value();
  uint32_t host_row_pitch=args[7]->Uint32Value();
  uint32_t host_slice_pitch=args[8]->Uint32Value();

  void *ptr=NULL;
  if(!args[9]->IsUndefined()) {
    if(args[9]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[9]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[9]->IsObject())
      ptr = args[9]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[5]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueCopyBuffer)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueCopyBufferRect)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t dst_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[4]);
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

 if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[10]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueWriteImage)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t origin[3]={0,0,0};
  size_t region[3]={1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      origin[i]=arr->Get(i)->Uint32Value();

  arr= Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t row_pitch = args[4]->Uint32Value();
  size_t slice_pitch = args[5]->Uint32Value();

  void *ptr=NULL;
  if(!args[6]->IsUndefined()) {
    if(args[6]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[6]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[6]->IsObject())
      ptr = args[6]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[7]);

  cl_event event;
  bool no_event = (args[8]->IsUndefined() || args[8]->IsNull());

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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[8]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueReadImage)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());

  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  size_t origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      region[i]=arr->Get(i)->Uint32Value();

  size_t row_pitch = args[4]->Uint32Value();
  size_t slice_pitch = args[5]->Uint32Value();

  void *ptr=NULL;
  if(!args[6]->IsUndefined()) {
    if(args[6]->IsArray()) {
      Local<Array> arr=Array::Cast(*args[6]);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
    }
    else if(args[6]->IsObject())
      ptr = args[6]->ToObject()->GetIndexedPropertiesExternalArrayData();
    else
      ThrowError("Invalid memory object");
  }

  MakeEventWaitList(args[7]);

  cl_event event;
  bool no_event = (args[8]->IsUndefined() || args[8]->IsNull());

  cl_int ret=::clEnqueueReadImage(
      cq->getCommandQueue(), mo->getMemory(), blocking_read,
      origin,
      region,
      row_pitch, slice_pitch, ptr,
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[8]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueCopyImage)
{   
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t dst_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[4]);
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_IMAGE_FORMAT_MISMATCH);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_MEM_COPY_OVERLAP);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueCopyImageToBuffer)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_origin[3] = {0,0,0};
  size_t region[3] = {1,1,1};

  Local<Array> arr= Array::Cast(*args[2]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      src_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[3]);
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueCopyBufferToImage)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  MemoryObject *mo_src = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  MemoryObject *mo_dst = ObjectWrap::Unwrap<MemoryObject>(args[1]->ToObject());

  size_t src_offset = args[2]->Uint32Value();
  size_t dst_origin[3]={0,0,0};
  size_t region[3]={1,1,1};

  Local<Array> arr=Array::Cast(*args[3]);
  uint32_t i;
  for(i=0;i<arr->Length();i++)
      dst_origin[i]=arr->Get(i)->Uint32Value();

  arr=Array::Cast(*args[4]);
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

void free_callback(char *data, void *hint) {
  //cout<<"enqueueMapBuffer free_callback called"<<endl;
}

JS_METHOD(CommandQueue::enqueueMapBuffer)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_MISALIGNED_SUB_BUFFER_OFFSET);
    REQ_ERROR_THROW(CL_MAP_FAILURE);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  printf("\nmapbuffer %p = ",mo->getMemory());
  for (int i = 0; i < size/sizeof(int); ++i)
  {
    printf("%d ",((int*) result)[i]);
  }
  printf("\n");

  // wrap OpenCL result buffer into a node Buffer
  // WARNING: make sure result is shared not copied, otherwise unmapBuffer won't work
  node::Buffer *buf=node::Buffer::New((char*) result,size, free_callback,
  NULL);
  printf("result %p, wrapped data %p\n", result, node::Buffer::Data(buf));
  if(node::Buffer::Data(buf) != result) {
    printf("Error: data buffer has been copied\n");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }

  return scope.Close(buf->handle_);
}

JS_METHOD(CommandQueue::enqueueMapImage)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  // TODO: arg checking
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  cl_bool blocking = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_map_flags flags = args[2]->Uint32Value();

  size_t origin[3];
  size_t region[3];

  Local<Array> originArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = originArray->Get(i)->Uint32Value();
    origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
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
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_MAP_FAILURE);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  // TODO: return image_row_pitch, image_slice_pitch?

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[6]->ToObject());
    e->setEvent(event);
  }

  size_t nbytes = region[0] * region[1] * region[2];
  return scope.Close(node::Buffer::New((char*)result, nbytes,
  free_callback, NULL)->handle_);
}

JS_METHOD(CommandQueue::enqueueUnmapMemObject)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  // TODO: arg checking
  MemoryObject *mo = ObjectWrap::Unwrap<MemoryObject>(args[0]->ToObject());
  node::Buffer *buf = ObjectWrap::Unwrap<node::Buffer>(args[1]->ToObject());

  MakeEventWaitList(args[2]);

  cl_event event;
  bool no_event = (args[3]->IsUndefined() || args[3]->IsNull());

  printf("wrapped data %p, memobject %p\n", node::Buffer::Data(buf),mo->getMemory());
  printf("Before Unmap: ");
  for(int i=0;i<10;i++) {
    printf("%d ",((int*)node::Buffer::Data(buf))[i]);
  }
  printf("\n");

  cl_int ret=::clEnqueueUnmapMemObject(
      cq->getCommandQueue(), mo->getMemory(),
      node::Buffer::Data(buf),
      num_events_wait_list,
      events_wait_list,
      no_event ? NULL : &event);

  printf("After Unmap: ");
  for(int i=0;i<10;i++) {
    printf("%d ",((int*)node::Buffer::Data(buf))[i]);
  }
  printf("\n");

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[3]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueMarker)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  MakeEventWaitList(args[0]);

  cl_event event;
  bool no_event = (args[1]->IsUndefined() || args[1]->IsNull());

  cl_int ret = ::clEnqueueMarker(cq->getCommandQueue(),
  no_event ? NULL : &event);

  if(events_wait_list && ret==CL_SUCCESS) {
    cl_int ret2 = ::clEnqueueWaitForEvents(
        cq->getCommandQueue(),
        num_events_wait_list,
        events_wait_list);
    if (ret2 != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_CONTEXT);
      REQ_ERROR_THROW(CL_INVALID_EVENT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    delete[] events_wait_list;
  }

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[1]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

/*JS_METHOD(CommandQueue::enqueueWaitForEvents)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  MakeEventWaitList(args[0]);

  cl_int ret = ::clEnqueueWaitForEvents(
      cq->getCommandQueue(),
      num_events_wait_list,
      events_wait_list);

  if(events_wait_list) delete[] events_wait_list;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}*/

JS_METHOD(CommandQueue::enqueueBarrier)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

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
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_CONTEXT);
      REQ_ERROR_THROW(CL_INVALID_EVENT);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }

    delete[] events_wait_list;
  }

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  if(!no_event && event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[1]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::finish)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  cl_int ret = ::clFinish(cq->getCommandQueue());

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

JS_METHOD(CommandQueue::flush)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);
  cl_int ret = ::clFlush(cq->getCommandQueue());

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

JS_METHOD(CommandQueue::enqueueAcquireGLObjects)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[0]->IsArray()) {
    Local<Array> mem_objects_arr= Array::Cast(*args[0]);
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

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::enqueueReleaseGLObjects)
{
  HandleScope scope;
  CommandQueue *cq = UnwrapThis<CommandQueue>(args);

  cl_mem *mem_objects=NULL;
  int num_objects=0;
  if(args[0]->IsArray()) {
    Local<Array> mem_objects_arr= Array::Cast(*args[0]);
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

  if(!no_event) {
    Event *e=ObjectWrap::Unwrap<Event>(args[2]->ToObject());
    e->setEvent(event);
  }
  return Undefined();
}

JS_METHOD(CommandQueue::New)
{
  HandleScope scope;
  CommandQueue *cq = new CommandQueue(args.This());
  cq->Wrap(args.This());
  registerCLObj(cq);
  return scope.Close(args.This());
}

CommandQueue *CommandQueue::New(cl_command_queue cw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  CommandQueue *commandqueue = ObjectWrap::Unwrap<CommandQueue>(obj);
  commandqueue->command_queue = cw;

  return commandqueue;
}

}
