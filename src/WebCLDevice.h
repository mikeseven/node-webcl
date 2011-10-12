/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_DEVICE_H_
#define WEBCL_DEVICE_H_

#include "common.h"

namespace webcl {

class WebCLDevice : public node::ObjectWrap
{

public:
  ~WebCLDevice();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLDevice *New(cl::Device* dw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);

  cl::Device *getDevice() { return device; };

private:
  WebCLDevice(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Device *device;
};

} // namespace

#endif
