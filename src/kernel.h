// Copyright (c) 2011-2012, Motorola Mobility, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of the Motorola Mobility, Inc. nor the names of its
//    contributors may be used to endorse or promote products derived from this
//    software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  //
  static JS_METHOD(release);

  cl_kernel getKernel() const { return kernel; };
  
  bool isKernel() const { return true; }

private:
  Kernel(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_kernel kernel;
};

} // namespace

#endif
