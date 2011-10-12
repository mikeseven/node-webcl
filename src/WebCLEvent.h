/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef WEBCL_EVENT_H_
#define WEBCL_EVENT_H_

#include "common.h"

namespace webcl {

class WebCLEvent : public node::ObjectWrap
{

public:
  ~WebCLEvent();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLEvent *New(cl::Event* ew);

  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getProfilingInfo);
  static JS_METHOD(setUserEventStatus);

  cl::Event *getEvent() { return event; };

private:
  WebCLEvent(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl::Event *event;
};

} // namespace

#endif
