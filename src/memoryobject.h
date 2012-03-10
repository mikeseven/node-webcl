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
  static JS_METHOD(getGLObjectInfo);

  cl_mem getMemory() const { return memory; };

private:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;

protected:
  MemoryObject(v8::Handle<v8::Object> wrapper);

  cl_mem memory;
};

class WebCLBuffer : public MemoryObject {
public:
  static void Init(v8::Handle<v8::Object> target);

  static WebCLBuffer *New(cl_mem mw);
  static JS_METHOD(New);

  static JS_METHOD(createSubBuffer);

private:
  WebCLBuffer(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

class WebCLImage : public MemoryObject {
public:
  static void Init(v8::Handle<v8::Object> target);

  static WebCLImage *New(cl_mem mw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getGLTextureInfo);

private:
  WebCLImage(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

} // namespace

#endif
