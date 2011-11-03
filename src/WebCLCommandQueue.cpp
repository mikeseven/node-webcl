/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLCommandQueue.h"
#include "WebCLPlatform.h"
#include "WebCLContext.h"
#include "WebCLDevice.h"
#include "WebCLMemory.h"
#include "WebCLEvent.h"
#include "WebCLKernel.h"
#include "WebCLMappedRegion.h"

#include "node_buffer.h"

#include <iostream>
using namespace std;

using namespace v8;
using namespace webcl;
using namespace node;

#define MakeEventWaitList(arg) \
  VECTOR_CLASS<cl::Event> *pevents=NULL;\
  VECTOR_CLASS<cl::Event> events;\
  if(!(arg)->IsUndefined()) {\
    Local<Array> eventWaitArray = Array::Cast(*(arg));\
    if(eventWaitArray->Length()>0) {\
      pevents=&events;\
      for (int i=0; i<eventWaitArray->Length(); i++) {\
        Local<Object> obj = eventWaitArray->Get(i)->ToObject();\
        WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(obj);\
        events.push_back( *e->getEvent() );\
      }\
    }\
  }

Persistent<FunctionTemplate> WebCLCommandQueue::constructor_template;

// e.g. IsUint32Array(v)
#define IS_BUFFER_FUNC(name, type)\
    bool Is##name(v8::Handle<v8::Value> val) {\
  if (!val->IsObject()) return false;\
  v8::Local<v8::Object> obj = val->ToObject();\
  if (obj->GetIndexedPropertiesExternalArrayDataType() == type)\
  return true;\
  return false;\
}

IS_BUFFER_FUNC(Int8Array, kExternalByteArray);
IS_BUFFER_FUNC(Uint8Array, kExternalUnsignedByteArray);
IS_BUFFER_FUNC(Int16Array, kExternalShortArray);
IS_BUFFER_FUNC(Uint16Array, kExternalUnsignedShortArray);
IS_BUFFER_FUNC(Int32Array, kExternalIntArray);
IS_BUFFER_FUNC(Uint32Array, kExternalUnsignedIntArray);
IS_BUFFER_FUNC(Float32Array, kExternalFloatArray);

/* static  */
void WebCLCommandQueue::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLCommandQueue::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLCommandQueue"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueNDRangeKernel", enqueueNDRangeKernel);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueTask", enqueueTask);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueWriteBuffer", enqueueWriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueReadBuffer", enqueueReadBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueCopyBuffer", enqueueCopyImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueWriteBufferRect", enqueueWriteBufferRect);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueReadBufferRect", enqueueReadBufferRect);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueCopyBufferRect", enqueueCopyImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueWriteImage", enqueueWriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueReadImage", enqueueReadBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueCopyImage", enqueueCopyImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueCopyImageToBuffer", enqueueCopyImageToBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueCopyBufferToImage", enqueueCopyBufferToImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueMapBuffer", enqueueMapBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueMapImage", enqueueMapImage);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueUnmapMemObject", enqueueUnmapMemObject);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueMarker", enqueueMarker);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueWaitForEvents", enqueueWaitForEvents);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "enqueueBarrier", enqueueBarrier);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "flush", flush);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "finish", finish);

  target->Set(String::NewSymbol("WebCLCommandQueue"), constructor_template->GetFunction());
}

WebCLCommandQueue::WebCLCommandQueue(Handle<Object> wrapper) : command_queue(0)
{
}

WebCLCommandQueue::~WebCLCommandQueue()
{
  if (command_queue) delete command_queue;
}

/* static */
JS_METHOD(WebCLCommandQueue::getInfo)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  cl_command_queue_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_QUEUE_CONTEXT: {
    cl_context ctx=0;
    cl_int ret=cq->getCommandQueue()->getInfo(param_name,&ctx);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    cl::Context *cw = new cl::Context(ctx);
    return scope.Close(WebCLContext::New(cw)->handle_);
  }
  case CL_QUEUE_DEVICE: {
    cl_device_id dev=0;
    cl_int ret=cq->getCommandQueue()->getInfo(param_name,&dev);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    cl::Device *cw = new cl::Device(dev);
    return scope.Close(WebCLDevice::New(cw)->handle_);
  }
  case CL_QUEUE_REFERENCE_COUNT:
  case CL_QUEUE_PROPERTIES: {
    cl_uint param_value;
    cl_int ret=cq->getCommandQueue()->getInfo(param_name,&param_value);
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

/* static */
JS_METHOD(WebCLCommandQueue::enqueueNDRangeKernel)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  REQ_ARGS(4);

  WebCLKernel *k = ObjectWrap::Unwrap<WebCLKernel>(args[0]->ToObject());

  Local<Array> arr = Array::Cast(*args[1]);
  int num=arr->Length();
  cl::NDRange *offset;
  if(num==0)
    offset=(cl::NDRange*) &cl::NullRange;
  else if(num==1)
    offset=new cl::NDRange(arr->Get(0)->Uint32Value());
  else if(num==2)
    offset=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value());
  else if(num==3)
    offset=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value(),arr->Get(2)->Uint32Value());

  arr = Array::Cast(*args[2]);
  num=arr->Length();
  cl::NDRange *global;
  if(num==0)
    global=(cl::NDRange*) &cl::NullRange;
  else if(num==1)
    global=new cl::NDRange(arr->Get(0)->Uint32Value());
  else if(num==2)
    global=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value());
  else if(num==3)
    global=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value(),arr->Get(2)->Uint32Value());

  arr = Array::Cast(*args[3]);
  num=arr->Length();
  cl::NDRange *local;
  if(num==0)
    local=(cl::NDRange*) &cl::NullRange;
  else if(num==1)
    local=new cl::NDRange(arr->Get(0)->Uint32Value());
  else if(num==2)
    local=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value());
  else if(num==3)
    local=new cl::NDRange(arr->Get(0)->Uint32Value(),arr->Get(1)->Uint32Value(),arr->Get(2)->Uint32Value());

  MakeEventWaitList(args[4]);
  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueNDRangeKernel(*k->getKernel(),
      *offset,*global,*local,
      pevents,event);
  if(*offset!=cl::NullRange) delete offset;
  if(*global!=cl::NullRange) delete global;
  if(*local!=cl::NullRange) delete local;

  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueTask)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  REQ_ARGS(1);

  WebCLKernel *k = ObjectWrap::Unwrap<WebCLKernel>(args[0]->ToObject());

  MakeEventWaitList(args[1]);
  cl::Event *event=new cl::Event();

  cl_int ret=cq->getCommandQueue()->enqueueTask(*k->getKernel(),
      pevents,event);

  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueWriteBuffer)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;


  Local<Array> region=Array::Cast(*args[2]);
  Local<Value> vbuffer=region->Get(JS_STR("buffer"));
  void *ptr=NULL;
  if(!vbuffer->IsUndefined())
    ptr = vbuffer->ToObject()->GetIndexedPropertiesExternalArrayData();

  size_t offset=0;
  Local<Value> voffset=region->Get(JS_STR("offset"));
  if(!voffset->IsUndefined())
    offset=voffset->Uint32Value();

  size_t size=0;
  Local<Value> vsize=region->Get(JS_STR("size"));
  if(!vsize->IsUndefined())
    size=vsize->Uint32Value();

  MakeEventWaitList(args[3]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueWriteBuffer(
      *(cl::Buffer*)mo->getMemory(),
      blocking_write,offset,size,ptr,pevents,event);

  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueReadBuffer)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;

  Local<Array> region=Array::Cast(*args[2]);
  Local<Value> vbuffer=region->Get(JS_STR("buffer"));
  void *ptr=NULL;
  if(!vbuffer->IsUndefined())
    ptr = vbuffer->ToObject()->GetIndexedPropertiesExternalArrayData();

  size_t offset=0;
  Local<Value> voffset=region->Get(JS_STR("offset"));
  if(!voffset->IsUndefined())
    offset=voffset->Uint32Value();

  size_t size=0;
  Local<Value> vsize=region->Get(JS_STR("size"));
  if(!vsize->IsUndefined())
    size=vsize->Uint32Value();

  MakeEventWaitList(args[3]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueReadBuffer(
      *(cl::Buffer*)mo->getMemory(),
      blocking_read, offset, size, ptr, pevents, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
// TODO update with regions
JS_METHOD(WebCLCommandQueue::enqueueCopyBuffer)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  WebCLMemory *mo_src = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMemory *mo_dst = ObjectWrap::Unwrap<WebCLMemory>(args[1]->ToObject());

  // TODO: arg checking
  ::size_t src_offset = args[2]->NumberValue();
  ::size_t dst_offset = args[3]->NumberValue();
  ::size_t size = args[4]->NumberValue();

  MakeEventWaitList(args[5]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueCopyBuffer(
      *(cl::Buffer*)mo_src->getMemory(), *(cl::Buffer*)mo_dst->getMemory(),
      src_offset,dst_offset,size,&events,event);

  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueWriteBufferRect)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl::size_t<3> buffer_offset;
  cl::size_t<3> host_offset;
  cl::size_t<3> region;

  Local<Array> bufferOrigin = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = bufferOrigin->Get(i)->NumberValue();
    buffer_offset[i] = s;
  }

  Local<Array> hostOrigin = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = hostOrigin->Get(i)->NumberValue();
    host_offset[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->NumberValue();
    region[i] = s;
  }

  size_t buffer_row_pitch = args[5]->NumberValue();
  size_t buffer_slice_pitch = args[6]->NumberValue();
  size_t host_row_pitch = args[7]->NumberValue();
  size_t host_slice_pitch = args[8]->NumberValue();

  void *ptr = args[9]->ToObject()->GetIndexedPropertiesExternalArrayData();

  MakeEventWaitList(args[10]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueWriteBufferRect(
      *(cl::Buffer*)mo->getMemory(),
      blocking_write,buffer_offset,host_offset,region,
      buffer_row_pitch,buffer_slice_pitch,host_row_pitch,host_slice_pitch,
      ptr,pevents,event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueReadBufferRect)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl::size_t<3> buffer_offset;
  cl::size_t<3> host_offset;
  cl::size_t<3> region;

  Local<Array> bufferOrigin = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = bufferOrigin->Get(i)->Uint32Value();
    buffer_offset[i] = s;
  }

  Local<Array> hostOrigin = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = hostOrigin->Get(i)->Uint32Value();
    host_offset[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->Uint32Value();
    region[i] = s;
  }

  ::size_t buffer_row_pitch = args[5]->Uint32Value();
  ::size_t buffer_slice_pitch = args[6]->Uint32Value();
  ::size_t host_row_pitch = args[7]->Uint32Value();
  ::size_t host_slice_pitch = args[8]->Uint32Value();

  // TODO use WebCLMappedRegion instead of ptr
  void *ptr = args[9]->ToObject()->GetIndexedPropertiesExternalArrayData();

  MakeEventWaitList(args[10]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueReadBufferRect(
      *(cl::Buffer*) mo->getMemory(),
      blocking_read, buffer_offset, host_offset, region,
      buffer_row_pitch, buffer_slice_pitch, host_row_pitch, host_slice_pitch,
      ptr, pevents, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueCopyBufferRect)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo_src = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMemory *mo_dst = ObjectWrap::Unwrap<WebCLMemory>(args[1]->ToObject());

  cl::size_t<3> src_origin;
  cl::size_t<3> dst_origin;
  cl::size_t<3> region;

  Local<Array> srcOrigin = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = srcOrigin->Get(i)->Uint32Value();
    src_origin[i] = s;
  }

  Local<Array> dstOrigin = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = dstOrigin->Get(i)->Uint32Value();
    dst_origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->Uint32Value();
    region[i] = s;
  }

  ::size_t src_row_pitch = args[5]->Uint32Value();
  ::size_t src_slice_pitch = args[6]->Uint32Value();
  ::size_t dst_row_pitch = args[7]->Uint32Value();
  ::size_t dst_slice_pitch = args[8]->Uint32Value();

  MakeEventWaitList(args[10]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueCopyBufferRect(
      *(cl::Buffer*)mo_src->getMemory(), *(cl::Buffer*)mo_dst->getMemory(),
      src_origin, dst_origin, region,
      src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
      &events, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueWriteImage)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_write = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl::size_t<3> origin;
  cl::size_t<3> region;

  Local<Array> originArray = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = originArray->Get(i)->NumberValue();
    origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->NumberValue();
    region[i] = s;
  }

  ::size_t row_pitch = args[4]->NumberValue();
  ::size_t slice_pitch = args[5]->NumberValue();

  void *ptr = args[6]->ToObject()->GetIndexedPropertiesExternalArrayData();

  MakeEventWaitList(args[7]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueWriteImage(
      *(cl::Image*)mo->getMemory(),
      blocking_write, origin, region, row_pitch, slice_pitch, ptr,
      pevents, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueReadImage)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());

  // TODO: arg checking
  cl_bool blocking_read = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl::size_t<3> origin;
  cl::size_t<3> region;

  Local<Array> originArray = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = originArray->Get(i)->Uint32Value();
    origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->Uint32Value();
    region[i] = s;
  }

  size_t row_pitch = args[4]->Uint32Value();
  size_t slice_pitch = args[5]->Uint32Value();

  void *ptr = args[6]->ToObject()->GetIndexedPropertiesExternalArrayData();

  MakeEventWaitList(args[7]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueReadImage(
      *(cl::Image*)mo->getMemory(),
      blocking_read,origin,region,row_pitch,slice_pitch,
      ptr, pevents,
      event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueCopyImage)
{   
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo_src = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMemory *mo_dst = ObjectWrap::Unwrap<WebCLMemory>(args[1]->ToObject());

  // TODO: arg checking
  cl::size_t<3> src_origin;
  cl::size_t<3> dst_origin;
  cl::size_t<3> region;

  Local<Array> srcOriginArray = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = srcOriginArray->Get(i)->NumberValue();
    src_origin[i] = s;
  }

  Local<Array> dstOriginArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = dstOriginArray->Get(i)->NumberValue();
    dst_origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->NumberValue();
    region[i] = s;
  }

  MakeEventWaitList(args[5]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueCopyImage(
      *(cl::Image*)mo_src->getMemory(),
      *(cl::Image*)mo_dst->getMemory(),
      src_origin, dst_origin, region,
      pevents,
      event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueCopyImageToBuffer)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo_src = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMemory *mo_dst = ObjectWrap::Unwrap<WebCLMemory>(args[1]->ToObject());

  // TODO: arg checking
  cl::size_t<3> src_origin;
  cl::size_t<3> region;

  Local<Array> srcOriginArray = Array::Cast(*args[2]);
  for (int i=0; i<3; i++) {
    size_t s = srcOriginArray->Get(i)->NumberValue();
    src_origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->NumberValue();
    region[i] = s;
  }

  ::size_t dst_offset = args[4]->NumberValue();

  MakeEventWaitList(args[5]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueCopyImageToBuffer(
      *(cl::Image*) mo_src->getMemory(),
      *(cl::Buffer*) mo_dst->getMemory(),
      src_origin, region, dst_offset,
      pevents, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueCopyBufferToImage)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  WebCLMemory *mo_src = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMemory *mo_dst = ObjectWrap::Unwrap<WebCLMemory>(args[1]->ToObject());

  // TODO: arg checking
  cl::size_t<3> dst_origin;
  cl::size_t<3> region;

  ::size_t src_offset = args[2]->NumberValue();

  Local<Array> dstOriginArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    size_t s = dstOriginArray->Get(i)->NumberValue();
    dst_origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    size_t s = regionArray->Get(i)->NumberValue();
    region[i] = s;
  }

  MakeEventWaitList(args[5]);

  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueCopyBufferToImage(
      *(cl::Buffer*)mo_src->getMemory(),
      *(cl::Image*)mo_dst->getMemory(),
      src_offset, dst_origin, region,
      pevents, event);
  if (ret != CL_SUCCESS) {
    delete event;
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

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueMapBuffer)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  // TODO: arg checking
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  cl_bool blocking = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_map_flags flags = args[2]->Uint32Value();
  size_t offset = args[3]->Uint32Value();
  size_t size = args[4]->Uint32Value();

  MakeEventWaitList(args[5]);

  /*Local<Array> region=Array::Cast(*args[0]);
  Local<Value> vbuffer=region->Get(JS_STR("buffer"));
  WebCLMemory *mo=NULL;
  if(!vbuffer->IsUndefined())
    mo = ObjectWrap::Unwrap<WebCLMemory>(vbuffer->ToObject());

  size_t offset=0;
  Local<Value> voffset=region->Get(JS_STR("offset"));
  if(!voffset->IsUndefined())
    offset=voffset->Uint32Value();

  size_t size=0;
  Local<Value> vsize=region->Get(JS_STR("size"));
  if(!vsize->IsUndefined())
    size=vsize->Uint32Value();

  MakeEventWaitList(args[3]);*/

  cl_int ret=CL_SUCCESS;
  cl::Event *event=new cl::Event(); // TODO should we create an Event
  void *result=cq->getCommandQueue()->enqueueMapBuffer(*(cl::Buffer*)mo->getMemory(),
      blocking, flags,
      offset, size,
      pevents, event,&ret);

  if (ret != CL_SUCCESS) {
    delete event;
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

  cout<<"enqueueMapBuffer"<<endl;
  for(int i=0;i<size;i++) {
    cout<<(int)((char*)result)[i]<<" ";
  }
  cout<<endl;

  _MappedRegion *mapped_region=new _MappedRegion();
  mapped_region->buffer=Buffer::New((char*) result,size);
  mapped_region->mapped_ptr=(ulong) result; // TODO use SetHiddenValue on buffer
  mapped_region->event=WebCLEvent::New(event); // TODO check if this event can be correctly used? and deleted?

  return scope.Close(WebCLMappedRegion::New(mapped_region)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueMapImage)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  // TODO: arg checking
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  cl_bool blocking = args[1]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_map_flags flags = args[2]->Uint32Value();

  cl::size_t<3> origin;
  cl::size_t<3> region;

  Local<Array> originArray = Array::Cast(*args[3]);
  for (int i=0; i<3; i++) {
    ::size_t s = originArray->Get(i)->Uint32Value();
    origin[i] = s;
  }

  Local<Array> regionArray = Array::Cast(*args[4]);
  for (int i=0; i<3; i++) {
    ::size_t s = regionArray->Get(i)->Uint32Value();
    region[i] = s;
  }

  MakeEventWaitList(args[5]);

  ::size_t row_pitch;
  ::size_t slice_pitch;

  cl::Event *event=new cl::Event(); // TODO should we create an Event
  cl_int ret=CL_SUCCESS;
  void *result=cq->getCommandQueue()->enqueueMapImage(*(cl::Image*)mo->getMemory(),
      blocking,flags,
      origin,region,
      &row_pitch,&slice_pitch,pevents,event,&ret);

  // TODO check errors, delete event

  // TODO: return image_row_pitch, image_slice_pitch?

  size_t nbytes = region[0] * region[1] * region[2];
  return scope.Close(node::Buffer::New((char*)result, nbytes)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueUnmapMemObject)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  // TODO: arg checking
  WebCLMemory *mo = ObjectWrap::Unwrap<WebCLMemory>(args[0]->ToObject());
  WebCLMappedRegion *region = ObjectWrap::Unwrap<WebCLMappedRegion>(args[1]->ToObject());
  char *mapped_ptr=(char*) region->getMappedRegion()->mapped_ptr;

  cout<<"enqueueUnmapMemObject"<<endl;
  for(int i=0;i<Buffer::Length(region->getMappedRegion()->buffer);i++) {
    mapped_ptr[i]=Buffer::Data(region->getMappedRegion()->buffer)[i];
    cout<<(int) mapped_ptr[i]<<" ";
  }
  cout<<endl;

  MakeEventWaitList(args[2]);

  // TODO should we create the Event or allow user to specify it?
  cl::Event *event=new cl::Event();
  cl_int ret=cq->getCommandQueue()->enqueueUnmapMemObject(*mo->getMemory(),mapped_ptr,pevents,event);

  if (ret != CL_SUCCESS) {
    delete event;
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_MEM_OBJECT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_EVENT_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueMarker)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  cl::Event *event=new cl::Event();
  cl_int ret = cq->getCommandQueue()->enqueueMarker(event);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(WebCLEvent::New(event)->handle_);
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueWaitForEvents)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);

  VECTOR_CLASS<cl::Event> event_wait_list;
  Local<Array> eventWaitArray = Array::Cast(*args[0]);
  for (int i=0; i<eventWaitArray->Length(); i++) {
    Local<Object> obj = eventWaitArray->Get(i)->ToObject();
    WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(obj);
    event_wait_list.push_back( *e->getEvent() );
  }

  cl_int ret = cq->getCommandQueue()->enqueueWaitForEvents(event_wait_list);

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
}

/* static */
JS_METHOD(WebCLCommandQueue::enqueueBarrier)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  cl_int ret = cq->getCommandQueue()->enqueueBarrier();

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

/* static */
JS_METHOD(WebCLCommandQueue::finish)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  cl_int ret = cq->getCommandQueue()->finish();

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

/* static */
JS_METHOD(WebCLCommandQueue::flush)
{
  HandleScope scope;
  WebCLCommandQueue *cq = UnwrapThis<WebCLCommandQueue>(args);
  cl_int ret = cq->getCommandQueue()->flush();

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_COMMAND_QUEUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

/* static  */
JS_METHOD(WebCLCommandQueue::New)
{
  HandleScope scope;
  WebCLCommandQueue *cl = new WebCLCommandQueue(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLCommandQueue *WebCLCommandQueue::New(cl::CommandQueue* cw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLCommandQueue *commandqueue = ObjectWrap::Unwrap<WebCLCommandQueue>(obj);
  commandqueue->command_queue = cw;

  return commandqueue;
}
