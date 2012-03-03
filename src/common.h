/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/
#ifndef WEBCL_COMMON_H_
#define WEBCL_COMMON_H_

// OpenCL includes
#if defined (__APPLE__) || defined(MACOSX)
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
    #include <OpenCL/opencl.h>
    #define CL_GL_CONTEXT_KHR 0x2008
    #define CL_EGL_DISPLAY_KHR 0x2009
#else
    #include <GL/gl.h>
    #include <CL/opencl.h>
#endif

// Node includes
#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>
#include <string>
using namespace std;

namespace {
#define JS_STR(...) v8::String::New(__VA_ARGS__)
#define JS_INT(val) v8::Integer::New(val)
#define JS_NUM(val) v8::Number::New(val)
#define JS_BOOL(val) v8::Boolean::New(val)
#define JS_METHOD(name) v8::Handle<v8::Value> name(const v8::Arguments& args)
//#define JS_EXCEPTION(reason) v8::ThrowException(v8::Exception::Error(JS_STR(reason)))
#define JS_RETHROW(tc) v8::Local<v8::Value>::New(tc.Exception());

#define REQ_ARGS(N)                                                     \
  if (args.Length() < (N))                                              \
    return ThrowException(Exception::TypeError(                         \
                             String::New("Expected " #N "arguments")));

#define REQ_STR_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsString())                     \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a string"))); \
  String::Utf8Value VAR(args[I]->ToString());

#define REQ_EXT_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsExternal())                   \
    return ThrowException(Exception::TypeError(                         \
                              String::New("Argument " #I " invalid"))); \
  Local<External> VAR = Local<External>::Cast(args[I]);

#define REQ_FUN_ARG(I, VAR)                                             \
  if (args.Length() <= (I) || !args[I]->IsFunction())                   \
    return ThrowException(Exception::TypeError(                         \
                  String::New("Argument " #I " must be a function")));  \
  Local<Function> VAR = Local<Function>::Cast(args[I]);

#define REQ_ERROR_THROW(error) if (ret == error) return ThrowException(Exception::Error(String::New(#error)));

template <typename T>
static T* UnwrapThis(const v8::Arguments& args) {
  return node::ObjectWrap::Unwrap<T>(args.This());
}

v8::Handle<v8::Value> ThrowError(const char* msg) {
  return v8::ThrowException(v8::Exception::Error(v8::String::New(msg)));
}

v8::Handle<v8::Value> ThrowTypeError(const char* msg) {
  return v8::ThrowException(v8::Exception::TypeError(v8::String::New(msg)));
}

v8::Handle<v8::Value> ThrowRangeError(const char* msg) {
  return v8::ThrowException(v8::Exception::RangeError(v8::String::New(msg)));
}

}

namespace webcl {

class WebCLObject;
void registerCLObj(WebCLObject* obj);
void unregisterCLObj(WebCLObject* obj);
void AtExit();

class WebCLObject : public node::ObjectWrap {
protected:
  virtual ~WebCLObject() {
    Destructor();
    unregisterCLObj(this);
  }

public:
  virtual void Destructor() {}
};

}

#endif
