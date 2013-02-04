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

#ifndef COMMANDQUEUE_H_
#define COMMANDQUEUE_H_

#include "common.h"

namespace webcl {

class CommandQueue : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static CommandQueue *New(cl_command_queue cw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
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
  //static JS_METHOD(enqueueWaitForEvents);
  static JS_METHOD(enqueueBarrier);
  static JS_METHOD(flush);
  static JS_METHOD(finish);
  static JS_METHOD(enqueueAcquireGLObjects);
  static JS_METHOD(enqueueReleaseGLObjects);
  // Patch
  static JS_METHOD(_release);

  cl_command_queue getCommandQueue() const { return command_queue; };
  bool isCommandQueue() const { return true; }

private:
  CommandQueue(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_command_queue command_queue;
};

} // namespace

#endif
