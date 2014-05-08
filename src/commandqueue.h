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
  static NAN_METHOD(New);

  static NAN_METHOD(getInfo);
  static NAN_METHOD(enqueueNDRangeKernel);
  static NAN_METHOD(enqueueTask);
  static NAN_METHOD(enqueueWriteBuffer);
  static NAN_METHOD(enqueueReadBuffer);
  static NAN_METHOD(enqueueCopyBuffer);
  static NAN_METHOD(enqueueWriteBufferRect);
  static NAN_METHOD(enqueueReadBufferRect);
  static NAN_METHOD(enqueueCopyBufferRect);
  static NAN_METHOD(enqueueWriteImage);
  static NAN_METHOD(enqueueReadImage);
  static NAN_METHOD(enqueueCopyImage);
  static NAN_METHOD(enqueueCopyImageToBuffer);
  static NAN_METHOD(enqueueCopyBufferToImage);
  static NAN_METHOD(enqueueMapBuffer);
  static NAN_METHOD(enqueueMapImage);
  static NAN_METHOD(enqueueUnmapMemObject);
  static NAN_METHOD(enqueueMarker);
  //static NAN_METHOD(enqueueWaitForEvents);
  static NAN_METHOD(enqueueBarrier);
  static NAN_METHOD(flush);
  static NAN_METHOD(finish);
  static NAN_METHOD(enqueueAcquireGLObjects);
  static NAN_METHOD(enqueueReleaseGLObjects);
  static NAN_METHOD(release);

  cl_command_queue getCommandQueue() const { return command_queue; };
  bool isCommandQueue() const { return true; }

private:
  CommandQueue(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_command_queue command_queue;
};

} // namespace

#endif
