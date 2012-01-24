/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#include "sampler.h"
#include "platform.h"
#include "context.h"

#include <iostream>
using namespace std;

using namespace v8;
using namespace node;

namespace webcl {

Persistent<FunctionTemplate> Sampler::constructor_template;

void Sampler::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(Sampler::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLSampler"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "_getSamplerInfo", getSamplerInfo);

  target->Set(String::NewSymbol("WebCLSampler"), constructor_template->GetFunction());
}

Sampler::Sampler(Handle<Object> wrapper) : sampler(0)
{
}

void Sampler::Destructor() {
  cout<<"  Destroying CL sampler"<<endl;
  if(sampler) ::clReleaseSampler(sampler);
  sampler=0;
}

JS_METHOD(Sampler::getSamplerInfo)
{
  HandleScope scope;
  Sampler *sampler = UnwrapThis<Sampler>(args);
  cl_sampler_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_SAMPLER_ADDRESSING_MODE:
  case CL_SAMPLER_FILTER_MODE:
  case CL_SAMPLER_NORMALIZED_COORDS:
  case CL_SAMPLER_REFERENCE_COUNT: {
    cl_uint param_value=0;
    cl_int ret=::clGetSamplerInfo(sampler->getSampler(), param_name,sizeof(cl_uint), &param_value, NULL);
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
    cl_int ret=::clGetSamplerInfo(sampler->getSampler(), param_name,sizeof(cl_context), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(CL_INVALID_VALUE);
      REQ_ERROR_THROW(CL_INVALID_SAMPLER);
      REQ_ERROR_THROW(CL_OUT_OF_RESOURCES);
      REQ_ERROR_THROW(CL_OUT_OF_HOST_MEMORY);
      return ThrowError("UNKNOWN ERROR");
    }
    return scope.Close(Context::New(param_value)->handle_);
  }
  default:
    return ThrowError("UNKNOWN param_name");
  }

}

JS_METHOD(Sampler::New)
{
  if (!args.IsConstructCall())
    return ThrowTypeError("Constructor cannot be called as a function.");

  HandleScope scope;
  Sampler *s = new Sampler(args.This());
  s->Wrap(args.This());
  registerCLObj(s);
  return scope.Close(args.This());
}

Sampler *Sampler::New(cl_sampler sw)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  Sampler *sampler = ObjectWrap::Unwrap<Sampler>(obj);
  sampler->sampler = sw;

  return sampler;
}

}
