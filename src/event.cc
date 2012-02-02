/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "event.h"
#include "context.h"
#include "commandqueue.h"

#include <iostream>
using namespace std;

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Event::constructor_template;

void Event::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Event::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(JS_STR("WebCLEvent"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getEventInfo", getEventInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getProfilingInfo", getProfilingInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_setUserEventStatus", setUserEventStatus);

  target->Set(JS_STR("WebCLEvent"), constructor_template->GetFunction());
}

Event::Event(Handle<Object> wrapper) : event(0)
{
}

void Event::Destructor()
{
  cout<<"  Destroying CL event "<<event<<" thread: 0x"<<hex<<pthread_self()<<dec<<endl;
  if(event) ::clReleaseEvent(event);
  event=0;
}

JS_METHOD(Event::getEventInfo)
{
  HandleScope scope;
  Event *e = UnwrapThis<Event>(args);
  cl_event_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_EVENT_CONTEXT:{
    cl_context param_value=NULL;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_context), &param_value, NULL);
    return scope.Close(Context::New(param_value)->handle_);
  }
  case CL_EVENT_COMMAND_QUEUE:{
    cl_command_queue param_value=NULL;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_command_queue), &param_value, NULL);
    return scope.Close(CommandQueue::New(param_value)->handle_);
  }
  case CL_EVENT_REFERENCE_COUNT:
  case CL_EVENT_COMMAND_TYPE:
  case CL_EVENT_COMMAND_EXECUTION_STATUS: {
    cl_uint param_value=0;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_uint), &param_value, NULL);
    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }

}

JS_METHOD(Event::getProfilingInfo)
{
  HandleScope scope;
  Event *e = UnwrapThis<Event>(args);
  cl_event_info param_name = args[0]->NumberValue();

  switch (param_name) {
  case CL_PROFILING_COMMAND_QUEUED:
  case CL_PROFILING_COMMAND_SUBMIT:
  case CL_PROFILING_COMMAND_START:
  case CL_PROFILING_COMMAND_END: {
    cl_ulong param_value=0;
    ::clGetEventProfilingInfo(e->getEvent(), param_name, sizeof(cl_ulong), &param_value, NULL);
    return scope.Close(JS_NUM(param_value));
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }
}

JS_METHOD(Event::setUserEventStatus)
{
  HandleScope scope;
  Event *e = UnwrapThis<Event>(args);

  cl_int ret=::clSetUserEventStatus(e->getEvent(),args[0]->Int32Value());
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}

JS_METHOD(Event::New)
{
  HandleScope scope;
  Event *e = new Event(args.This());
  e->Wrap(args.This());
  registerCLObj(e);
  return scope.Close(args.This());
}

Event *Event::New(cl_event ew)
{
  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Event *e = ObjectWrap::Unwrap<Event>(obj);
  e->event = ew;

  return e;
}

} // namespace
