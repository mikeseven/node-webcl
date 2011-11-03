/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "WebCLPlatform.h"
#include "WebCLSampler.h"
#include "WebCLContext.h"

using namespace v8;
using namespace webcl;

Persistent<FunctionTemplate> WebCLSampler::constructor_template;

/* static  */
void WebCLSampler::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLSampler::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLSampler"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getInfo", getInfo);

  target->Set(String::NewSymbol("WebCLSampler"), constructor_template->GetFunction());
}

WebCLSampler::WebCLSampler(Handle<Object> wrapper) : sampler(0)
{
}

WebCLSampler::~WebCLSampler()
{
  if (sampler) delete sampler;
}

/* static */
JS_METHOD(WebCLSampler::getInfo)
{
  HandleScope scope;
  WebCLSampler *sampler = UnwrapThis<WebCLSampler>(args);
  cl_sampler_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_SAMPLER_ADDRESSING_MODE:
  case CL_SAMPLER_FILTER_MODE:
  case CL_SAMPLER_NORMALIZED_COORDS:
  case CL_SAMPLER_REFERENCE_COUNT: {
    cl_uint param_value=0;
    cl_int ret=sampler->getSampler()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_SAMPLER);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Integer::NewFromUnsigned(param_value));
  }
  case CL_SAMPLER_CONTEXT:{
    cl_context param_value=0;
    cl_int ret=sampler->getSampler()->getInfo(param_name,&param_value);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_SAMPLER);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    cl::Context *cw = new cl::Context(param_value);
    return scope.Close(WebCLContext::New(cw)->handle_);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }

}

/* static  */
JS_METHOD(WebCLSampler::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  WebCLSampler *cl = new WebCLSampler(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLSampler *WebCLSampler::New(cl::Sampler* sw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLSampler *sampler = ObjectWrap::Unwrap<WebCLSampler>(obj);
  sampler->sampler = sw;

  return sampler;
}
