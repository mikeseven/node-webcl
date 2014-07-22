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

#include "sampler.h"
#include "platform.h"
#include "context.h"

using namespace v8;
using namespace node;

namespace webcl {

Persistent<FunctionTemplate> Sampler::constructor_template;

void Sampler::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Sampler::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLSampler"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);

  target->Set(NanSymbol("WebCLSampler"), ctor->GetFunction());
}

Sampler::Sampler(Handle<Object> wrapper) : sampler(0)
{
}

void Sampler::Destructor() {
  #ifdef LOGGING
  cout<<"  Destroying CL sampler"<<endl;
  #endif
  if(sampler) ::clReleaseSampler(sampler);
  sampler=0;
}

NAN_METHOD(Sampler::release)
{
  NanScope();
  Sampler *sampler = ObjectWrap::Unwrap<Sampler>(args.This());
  
  DESTROY_WEBCL_OBJECT(sampler);
  
  NanReturnUndefined();
}

NAN_METHOD(Sampler::getInfo)
{
  NanScope();
  Sampler *sampler = ObjectWrap::Unwrap<Sampler>(args.This());
  cl_sampler_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_SAMPLER_ADDRESSING_MODE:
  case CL_SAMPLER_FILTER_MODE:
  case CL_SAMPLER_NORMALIZED_COORDS:
  case CL_SAMPLER_REFERENCE_COUNT: {
    cl_uint param_value=0;
    cl_int ret=::clGetSamplerInfo(sampler->getSampler(), param_name,sizeof(cl_uint), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_SAMPLER);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned(param_value));
  }
  case CL_SAMPLER_CONTEXT:{
    cl_context param_value=0;
    cl_int ret=::clGetSamplerInfo(sampler->getSampler(), param_name,sizeof(cl_context), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_SAMPLER);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(NanObjectWrapHandle(Context::New(param_value)));
  }
  default:
    return NanThrowError("UNKNOWN param_name");
  }

}

NAN_METHOD(Sampler::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();
  Sampler *s = new Sampler(args.This());
  s->Wrap(args.This());
  registerCLObj(s);
  NanReturnValue(args.This());
}

Sampler *Sampler::New(cl_sampler sw)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Sampler *sampler = ObjectWrap::Unwrap<Sampler>(obj);
  sampler->sampler = sw;

  return sampler;
}

}
