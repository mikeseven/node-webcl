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
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
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
  cl_int ret=CL_SUCCESS;
  cl::Context *cw=NULL;

  if (args[0]->IsArray()) {
    if (!args[1]->IsArray())
      JS_EXCEPTION("CL_INVALID_VALUE");

    Local<Array> deviceArray = Array::Cast(*args[0]);
    VECTOR_CLASS<cl::Device> devices;
    for (int i=0; i<deviceArray->Length(); i++) {
      Local<Object> obj = deviceArray->Get(i)->ToObject();
      WebCLDevice *d = ObjectWrap::Unwrap<WebCLDevice>(obj);
      devices.push_back(*d->getDevice());
    }

    Local<Array> propertiesArray = Array::Cast(*args[1]);
    cl_context_properties *properties = new cl_context_properties[propertiesArray->Length()+1];

    for (int i=0; i<propertiesArray->Length(); i+=2) {
      properties[i] = (cl_context_properties)propertiesArray->Get(i)->NumberValue();
    }
    properties[propertiesArray->Length()] = 0;

    // TODO handle callback arg

    cw=new cl::Context(devices, properties,NULL,NULL, &ret);
    delete[] properties;
  }
  else if(args[0]->IsNumber()) {
    if (!args[1]->IsArray()) {
      JS_EXCEPTION("CL_INVALID_VALUE");
    }

    cl_device_type device_type = args[0]->Uint32Value();

    Local<Array> propertiesArray = Array::Cast(*args[1]);
    int num=propertiesArray->Length();
    cl_context_properties *properties=new cl_context_properties[num+1];
    for (int i=0; i<num; i+=2) {

      properties[i]=propertiesArray->Get(i)->Uint32Value();

      Local<Object> obj = propertiesArray->Get(i+1)->ToObject();
      WebCLPlatform *platform = ObjectWrap::Unwrap<WebCLPlatform>(obj);
      properties[i+1]=(cl_context_properties) platform->getPlatform()->operator ()();
    }
    properties[num]=0;

    cw = new cl::Context(device_type,properties,NULL,NULL,&ret);
  }
  else
    return JS_EXCEPTION("CL_INVALID_VALUE for argument 0");

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_PROPERTY);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_AVAILABLE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return scope.Close(WebCLContext::New(cw)->handle_);
}

/*static*/
JS_METHOD(WebCL::waitForEvents) {
  if (!args[0]->IsArray())
    JS_EXCEPTION("CL_INVALID_VALUE");

  Local<Array> eventsArray = Array::Cast(*args[0]);
  VECTOR_CLASS<cl::Event> events;
  for (int i=0; i<eventsArray->Length(); i++) {
    Local<Object> obj = eventsArray->Get(i)->ToObject();
    cl::Event *e = ObjectWrap::Unwrap<WebCLEvent>(obj)->getEvent();
    events.push_back(*e);
  }
  cl_int ret=cl::Event::waitForEvents(events);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return Undefined();
}


/*static*/
JS_METHOD(WebCL::unloadCompiler) {
  cl_int ret = cl::UnloadCompiler();

  if (ret != CL_SUCCESS) {
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return Undefined();
}

