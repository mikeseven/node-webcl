/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLEvent.h"
#include "WebCLContext.h"
#include "WebCLCommandQueue.h"

#include <iostream>

using namespace v8;
using namespace webcl;

Persistent<FunctionTemplate> WebCLEvent::constructor_template;

/* static  */
void WebCLEvent::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLEvent::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLEvent"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getProfilingInfo", getProfilingInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "setUserEventStatus", setUserEventStatus);

  target->Set(String::NewSymbol("WebCLEvent"), constructor_template->GetFunction());
}

WebCLEvent::WebCLEvent(Handle<Object> wrapper) : event(0)
{
}

WebCLEvent::~WebCLEvent()
{
  if (event) delete event;
}

/* static  */
JS_METHOD(WebCLEvent::getInfo)
{
  HandleScope scope;
  WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(args.This());
  cl_event_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_EVENT_CONTEXT:{
    cl::Context *param_value=new cl::Context();
    e->getEvent()->getInfo(param_name,param_value);
    return scope.Close(WebCLContext::New(param_value)->handle_);
  }
  case CL_EVENT_COMMAND_QUEUE:{
    cl::CommandQueue *param_value=new cl::CommandQueue();
    e->getEvent()->getInfo(param_name,param_value);
    return scope.Close(WebCLCommandQueue::New(param_value)->handle_);
  }
  case CL_EVENT_REFERENCE_COUNT:
  case CL_EVENT_COMMAND_TYPE:
  case CL_EVENT_COMMAND_EXECUTION_STATUS: {
    cl_uint param_value=0;
    e->getEvent()->getInfo(param_name,&param_value);
    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  default:
    return JS_EXCEPTION("UNKNOWN param_name");
  }

}

/* static  */
JS_METHOD(WebCLEvent::getProfilingInfo)
{
  HandleScope scope;
  WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(args.This());
  cl_event_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_PROFILING_COMMAND_QUEUED:
  case CL_PROFILING_COMMAND_SUBMIT:
  case CL_PROFILING_COMMAND_START:
  case CL_PROFILING_COMMAND_END: {
    cl_ulong param_value=0;
    e->getEvent()->getProfilingInfo(param_name,&param_value);
    return scope.Close(Integer::New(param_value));
  }
  default:
    return JS_EXCEPTION("UNKNOWN param_name");
  }
}

/* static  */
JS_METHOD(WebCLEvent::setUserEventStatus)
{
  HandleScope scope;
  WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(args.This());

  cl_int ret=((cl::UserEvent*) e->getEvent())->setStatus((cl_int)args[0]->IntegerValue());
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return JS_EXCEPTION("UNKNOWN ERROR");
  }

  return Undefined();
}

/* static  */
JS_METHOD(WebCLEvent::New)
{
  HandleScope scope;
  WebCLEvent *cl = new WebCLEvent(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLEvent *WebCLEvent::New(cl::Event* ew)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLEvent *e = ObjectWrap::Unwrap<WebCLEvent>(obj);
  e->event = ew;

  return e;
}

