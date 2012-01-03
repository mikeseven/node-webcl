/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef COMMANDQUEUE_H_
#define COMMANDQUEUE_H_

#include "common.h"

namespace webcl {

class CommandQueue : public node::ObjectWrap, public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static CommandQueue *New(cl_command_queue cw);
  static JS_METHOD(New);

  static JS_METHOD(getCommandQueueInfo);
  static JS_METHOD(enqueueNDRangeKernel);
  static JS_METHOD(enqueueTask);
  static JS_METHOD(enqueueWriteBuffer);
  static JS_METHOD(enqueueReadBuffer);
  static JS_METHOD(enqueueCopyBuffer);
  static JS_METHOD(enqueueWriteBufferRect);
  static JS_METHOD(enqueueReadBufferRect);
  static JS_METHOD(enqueueCopyBufferRect);
  static JS_METHOD(enqueueWriteImage);
  static JS_METHOD(enqueueReadImage);
  static JS_METHOD(enqueueCopyImage);
  static JS_METHOD(enqueueCopyImageToBuffer);
  static JS_METHOD(enqueueCopyBufferToImage);
  static JS_METHOD(enqueueMapBuffer);
  static JS_METHOD(enqueueMapImage);
  static JS_METHOD(enqueueUnmapMemObject);
  static JS_METHOD(enqueueMarker);
  static JS_METHOD(enqueueWaitForEvents);
  static JS_METHOD(enqueueBarrier);
  static JS_METHOD(flush);
  static JS_METHOD(finish);

  cl_command_queue getCommandQueue() const { return command_queue; };

private:
  CommandQueue(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_command_queue command_queue;
};

} // namespace

#endif
