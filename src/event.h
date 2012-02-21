/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef EVENT_H_
#define EVENT_H_

#include "common.h"

namespace webcl {

class Event : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static Event *New(cl_event ew);

  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getProfilingInfo);
  static JS_METHOD(setUserEventStatus);

  cl_event getEvent() const { return event; };

private:
  Event(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_event event;
};

} // namespace

#endif
