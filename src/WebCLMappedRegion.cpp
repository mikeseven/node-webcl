/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/


#include "WebCLMappedRegion.h"

using namespace v8;
using namespace webcl;

Persistent<FunctionTemplate> WebCLMappedRegion::constructor_template;

/* static  */
void WebCLMappedRegion::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(WebCLMappedRegion::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("MappedRegion"));

  target->Set(String::NewSymbol("MappedRegion"), constructor_template->GetFunction());
}

WebCLMappedRegion::WebCLMappedRegion(Handle<Object> wrapper) : region(0)
{
}

WebCLMappedRegion::~WebCLMappedRegion()
{
  if (region) delete region;
}

/* static  */
JS_METHOD(WebCLMappedRegion::New)
{
  HandleScope scope;
  WebCLMappedRegion *cl = new WebCLMappedRegion(args.This());
  cl->Wrap(args.This());
  return scope.Close(args.This());
}

/* static  */
WebCLMappedRegion *WebCLMappedRegion::New(_MappedRegion* region)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  WebCLMappedRegion *mapped_region = ObjectWrap::Unwrap<WebCLMappedRegion>(obj);
  mapped_region->region = region;

  return mapped_region;
}

