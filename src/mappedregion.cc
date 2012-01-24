/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/


#include "mappedregion.h"

#include <iostream>
using namespace std;

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> MappedRegion::constructor_template;

/* static  */
void MappedRegion::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(MappedRegion::New);
  constructor_template = Persistent<FunctionTemplate>::New(t);

  constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
  constructor_template->SetClassName(String::NewSymbol("WebCLMappedRegion"));

  NODE_SET_PROTOTYPE_METHOD(constructor_template, "getBuffer", getBuffer);

  target->Set(String::NewSymbol("WebCLMappedRegion"), constructor_template->GetFunction());
}

MappedRegion::MappedRegion(Handle<Object> wrapper) : region(0)
{
}

void MappedRegion::Destructor()
{
  cout<<"  Destroying MappedRegion"<<endl;
  if (region) delete region;
  region=0;
}

/* static  */
JS_METHOD(MappedRegion::New)
{
  HandleScope scope;
  MappedRegion *mr = new MappedRegion(args.This());
  mr->Wrap(args.This());
  registerCLObj(mr);
  return scope.Close(args.This());
}

/* static  */
MappedRegion *MappedRegion::New(_MappedRegion* region)
{

  HandleScope scope;

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<Object> obj = constructor_template->GetFunction()->NewInstance(1, &arg);

  MappedRegion *mapped_region = ObjectWrap::Unwrap<MappedRegion>(obj);
  mapped_region->region = region;

  return mapped_region;
}

JS_METHOD(MappedRegion::getBuffer)
{
  HandleScope scope;
  MappedRegion *region = UnwrapThis<MappedRegion>(args);

  return scope.Close(region->getMappedRegion()->buffer->handle_);
}

}
