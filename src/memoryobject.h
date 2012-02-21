/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef MEMORYOBJECT_H_
#define MEMORYOBJECT_H_

#include "common.h"

namespace webcl {

class MemoryObject : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static MemoryObject *New(cl_mem mw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getImageInfo);
  static JS_METHOD(createSubBuffer);
  static JS_METHOD(getGLTextureInfo);
  static JS_METHOD(getGLObjectInfo);

  cl_mem getMemory() const { return memory; };

private:
  MemoryObject(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_mem memory;
};

} // namespace

#endif
