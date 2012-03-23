/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef PROGRAM_H_
#define PROGRAM_H_

#include "common.h"

namespace webcl {

class Program : public WebCLObject
{

public:
  void Destructor();

  static void Init(v8::Handle<v8::Object> target);

  static Program *New(cl_program pw);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getBuildInfo);
  static JS_METHOD(build);
  static JS_METHOD(createKernel);

  cl_program getProgram() const { return program; };

private:
  Program(v8::Handle<v8::Object> wrapper);

  static void After_cb(uv_async_t* handle, int status);
  static void callback (cl_program program, void *user_data);

  // using uv_work_queue
  //static void AsyncWork(uv_work_t* req);
  //static void AsyncAfter(uv_work_t* req);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_program program;
};

} // namespace

#endif
