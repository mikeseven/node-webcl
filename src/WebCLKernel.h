/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_KERNELOBJECT_H_
#define WEBCL_KERNELOBJECT_H_

#include "common.h"

namespace webcl {

class WebCLKernel : public node::ObjectWrap
{

public:
  ~WebCLKernel();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLKernel *New(cl::Kernel* kw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getWorkGroupInfo);
  static JS_METHOD(setArg);

  cl::Kernel *getKernel() { return kernel; };

private:
  WebCLKernel(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Kernel *kernel;
};

} // namespace

#endif
