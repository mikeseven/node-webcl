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

#include <GL/gl.h>
#include <GL/glx.h>

#include <vector>
#include <iostream>

using namespace v8;
using namespace node;
using namespace std;

namespace webcl {

vector<WebCLObject*> clobjs;

void registerCLObj(WebCLObject* obj) {
  if(obj) clobjs.push_back(obj);
}


void unregisterCLObj(WebCLObject* obj) {
  if(!obj) return;

  vector<WebCLObject*>::iterator it = clobjs.begin();
  while(it != clobjs.end()) {
    if(*it==obj) {
      clobjs.erase(it);
      break;
    }
    it++;
  }
}

void AtExit() {
  cout<<"WebCL AtExit() called"<<endl;
  cout<<"# objects allocated: "<<clobjs.size()<<endl;
  vector<WebCLObject*>::iterator it = clobjs.begin();
  while(it != clobjs.end())
    (*it++)->Destructor();

  clobjs.clear();
}

JS_METHOD(getPlatforms) {
  HandleScope scope;

  cl_uint num_platforms;

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
    cout<<"Found platform: "<<hex<<platforms[i]<<endl;
  }

  delete[] platforms;

  return scope.Close(platformArray);
}

JS_METHOD(createContext) {
  HandleScope scope;
  cl_int ret=CL_SUCCESS;
  cl_context cw=NULL;

  if (args[0]->IsArray() || args[0]->IsObject()) {
    vector<cl_device_id> devices;
    if(args[0]->IsArray()) {
      Local<Array> deviceArray = Array::Cast(*args[0]);
      for (int i=0; i<deviceArray->Length(); i++) {
        Local<Object> obj = deviceArray->Get(i)->ToObject();
        Device *d = ObjectWrap::Unwrap<Device>(obj);
        //cout<<"adding device "<<hex<<d->getDevice()<<dec<<endl;
        devices.push_back(d->getDevice());
      }
    }
    else {
      Device *d = ObjectWrap::Unwrap<Device>(args[0]->ToObject());
      devices.push_back(d->getDevice());
    }

    vector<cl_context_properties> properties;
    int num=0;
    if(!(args[1]->IsUndefined() || args[1]->IsNull())) {
      Local<Array> propertiesArray = Array::Cast(*args[1]);
      num=propertiesArray->Length();
      for (int i=0; i<num; i+=2) {
        uint32_t prop=propertiesArray->Get(i)->Uint32Value();
        properties.push_back(prop);

        if( prop==CL_CONTEXT_PLATFORM) {
          Local<Object> obj = propertiesArray->Get(i+1)->ToObject();
          Platform *platform = ObjectWrap::Unwrap<Platform>(obj);
          properties.push_back((cl_context_properties) platform->getPlatformId());
        }
        else if(prop==CL_GL_CONTEXT_KHR) {
          properties.push_back((cl_context_properties) glXGetCurrentContext());
          properties.push_back(CL_GLX_DISPLAY_KHR);
          properties.push_back((cl_context_properties) glXGetCurrentDisplay());
        }
      }
      properties.push_back(0);
    }

    // TODO handle callback arg
    cw = ::clCreateContext(properties.size() ? &properties.front() : NULL, devices.size(), &devices.front(), NULL /*notifyFptr*/, NULL /*data*/, &ret);
  }
  else if(args[0]->IsNumber()) {
    if (!args[1]->IsArray()) {
      ThrowError("CL_INVALID_VALUE");
    }

    cl_device_type device_type = args[0]->Uint32Value();
    cout<<"Creating context for device type: "<<device_type<<endl;

    Local<Array> propertiesArray = Array::Cast(*args[1]);
    int num=propertiesArray->Length();
    cl_context_properties *properties=new cl_context_properties[num+1];
    for (int i=0; i<num; i+=2) {

      properties[i]=propertiesArray->Get(i)->Uint32Value();

      Local<Object> obj = propertiesArray->Get(i+1)->ToObject();
      Platform *platform = ObjectWrap::Unwrap<Platform>(obj);
      properties[i+1]=(cl_context_properties) platform->getPlatformId();
      //cout<<"platform id: "<<hex<<properties[i+1]<<dec<<endl;
    }
    properties[num]=0;

    // TODO handle callback arg
    cw = ::clCreateContextFromType(properties, device_type, NULL /*notifyFptr*/, NULL /*data*/, &ret);
    delete[] properties;
  }
  else
    return ThrowError("CL_INVALID_VALUE for argument 0");

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
    Local<Object> obj = eventsArray->Get(i)->ToObject();
    Event *we=ObjectWrap::Unwrap<Event>(obj);
    cl_event e = we->getEvent();
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
