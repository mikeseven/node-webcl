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

#include "event.h"
#include "context.h"
#include "commandqueue.h"

using namespace node;
using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Event::constructor_template;

void Event::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Event::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(JS_STR("WebCLEvent"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getProfilingInfo", getProfilingInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_setUserEventStatus", setUserEventStatus);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_setCallback", setCallback);
  // Patch
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  // attributes
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  proto->SetAccessor(JS_STR("status"), GetStatus, NULL);
  proto->SetAccessor(JS_STR("buffer"), GetBuffer, NULL);

  target->Set(JS_STR("WebCLEvent"), ctor->GetFunction());
}

Event::Event(Handle<Object> wrapper) : callback(NULL), event(0), status(0)
{
}

void Event::Destructor()
{
  if(event) {
#ifdef LOGGING
    cout<<"  Destroying CL event "<<event<<endl;//<<" thread: 0x"<<hex<<pthread_self()<<dec<<endl;
#endif
    ::clReleaseEvent(event);
  }
  event=0;
}

NAN_METHOD(Event::release)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());
  
  DESTROY_WEBCL_OBJECT(e);
  
  NanReturnUndefined();
}

NAN_METHOD(Event::getInfo)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());
  cl_event_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_EVENT_CONTEXT:{
    cl_context param_value=NULL;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_context), &param_value, NULL);
    NanReturnValue(Context::New(param_value)->handle());
  }
  case CL_EVENT_COMMAND_QUEUE:{
    cl_command_queue param_value=NULL;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_command_queue), &param_value, NULL);
    NanReturnValue(CommandQueue::New(param_value)->handle());
  }
  case CL_EVENT_REFERENCE_COUNT:
  case CL_EVENT_COMMAND_TYPE:
  case CL_EVENT_COMMAND_EXECUTION_STATUS: {
    cl_uint param_value=0;
    ::clGetEventInfo(e->getEvent(), param_name, sizeof(cl_uint), &param_value, NULL);
    NanReturnValue(Integer::NewFromUnsigned(param_value));
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }

}

NAN_METHOD(Event::getProfilingInfo)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());
  cl_event_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_PROFILING_COMMAND_QUEUED:
  case CL_PROFILING_COMMAND_SUBMIT:
  case CL_PROFILING_COMMAND_START:
  case CL_PROFILING_COMMAND_END: {
    cl_ulong param_value=0;
    ::clGetEventProfilingInfo(e->getEvent(), param_name, sizeof(cl_ulong), &param_value, NULL);
    NanReturnValue(JS_INT((int32_t)param_value));
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }
}

NAN_METHOD(Event::setUserEventStatus)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());

  cl_int ret=::clSetUserEventStatus(e->getEvent(),args[0]->Int32Value());
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_OPERATION);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

void Event::setEvent(cl_event e) {
  Destructor();
  event=e;
}
/*
void Event::callback (cl_event event, cl_int event_command_exec_status, void *user_data)
{
  //cout<<"[Event::callback] event="<<event<<" exec status="<<event_command_exec_status<<endl;
  Baton *baton = static_cast<Baton*>(user_data);
  baton->error = event_command_exec_status;

  Event *e = ObjectWrap::Unwrap<Event>(baton->parent);
  e->status=event_command_exec_status;

  uv_async_init(uv_default_loop(), &baton->async, After_cb);
  uv_async_send(&baton->async);
}

void
Event::After_cb(uv_async_t* handle, int status) {
  //cout<<"In Event::After_cb"<<endl;

  NanScope();

  Baton *baton = static_cast<Baton*>(handle->data);
  uv_close((uv_handle_t*) &baton->async,NULL);

  Handle<Value> argv[]={
      baton->parent,
      baton->data
  };

  TryCatch try_catch;

  baton->callback->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
      FatalException(try_catch);

  baton->callback.Dispose();
  baton->parent.Dispose();
  baton->data.Dispose();
  delete baton;
}

NAN_METHOD(Event::setCallback)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());
  cl_int command_exec_callback_type = args[0]->Int32Value();
  Local<Function> fct=Local<Function>::Cast(args[1]);
  Local<Value> data=args[2];

  Baton *baton=new Baton();
  //baton->type=command_exec_callback_type;
  baton->callback=Persistent<Function>::New(fct);
  baton->data=Persistent<Value>::New(data);
  baton->parent=Persistent<Object>::New(e->handle_);

  baton->async.data=baton;

  cl_int ret=::clSetEventCallback(e->getEvent(), command_exec_callback_type, callback, baton);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(Undefined());
}*/

class EventWorker : public NanAsyncWorker {
 public:
  EventWorker(
    Local<Object> data,
    Local<Object> parent,
    NanCallback *callback)
    : NanAsyncWorker(callback) {
      NanScope();
      SavePersistent("data", data);
      SavePersistent("parent", parent);
    }

  ~EventWorker() {}

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    // estimate = Estimate(points);
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

    /*Local<Value> argv[] = {
        NanNewLocal<Value>(Null())
      , Number::New(estimate)
    };

    callback->Call(2, argv);*/
  };

 private:
};

NAN_METHOD(Event::setCallback)
{
  NanScope();
  Event *e = ObjectWrap::Unwrap<Event>(args.This());
  cl_int command_exec_callback_type = args[0]->Int32Value();
  Local<Value> data=args[2];

  e->callback=new NanCallback(args[1].As<Function>());
  NanAsyncQueueWorker(new EventWorker(
    data.As<Object>(),
    e->handle(),
    e->callback
  ));

  NanReturnUndefined();
}

NAN_GETTER(Event::GetStatus) {
  Event *event = ObjectWrap::Unwrap<Event>(args.Holder());
  NanReturnValue(JS_INT(event->status));
}

// TODO buffer can only be set by enqueueReadBuffer/ReadBufferRect/Image
// TODO update callback to return the event object, not the status
NAN_GETTER(Event::GetBuffer) {
  //Event *event = ObjectWrap::Unwrap<Event>(args.Holder());
  NanReturnUndefined();
}

NAN_METHOD(Event::New)
{
  NanScope();
  Event *e = new Event(args.This());
  e->Wrap(args.This());
  registerCLObj(e);
  NanReturnValue(args.This());
}

Event *Event::New(cl_event ew)
{
  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Event *e = ObjectWrap::Unwrap<Event>(obj);
  e->event = ew;

  return e;
}

} // namespace
