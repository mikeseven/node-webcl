/*
 * webcl.cc
 *
 *  Created on: Dec 18, 2011
 *      Author: ngk437
 */

#include "webcl.h"
#include "platform.h"
#include "context.h"
#include "device.h"
#include "event.h"

#include <vector>
#include <iostream>
#include <algorithm>

using namespace v8;
using namespace node;
using namespace std;

namespace webcl {

static vector<WebCLObject*> clobjs;
static bool atExit=false;

void registerCLObj(WebCLObject* obj) {
  if(obj) {
    clobjs.push_back(obj);
  }
}

void unregisterCLObj(WebCLObject* obj) {
  if(atExit || !obj) return;

  vector<WebCLObject*>::iterator it = clobjs.begin();
  while(clobjs.size() && it != clobjs.end()) {
    if(*it==obj) {
      clobjs.erase(it);
      break;
    }
    ++it;
  }
}

void AtExit() {
  atExit=true;
  cout<<"WebCL AtExit() called"<<endl;
  cout<<"  # objects allocated: "<<clobjs.size()<<endl;
  vector<WebCLObject*>::const_iterator it = clobjs.begin();
  while(clobjs.size() && it != clobjs.end()) {
    WebCLObject *clo=*it;
    clo->Destructor();
    //v8::Persistent<v8::Value> value = clo->handle_;
    //value.ClearWeak();
    //value.Dispose();
    //cout<<"[atExit] calling destructor of "<<clo<<" thread "<<hex<<pthread_self()<<dec<<endl;
    ++it;
  }

  clobjs.clear();
}

JS_METHOD(getPlatforms) {
  HandleScope scope;

  cl_uint num_entries = 0;
  cl_int ret = ::clGetPlatformIDs(0, NULL, &num_entries);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  cl_platform_id* platforms=new cl_platform_id[num_entries];
  ret = ::clGetPlatformIDs(num_entries, platforms, NULL);
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }


  Local<Array> platformArray = Array::New(num_entries);
  for (int i=0; i<num_entries; i++) {
    platformArray->Set(i, Platform::New(platforms[i])->handle_);
    //cout<<"Found platform: "<<hex<<platforms[i]<<endl;
  }

  delete[] platforms;

  return scope.Close(platformArray);
}

// TODO: no idea what to do with private_info and cb
// Note: this is called only if there is an error in this context during the life of the app
void createContext_callback (const char *errinfo, const void *private_info, size_t cb, void *user_data)
{
  //cout<<"[createContext_callback] thread "<<pthread_self()<<" errinfo ="<<(errinfo ? errinfo : "none")<<endl;
  Baton *baton = static_cast<Baton*>(user_data);
  //cout<<"  baton"<<hex<<baton<<dec<<endl;
  baton->error=0;

  if(errinfo) {
    baton->error_msg=strdup(errinfo);
    baton->error=1;
  }

  //cout<<"  async init"<<endl<<flush;
  uv_async_init(uv_default_loop(), &baton->async, createContext_After_cb);
  uv_async_send(&baton->async);
}

void
createContext_After_cb(uv_async_t* handle, int status) {
  HandleScope scope;

  Baton *baton = static_cast<Baton*>(handle->data);
  uv_close((uv_handle_t*) &baton->async,NULL);
  //cout<<"[createContext::After_cb]  baton: "<<hex<<baton<<dec<<endl;

  Handle<Value> argv[2];
  if(baton->error_msg)
    argv[0]=JS_STR(baton->error_msg);
  else
    argv[0]=JS_INT(CL_SUCCESS);
  argv[1] = baton->data;

  TryCatch try_catch;

  baton->callback->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  if (try_catch.HasCaught())
      node::FatalException(try_catch);

  baton->callback.Dispose();
  baton->data.Dispose();
  if(baton->error_msg) free(baton->error_msg);
  delete baton;
}

JS_METHOD(createContext) {
  HandleScope scope;
  cl_int ret=CL_SUCCESS;
  cl_context cw=NULL;

  // callback handling
  Baton *baton=NULL;
  if(args.Length()==3 && !args[2]->IsUndefined() && args[2]->IsFunction()) {
    baton=new Baton();
    Local<Function> fct=Local<Function>::Cast(args[2]);
    baton->callback=Persistent<Function>::New(fct);
    //cout<<"[createContext] creating baton with callback: "<<*String::AsciiValue(fct->GetName())<<"()";
    //cout<<" at line "<<fct->GetScriptLineNumber()<<endl<<flush;

    if(!args[1]->IsUndefined()) {
      baton->data=Persistent<Value>::New(args[1]);
      //String::AsciiValue str(args[1]);
      //cout<<"  adding user_data '"<<*str<<"' to baton"<<endl<<flush;
    }

    baton->init_thread=pthread_self();

    //uv_async_init(uv_default_loop(), &baton->async, createContext_After_cb);
    baton->async.data=baton;
  }

  // property handling
  if(!args[0]->IsUndefined() && args[0]->IsObject()) {
    Platform *platform = NULL;
    vector<cl_device_id> devices;
    vector<cl_context_properties> properties;
    Local<Array> props = Array::Cast(*args[0]);

    if(props->Has(JS_STR("platform"))) {
      Local<Object> obj = props->Get(JS_STR("platform"))->ToObject();
      if(!obj->IsNull()) {
        platform=ObjectWrap::Unwrap<Platform>(obj);
        properties.push_back(CL_CONTEXT_PLATFORM);
        properties.push_back((cl_context_properties) platform->getPlatformId());
        //cout<<"adding platform "<<hex<<platform->getPlatformId()<<dec<<endl;
      }
    }

    if(props->Has(JS_STR("shareGroup"))) {
      // TODO get WebGL object and retrieve the context from it
      //Local<Object> obj = props->Get(JS_STR("shareGroup"))->ToObject();
      //if(!obj->IsNull()) {
        //platform=ObjectWrap::Unwrap<Platform>(obj);
#if defined (__APPLE__)
        CGLContextObj kCGLContext = CGLGetCurrentContext();
        CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
        properties.push_back(CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE);
        properties.push_back((cl_context_properties) kCGLShareGroup);
#else
  #ifdef _WIN32
        properties.push_back(CL_GL_CONTEXT_KHR);
        properties.push_back((cl_context_properties) wglGetCurrentContext());
        properties.push_back(CL_WGL_HDC_KHR);
        properties.push_back((cl_context_properties) wglGetCurrentDC());
  #else // Unix
        properties.push_back(CL_GL_CONTEXT_KHR);
        properties.push_back((cl_context_properties) glXGetCurrentContext());
        properties.push_back(CL_GLX_DISPLAY_KHR);
        properties.push_back((cl_context_properties) glXGetCurrentDisplay());
  #endif
#endif
      //}
    }

    // terminate properties array if any
    if(properties.size()) properties.push_back(0);

    if(props->Has(JS_STR("deviceType"))) {
      cl_uint device_type=props->Get(JS_STR("deviceType"))->Uint32Value();
      cw = ::clCreateContextFromType(properties.size() ? &properties.front() : NULL,
                                     device_type,
                                     baton ? createContext_callback : NULL,
                                     baton , &ret);
    }
    else if(props->Has(JS_STR("devices"))) {
      Local<Object> obj = props->Get(JS_STR("devices"))->ToObject();
      if(!obj->IsNull()) {
        if(obj->IsArray()) {
          Local<Array> deviceArray = Array::Cast(*obj);
          for (int i=0; i<deviceArray->Length(); i++) {
            Local<Object> obj = deviceArray->Get(i)->ToObject();
            Device *d = ObjectWrap::Unwrap<Device>(obj);
            cout<<"adding device "<<hex<<d->getDevice()<<dec<<endl;
            devices.push_back(d->getDevice());
          }
        }
        else {
          Device *d = ObjectWrap::Unwrap<Device>(obj);
          devices.push_back(d->getDevice());
        }
      }

      //cout<<"[createContext] creating context with devices"<<endl<<flush;
      cw = ::clCreateContext(properties.size() ? &properties.front() : NULL,
                             devices.size(), &devices.front(),
                             baton ? createContext_callback : NULL,
                             baton , &ret);
    }
    else
      return scope.Close(ThrowError("Invalid parameters"));
  }

  // automatic context creation
  else if(args[0]->IsUndefined()) {
#if defined (__APPLE__) || defined(MACOSX)
    CGLContextObj kCGLContext = CGLGetCurrentContext();
    cout<<"using CGL context: "<<hex<<kCGLContext<<dec<<endl;
    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
    cl_context_properties props[] =
    {
        CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, (cl_context_properties)kCGLShareGroup,
        0
    };
    cout<<"[createContext] creating context"<<endl<<flush;
    cw = clCreateContext(props, 0, 0,
        baton ? createContext_callback : NULL /*clLogMessagesToStdoutAPPLE*/,
        baton /*0*/, &ret);

    if (!cw)
    {
        ThrowError("Error: Failed to create a compute context!");
        return scope.Close(Undefined());
    }
    cout<<"Apple OpenCL SharedGroup context created"<<endl;

#else
    #ifndef _WIN32
    cl_context_properties props[] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
        CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
        0
    };
    cw = clCreateContext(props, 0, 0,
        baton ? createContext_callback : NULL,
        baton , &ret);

    if (!cw)
    {
        return scope.Close(ThrowError("Error: Failed to create a compute context!"));
    }
    #else
    // TODO add automatic context creation for Unix and Win32
    return scope.Close(ThrowError("Unsupported createContext() without parameters"));
    #endif
#endif
  }

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_PLATFORM);
    REQ_ERROR_THROW(CL_INVALID_PROPERTY);
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_DEVICE_TYPE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_AVAILABLE);
    REQ_ERROR_THROW(CL_DEVICE_NOT_FOUND);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return scope.Close(Context::New(cw)->handle_);
}

JS_METHOD(waitForEvents) {
  HandleScope scope;

  if (!args[0]->IsArray())
    ThrowError("CL_INVALID_VALUE");

  Local<Array> eventsArray = Array::Cast(*args[0]);
  std::vector<cl_event> events;
  for (int i=0; i<eventsArray->Length(); i++) {
   Event *we=ObjectWrap::Unwrap<Event>(eventsArray->Get(i)->ToObject());
    cl_event e = we->getEvent();
    //cout<<"Waiting for event "<<e<<endl<<flush;
    events.push_back(e);
  }
  cl_int ret=::clWaitForEvents( events.size(), &events.front());
  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(CL_INVALID_VALUE);
    REQ_ERROR_THROW(CL_INVALID_CONTEXT);
    REQ_ERROR_THROW(CL_INVALID_EVENT);
    REQ_ERROR_THROW(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST);
    REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
    REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}


JS_METHOD(unloadCompiler) {
  cl_int ret = ::clUnloadCompiler();

  if (ret != CL_SUCCESS) {
    return ThrowError("UNKNOWN ERROR");
  }

  return Undefined();
}


}
