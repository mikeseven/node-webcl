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

#ifndef MEMORYOBJECT_H_
#define MEMORYOBJECT_H_

#include "common.h"

namespace webcl {

class MemoryObject : public WebCLObject
{

public:
  virtual void Destructor();

  static void Init(v8::Handle<v8::Object> exports);

  static MemoryObject *New(cl_mem mw);
  static NAN_METHOD(New);
  static NAN_METHOD(getInfo);
  static NAN_METHOD(getGLObjectInfo);
  static NAN_METHOD(release);

  cl_mem getMemory() const { return memory; };
  virtual bool operator==(void *clObj) { return ((cl_mem)clObj)==memory; }

private:
  static v8::Persistent<v8::Function> constructor;

protected:
  MemoryObject(v8::Handle<v8::Object> wrapper);
  ~MemoryObject();

  cl_mem memory;

private:
  DISABLE_COPY(MemoryObject)
};

class WebCLBuffer : public MemoryObject {
public:
  static void Init(v8::Handle<v8::Object> exports);

  static WebCLBuffer *New(cl_mem mw, WebCLObject *parent);
  static NAN_METHOD(New);
  static NAN_METHOD(getInfo);
  static NAN_METHOD(getGLObjectInfo);
  static NAN_METHOD(release);
  static NAN_METHOD(createSubBuffer);

  bool isSubBuffer() const { return isSubBuffer_; }

private:
  WebCLBuffer(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::Function> constructor;

  bool isSubBuffer_;

private:
  DISABLE_COPY(WebCLBuffer)
};

class WebCLImage : public MemoryObject {
public:
  static void Init(v8::Handle<v8::Object> exports);

  static WebCLImage *New(cl_mem mw, WebCLObject *parent);
  static NAN_METHOD(New);
  static NAN_METHOD(release);
  static NAN_METHOD(getInfo);
  static NAN_METHOD(getGLObjectInfo);
  static NAN_METHOD(getGLTextureInfo);

private:
  WebCLImage(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::Function> constructor;

private:
  DISABLE_COPY(WebCLImage)
};

class WebCLImageDescriptor : public WebCLObject
{
public:
  static void Init(v8::Handle<v8::Object> exports);

  static WebCLImageDescriptor* New(int order=0, int type=0, int w=0, int h=0, int d=0, int rp=0, int sp=0);
  static NAN_METHOD(New);
  static NAN_GETTER(getChannelOrder);
  static NAN_GETTER(getChannelType);
  static NAN_GETTER(getWidth);
  static NAN_GETTER(getHeight);
  static NAN_GETTER(getDepth);
  static NAN_GETTER(getRowPitch);
  static NAN_GETTER(getSlicePitch);

private:
  WebCLImageDescriptor(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::Function> constructor;

  int channelOrder, channelType;
  int width, height, depth;
  int rowPitch, slicePitch;

private:
  DISABLE_COPY(WebCLImageDescriptor)
};

} // namespace

#endif
