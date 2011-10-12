/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_SAMPLER_H_
#define WEBCL_SAMPLER_H_

#include "common.h"

namespace webcl {

class WebCLSampler : public node::ObjectWrap
{

public:
  ~WebCLSampler();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLSampler *New(cl::Sampler* sw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);

  cl::Sampler *getSampler() { return sampler; };

private:
  WebCLSampler(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Sampler *sampler;
};

} // namespace

#endif
