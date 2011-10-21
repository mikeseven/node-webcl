/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_CONTEXT_H_
#define WEBCL_CONTEXT_H_

#include "common.h"

namespace webcl {

class WebCLContext : public node::ObjectWrap
{

public:
  ~WebCLContext();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLContext *New(cl::Context* cw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(createProgramWithSource);
  static JS_METHOD(createCommandQueue);
  static JS_METHOD(createBuffer);
  static JS_METHOD(createBufferGL);
  static JS_METHOD(createImage2D);
  static JS_METHOD(createImage3D);
  static JS_METHOD(createSampler);
  static JS_METHOD(createUserEvent);
  static JS_METHOD(getSupportedImageFormats);

  cl::Context *getContext() { return context; };

private:
  WebCLContext(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Context *context;
};

} // namespace

#endif
