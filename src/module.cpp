/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "common.h"
#include "WebCLPlatform.h"
#include "WebCLDevice.h"
#include "WebCLContext.h"
#include "WebCLMemory.h"
#include "WebCLProgram.h"
#include "WebCLKernel.h"
#include "WebCLCommandQueue.h"
#include "WebCLEvent.h"
#include "WebCLSampler.h"
#include "WebCL.h"
#include "WebCLMappedRegion.h"

#define NODE_DEFINE_CONSTANT_VALUE(target, name, value)                   \
  (target)->Set(v8::String::NewSymbol(name),                         \
                v8::Integer::New(value),                               \
                static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

//
extern "C" {
using namespace webcl;

static void init (v8::Handle<v8::Object> target)
{
  WebCL::Init(target);
  WebCLPlatform::Init(target);
  WebCLDevice::Init(target);
  WebCLContext::Init(target);
  WebCLMemory::Init(target);
  WebCLProgram::Init(target);
  WebCLKernel::Init(target);
  WebCLCommandQueue::Init(target);
  WebCLEvent::Init(target);
  WebCLSampler::Init(target);
  WebCLMappedRegion::Init(target);

  /**
   * Platform-dependent byte sizes
   */
  NODE_DEFINE_CONSTANT_VALUE(target, "CHAR", sizeof(char));
  NODE_DEFINE_CONSTANT_VALUE(target, "SHORT", sizeof(short));
  NODE_DEFINE_CONSTANT_VALUE(target, "INT", sizeof(int));
  NODE_DEFINE_CONSTANT_VALUE(target, "LONG", sizeof(long));
  NODE_DEFINE_CONSTANT_VALUE(target, "FLOAT", sizeof(float));
  NODE_DEFINE_CONSTANT_VALUE(target, "DOUBLE", sizeof(double));
  NODE_DEFINE_CONSTANT_VALUE(target, "HALF", sizeof(float) >> 1);

}

NODE_MODULE(_webcl, init);
}


