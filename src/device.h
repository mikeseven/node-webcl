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

#ifndef DEVICE_H_
#define DEVICE_H_

#include "common.h"

namespace webcl {

class Device : public WebCLObject
{

public:
  static void Init(v8::Handle<v8::Object> target);

  static Device *New(cl_device_id did);
  static NAN_METHOD(New);
  static NAN_METHOD(getInfo);
  static NAN_METHOD(getSupportedExtensions);

  static NAN_METHOD(enableExtension);
  bool hasGLSharingEnabled() const { return (enableExtensions & GL_SHARING); }
  bool hasFP16Enabled() const { return (enableExtensions & FP16)==FP16; }
  bool hasFP64Enabled() const { return (enableExtensions & FP64)==FP64; }

  cl_device_id getDevice() const { return device_id; };
  virtual bool operator==(void *clObj) { return ((cl_device_id)clObj)==device_id; }

private:
  Device(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_device_id device_id;

  cl_uint enableExtensions;
  cl_uint availableExtensions;

  enum WEBCL_EXTENSIONS {
  	NONE		     = 0x00,
  	GL_SHARING 	 = 0x01,
  	FP16		     = 0x02,
  	FP64		     = 0x04
  };

private:
  DISABLE_COPY(Device)
};

} // namespace


#endif /* DEVICE_H_ */
