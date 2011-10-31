/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_H_
#define WEBCL_H_

namespace webcl {
namespace types {

enum CLType {
  LOCAL = 1<<0,
  POINTER = 1<<1,
  UNSIGNED = 1<<2,
  MEM=1<<3,
  COMPLEX = 1<<4,
  IMAGINARY = 1<<5,
  BOOL=1<<8,
  CHAR=1<<9,
  SHORT=1<<10,
  INT=1<<11,
  LONG=1<<12,
  FLOAT=1<<13,
  HALF_FLOAT=1<<14,
  DOUBLE=1<<15,
  QUAD=1<<16,
  LONG_LONG=1<<17,
  V2=1<<20,
  V3=1<<21,
  V4=1<<22,
  V8=1<<23,
  V16=1<<24,
  M2xN=1<<25,
  M3xN=1<<26,
  M4xN=1<<27,
  M8xN=1<<28,
  M16xN=1<<29
};

} // namespace types
} // namespace webcl

#include "common.h"

namespace webcl {
class WebCL : public node::ObjectWrap
{
  WebCL() {}

public:
  static void Init(v8::Handle<v8::Object> target);

  ~WebCL() {}

  static JS_METHOD(New);
  static JS_METHOD(getPlatformIDs);
  static JS_METHOD(createContext);
  static JS_METHOD(waitForEvents);
  static JS_METHOD(unloadCompiler);

private:
  static v8::Persistent<v8::FunctionTemplate> constructor_template;
};

}

#endif /* WEBCL_H_ */
