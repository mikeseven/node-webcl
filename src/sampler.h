/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef SAMPLER_H_
#define SAMPLER_H_

#include "common.h"

namespace webcl {

class Sampler : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static Sampler *New(cl_sampler sw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);

  cl_sampler getSampler() const { return sampler; };

private:
  Sampler(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_sampler sampler;
};

} // namespace

#endif
