/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef KERNEL_H_
#define KERNEL_H_

#include "common.h"

namespace webcl {
namespace types {

enum CLType {
  CHAR      = 0,
  UCHAR     = 1,
  SHORT     = 2,
  USHORT    = 3,
  INT       = 4,
  UINT      = 5,
  LONG      = 6,
  ULONG     = 7,
  FLOAT     = 8,
  HALF      = 9,
  DOUBLE    = 10,
  QUAD      = 11,
  LONG_LONG = 12,

  VEC2      = 1<<16,
  VEC3      = 1<<17,
  VEC4      = 1<<18,
  VEC8      = 1<<19,
  VEC16     = 1<<20,

  LOCAL_MEMORY_SIZE = 0xFF
};

} // namespace types

class Kernel : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static Kernel *New(cl_kernel kw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getWorkGroupInfo);
  static JS_METHOD(setArg);

  cl_kernel getKernel() const { return kernel; };

private:
  Kernel(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_kernel kernel;
};

} // namespace

#endif
