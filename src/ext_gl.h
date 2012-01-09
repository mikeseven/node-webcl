/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef EXT_GL_H_
#define EXT_GL_H_

#include "common.h"

namespace webcl {

class EXTGL : public WebCLObject
{

public:
  static void Init(v8::Handle<v8::Object> target);

  static EXTGL *New();
  static JS_METHOD(New);

  static JS_METHOD(createFromGLBuffer);
  static JS_METHOD(createFromGLTexture2D);
  static JS_METHOD(createFromGLTexture3D);
  static JS_METHOD(createFromGLRenderbuffer);
  static JS_METHOD(getGLObjectInfo);
  static JS_METHOD(getGLTextureInfo);
  static JS_METHOD(enqueueAcquireGLObjects);
  static JS_METHOD(enqueueReleaseGLObjects);

private:
  EXTGL(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

} // namespace

#endif
