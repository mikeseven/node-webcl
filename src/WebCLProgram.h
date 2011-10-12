/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_PROGRAMOBJECT_H_
#define WEBCL_PROGRAMOBJECT_H_

#include "common.h"

namespace webcl {

class WebCLProgram : public node::ObjectWrap
{

public:
  ~WebCLProgram();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLProgram *New(cl::Program* pw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getBuildInfo);
  static JS_METHOD(build);
  static JS_METHOD(createKernel);

  cl::Program *getProgram() { return program; };

private:
  WebCLProgram(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Program *program;
};

} // namespace

#endif
