/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_MEMORYOBJECT_H_
#define WEBCL_MEMORYOBJECT_H_

#include "common.h"

namespace webcl {

class WebCLMemory : public node::ObjectWrap
{

public:
  ~WebCLMemory();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLMemory *New(cl::Memory* mw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getImageInfo);
  static JS_METHOD(createSubBuffer);

  cl::Memory *getMemory() { return memory; };

private:
  WebCLMemory(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Memory *memory;
};

} // namespace

#endif
