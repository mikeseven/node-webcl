/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCL.h"
#include "WebCLPlatform.h"
#include "WebCLDevice.h"
#include "WebCLContext.h"
#include "WebCLMemory.h"
#include "WebCLProgram.h"
#include "WebCLKernel.h"
#include "WebCLCommandQueue.h"
#include "WebCLEvent.h"
#include "WebCLSampler.h"

using namespace v8;
using namespace node;
using namespace webcl;

Persistent<FunctionTemplate> WebCL::constructor_template;

/*static*/
void WebCL::Init(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCL::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCL"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getPlatformIDs", getPlatformIDs);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createContext", createContext);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "createContextFromType", createContextFromType);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "waitForEvents", waitForEvents);

  target->Set(String::NewSymbol("WebCL"), constructor_template->GetFunction());
}

/*static*/
JS_METHOD(WebCL::New) {
  HandleScope scope;
  WebCL *cl = new WebCL();
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/*static*/
JS_METHOD(WebCL::getPlatformIDs) {
  HandleScope scope;
  VECTOR_CLASS<cl::Platform> platforms;
  cl_int ret=cl::Platform::get(&platforms);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  Local<Array> platformArray = Array::New(platforms.size());
  for (int i=0; i<platforms.size(); i++) {
    cl::Platform *platform=new cl::Platform(platforms[i]);
    platformArray->Set(i, WebCLPlatform::New(platform)->handle_);
  }

  return scope.Close(platformArray);
}

/*static*/
JS_METHOD(WebCL::createContext) {
  HandleScope scope;
  if (!args[0]->IsArray())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));
  if (!args[1]->IsArray())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));

  Local<Array> propertiesArray = Array::Cast(*args[0]);
  cl_context_properties *properties = new cl_context_properties[propertiesArray->Length()+1];

  for (int i=0; i<propertiesArray->Length(); i+=2) {
    properties[i] = (cl_context_properties)propertiesArray->Get(i)->NumberValue();
  }
  properties[propertiesArray->Length()] = 0;

  Local<Array> deviceArray = Array::Cast(*args[1]);
  VECTOR_CLASS<cl::Device> devices;
  for (int i=0; i<deviceArray->Length(); i++) {
    Local<Object> obj = deviceArray->Get(i)->ToObject();
    WebCLDevice *d = ObjectWrap::Unwrap<WebCLDevice>(obj);
    devices.push_back(*d->getDevice());
  }

  cl_int ret=CL_SUCCESS;
  cl::Context *cw=new cl::Context(devices, properties,NULL,NULL, &ret);
  delete[] properties;

  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROPERTY);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE);
    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_AVAILABLE);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLContext::New(cw)->handle_);
}

/*static*/
JS_METHOD(WebCL::createContextFromType) {
  HandleScope scope;
  if (!args[0]->IsArray())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));

  Local<Array> propertiesArray = Array::Cast(*args[0]);
  cl_context_properties *properties = new cl_context_properties[propertiesArray->Length()+1];
  for (int i=0; i<propertiesArray->Length(); i+=2) {
    Local<Object> obj = propertiesArray->Get(i+1)->ToObject();
    WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(obj);

    properties[i]=propertiesArray->Get(i)->NumberValue();
  }

  cl_device_type device_type = args[1]->Uint32Value();
  cl_int ret=CL_SUCCESS;
  cl::Context *cw=new cl::Context(device_type,properties,NULL,NULL,&ret);
  delete[] properties;
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_PLATFORM);
    WEBCL_COND_RETURN_THROW(CL_INVALID_PROPERTY);
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_DEVICE_TYPE);
    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_AVAILABLE);
    WEBCL_COND_RETURN_THROW(CL_DEVICE_NOT_FOUND);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return scope.Close(WebCLContext::New(cw)->handle_);
}

/*static*/
JS_METHOD(WebCL::waitForEvents) {
  if (!args[0]->IsArray())
    ThrowException(Exception::Error(String::New("CL_INVALID_VALUE")));

  Local<Array> eventsArray = Array::Cast(*args[0]);
  VECTOR_CLASS<cl::Event> events;
  for (int i=0; i<eventsArray->Length(); i++) {
    Local<Object> obj = eventsArray->Get(i)->ToObject();
    cl::Event *e = ObjectWrap::Unwrap<WebCLEvent>(obj)->getEvent();
    events.push_back(*e);
  }
  cl_int ret=cl::Event::waitForEvents(events);
  if (ret != CL_SUCCESS) {
    WEBCL_COND_RETURN_THROW(CL_INVALID_VALUE);
    WEBCL_COND_RETURN_THROW(CL_INVALID_CONTEXT);
    WEBCL_COND_RETURN_THROW(CL_INVALID_EVENT);
    WEBCL_COND_RETURN_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_RESOURCES);
    WEBCL_COND_RETURN_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return Undefined();
}


/*static*/
JS_METHOD(WebCL::unloadCompiler) {
  cl_int ret = cl::UnloadCompiler();

  if (ret != CL_SUCCESS) {
    return ThrowException(Exception::Error(String::New("UNKNOWN ERROR")));
  }

  return Undefined();
}

