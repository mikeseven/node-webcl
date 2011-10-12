/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_PLATFORM_H_
#define WEBCL_PLATFORM_H_

#include "common.h"

namespace webcl {

class WebCLPlatform : public node::ObjectWrap
{

public:
  ~WebCLPlatform();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLPlatform *New(cl::Platform* pw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getDevices);

  cl::Platform *getPlatform() { return platform; };

private:
  WebCLPlatform(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Platform *platform;
};

} // namespace

#endif
