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

class Context : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static Context *New(cl_context cw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(createProgram);
  static JS_METHOD(createCommandQueue);
  static JS_METHOD(createBuffer);
  static JS_METHOD(createImage2D);
  static JS_METHOD(createImage3D);
  static JS_METHOD(createSampler);
  static JS_METHOD(createUserEvent);
  static JS_METHOD(getSupportedImageFormats);
  static JS_METHOD(createFromGLBuffer);
  static JS_METHOD(createFromGLTexture2D);
  static JS_METHOD(createFromGLTexture3D);
  static JS_METHOD(createFromGLRenderbuffer);

  cl_context getContext() const { return context; };

private:
  Context(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_context context;
};

} // namespace

#endif
