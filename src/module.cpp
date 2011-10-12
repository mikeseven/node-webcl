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
}

NODE_MODULE(_webcl, init);
}


