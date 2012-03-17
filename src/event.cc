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
using namespace node;
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

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getProfilingInfo", getProfilingInfo);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_setUserEventStatus", setUserEventStatus);
  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_setCallback", setCallback);

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

JS_METHOD(Event::getInfo)
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
    //cout<<"param: "<<param_name<<" = "<<param_value<<endl;
    return scope.Close(JS_INT(param_value));
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

#include <pthread.h>

struct Baton {
    Persistent<Function> callback;
    int error;
    uv_async_t async;

    // Custom data
    Persistent<Object> data;
};

void callback (cl_event event, cl_int event_command_exec_status, void *user_data)
{
  cout<<"in callback: event="<<event<<" exec status="<<event_command_exec_status<<endl;
  Baton *baton = static_cast<Baton*>(user_data);
  baton->error = event_command_exec_status;

  uv_async_send(&baton->async);
}

void
Event::After_cb(uv_async_t* handle, int status) {
  cout<<"In After_cb"<<endl;

  HandleScope scope;

  Baton *baton = static_cast<Baton*>(handle->data);
  uv_close((uv_handle_t*) &baton->async,NULL);

  Handle<Value> argv[]={
      JS_INT(baton->error),
      baton->data
  };

  TryCatch try_catch;

  baton->callback->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
      FatalException(try_catch);

  baton->callback.Dispose();
  baton->data.Dispose();
  delete baton;
}

JS_METHOD(Event::setCallback)
{
  HandleScope scope;
  Event *e = UnwrapThis<Event>(args);
  cl_int command_exec_callback_type = args[0]->Int32Value();
  Local<Function> fct=Local<Function>::Cast(args[1]);
  Local<Object> data=args[2]->ToObject();

  Baton *baton=new Baton();
  //baton->type=command_exec_callback_type;
  baton->callback=Persistent<Function>::New(fct);
  baton->data=Persistent<Object>::New(data);

  uv_async_init(uv_default_loop(), &baton->async, After_cb);
  baton->async.data=baton;

  cl_int ret=::clSetEventCallback(e->getEvent(), command_exec_callback_type, callback, baton);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(Undefined());
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
