/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLContext.h"
#include "WebCLMemory.h"
#include "WebCLProgram.h"
#include "WebCLDevice.h"
#include "WebCLCommandQueue.h"
#include "WebCLEvent.h"
#include "WebCLSampler.h"
#include "WebCLPlatform.h"
#include <node_buffer.h>

#include <iostream>
using namespace std;

using namespace std;
using namespace v8;
using namespace webcl;
using namespace cl;

Persistent<FunctionTemplate> WebCLContext::constructor_template;

/* static  */
void WebCLContext::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLContext::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLContext"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createProgram", createProgram);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createCommandQueue", createCommandQueue);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createBuffer", createBuffer);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createBufferGL", createBufferGL);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createImage2D", createImage2D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createImage3D", createImage3D);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createSampler", createSampler);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createUserEvent", createUserEvent);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getSupportedImageFormats", getSupportedImageFormats);

  target->Set(String::NewSymbol("WebCLContext"), constructor_template->GetFunction());
}

WebCLContext::WebCLContext(Handle<Object> wrapper) : context(0)
{
}

WebCLContext::~WebCLContext()
{
  if (context) delete context;
}

/* static */
JS_METHOD(WebCLContext::getInfo)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_context_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_CONTEXT_REFERENCE_COUNT:
  case CL_CONTEXT_NUM_DEVICES: {
    cl_uint param_value=0;
    cl_int ret=context->getContext()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_CONTEXT);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(JS_INT(param_value));
  }
  case CL_CONTEXT_DEVICES: {
    VECTOR_CLASS<cl_device_id>  ctx;
    context->getContext()->getInfo(param_name,&ctx);

    int n=ctx.size();
    Local<Array> arr = Array::New(n);
    for(int i=0;i<n;i++) {
      cl::Device *device=new cl::Device(ctx[i]);
      arr->Set(i,WebCLDevice::New(device)->handle_);
    }
    return scope.Close(arr);
  }
  case CL_CONTEXT_PROPERTIES: {
    VECTOR_CLASS<cl_context_properties>  ctx;
    context->getContext()->getInfo(param_name,&ctx);

    int n=ctx.size();
    Local<Array> arr = Array::New(n);
    for(int i=0;i<n;i++) {
      arr->Set(i,JS_INT(ctx[i]));
    }
    return scope.Close(arr);
  }
  default:
    return JS_EXCEPTION("UNKNOWN param_name");
  }
}

JS_METHOD(WebCLContext::createProgram)
{
  HandleScope scope;
  WebCLContext *context = UnwrapThis<WebCLContext>(args);
  cl::Program *pw=NULL;
  cl_int ret = CL_SUCCESS;

  // either we pass a code (string) or binary buffers
  if(args[0]->IsString()) {
    Local<String> str = args[0]->ToString();
    String::AsciiValue astr(str);
    cl::Program::Sources sources;
    std::pair<const char*, ::size_t> source((*astr),astr.length());
    sources.push_back(source);

    pw=new cl::Program(*context->getContext(),sources,&ret);

    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_CONTEXT);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }
    return scope.Close(WebCLProgram::New(pw)->handle_);
  }
  else if(args[0]->IsArray()){
    Local<Array> devArray = Array::Cast(*args[0]);
    uint32_t num=devArray->Length();
    VECTOR_CLASS<Device> devices;

    for (int i=0; i<num; i++) {
      WebCLDevice *device = ObjectWrap::Unwrap<WebCLDevice>(devArray->Get(i)->ToObject());
      devices.push_back(*device->getDevice());
    }

    Local<Array> binArray = Array::Cast(*args[1]);
    num=binArray->Length();
    cl::Program::Binaries binaries;

    for(int i=0;i<num;i++) {
      void *ptr = binArray->Get(i)->ToObject()->GetIndexedPropertiesExternalArrayData();
      std::pair<void*, ::size_t> binary(ptr,binArray->Get(i)->ToObject()->GetIndexedPropertiesExternalArrayDataLength());
      binaries.push_back(binary);
    }

    VECTOR_CLASS<cl_int> binaryStatus(devArray->Length());
    pw=new cl::Program(*context->getContext(),devices,binaries,&binaryStatus,&ret);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_CONTEXT);
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_DEVICE);
      REQ_ERROR_THROW(CL_INVALID_BINARY);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return JS_EXCEPTION("UNKNOWN ERROR");
    }

    // TODO should we return binaryStatus?
    return scope.Close(WebCLProgram::New(pw)->handle_);
  }

  return Undefined();
}

/* static */
JS_METHOD(WebCLContext::createCommandQueue)
{
  HandleScope scope;
  WebCLContext *context = ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl::Device *device = ObjectWrap::Unwrap<WebCLDevice>(args[0]->ToObject())->getDevice();
  cl_command_queue_properties properties = args[1]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::CommandQueue *cw = new cl::CommandQueue(*context->getContext(), *device, properties,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_DEVICE);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_QUEUE_PROPERTIES);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLCommandQueue::New(cw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createBuffer)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->Uint32Value();
  ::size_t size = args[1]->Uint32Value();
  void *host_ptr = NULL;
  if(!args[2]->IsUndefined())
    host_ptr=args[2]->ToObject()->GetIndexedPropertiesExternalArrayData();

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Buffer(*context->getContext(),flags,size,host_ptr,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_BUFFER_SIZE);
    REQ_ERROR_THROW(CL_INVALID_HOST_PTR);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

JS_METHOD(WebCLContext::createBufferGL)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->Uint32Value();
  GLuint bufobj=args[1]->Uint32Value(); // TODO use WebGLBuffer instead

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::BufferGL(*context->getContext(),flags,bufobj,&ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_BUFFER_SIZE);
    REQ_ERROR_THROW(CL_INVALID_HOST_PTR);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createImage2D)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();

  cl::ImageFormat image_format;
  Local<Object> obj = args[1]->ToObject();
  image_format.image_channel_order = obj->Get(JS_STR("order"))->Uint32Value();
  image_format.image_channel_data_type = obj->Get(String::New("data_type"))->Uint32Value();

  std::size_t width = args[2]->NumberValue();
  std::size_t height = args[3]->NumberValue();
  std::size_t row_pitch = args[4]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Image2D(*context->getContext(),flags,image_format,width,height,row_pitch,&ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_INVALID_HOST_PTR);
    REQ_ERROR_THROW(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createImage3D)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();

  cl::ImageFormat image_format;
  Local<Object> obj = args[1]->ToObject();
  image_format.image_channel_order = obj->Get(JS_STR("order"))->Uint32Value();
  image_format.image_channel_data_type = obj->Get(JS_STR("data_type"))->Uint32Value();

  std::size_t width = args[2]->NumberValue();
  std::size_t height = args[3]->NumberValue();
  std::size_t depth = args[4]->NumberValue();
  std::size_t row_pitch = args[5]->NumberValue();
  std::size_t slice_pitch = args[6]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Memory *mw = new cl::Image3D(*context->getContext(),flags,image_format,width,height,depth,row_pitch,slice_pitch,&ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
    REQ_ERROR_THROW(CL_INVALID_IMAGE_SIZE);
    REQ_ERROR_THROW(CL_INVALID_HOST_PTR);
    REQ_ERROR_THROW(CL_IMAGE_FORMAT_NOT_SUPPORTED);
    REQ_ERROR_THROW(CL_MEM_OBJECT_ALLOCATION_FAILURE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLMemory::New(mw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::createSampler)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_bool norm_coords = args[0]->BooleanValue() ? CL_TRUE : CL_FALSE;
  cl_addressing_mode addr_mode = args[1]->NumberValue();
  cl_filter_mode filter_mode = args[2]->NumberValue();

  cl_int ret=CL_SUCCESS;
  cl::Sampler *sw=new cl::Sampler(*context->getContext(),norm_coords,addr_mode,filter_mode,&ret);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLSampler::New(sw)->handle_);
}

/* static */
JS_METHOD(WebCLContext::getSupportedImageFormats)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());
  cl_mem_flags flags = args[0]->NumberValue();
  cl_mem_object_type image_type = args[1]->NumberValue();
  VECTOR_CLASS<cl::ImageFormat> image_formats;

  cl_int ret = context->getContext()->getSupportedImageFormats(flags,image_type,&image_formats);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  Local<Array> imageFormats = Array::New();
  for (int i=0; i<image_formats.size(); i++) {
    Local<Object> format = Object::New();
    format->Set(JS_STR("order"), JS_INT(image_formats[i].image_channel_order));
    format->Set(JS_STR("data_type"), JS_INT(image_formats[i].image_channel_data_type));
    imageFormats->Set(i, format);
  }

  return scope.Close(imageFormats);
}

/* static */
JS_METHOD(WebCLContext::createUserEvent)
{
  HandleScope scope;
  WebCLContext *context = node::ObjectWrap::Unwrap<WebCLContext>(args.This());

  cl::UserEvent *ew=new cl::UserEvent();
  return scope.Close(WebCLEvent::New(ew)->handle_);
}

/* static  */
JS_METHOD(WebCLContext::New)
{
  HandleScope scope;
  WebCLContext *cl = new WebCLContext(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLContext *WebCLContext::New(cl::Context* cw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLContext *context = ObjectWrap::Unwrap<WebCLContext>(obj);
  context->context = cw;

  return context;
}
