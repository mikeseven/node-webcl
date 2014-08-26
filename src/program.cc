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

#include "program.h"
#include "device.h"
#include "kernel.h"
#include "context.h"

#include <vector>
#include <cstdlib>
#include <cstring>

using namespace v8;

namespace webcl {

Persistent<FunctionTemplate> Program::constructor;

void Program::Init(Handle<Object> exports)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(Program::New);
  NanAssignPersistent(FunctionTemplate, constructor, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLProgram"));

  // prototype
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getInfo", getInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_getBuildInfo", getBuildInfo);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_build", build);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_createKernel", createKernel);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_createKernelsInProgram", createKernelsInProgram);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_release", release);
  NODE_SET_PROTOTYPE_METHOD(ctor, "_retain", retain);

  exports->Set(NanSymbol("WebCLProgram"), ctor->GetFunction());
}

Program::Program(Handle<Object> wrapper) : program(0)
{
  _type=CLObjType::Program;
}

Program::~Program() {
#ifdef LOGGING
  printf("In ~Program\n");
#endif
  // Destructor();
}

void Program::Destructor() {
  if(program) {
    cl_uint count;
    ::clGetProgramInfo(program,CL_PROGRAM_REFERENCE_COUNT,sizeof(cl_uint),&count,NULL);
#ifdef LOGGING
    cout<<"  Destroying Program, CLrefCount is: "<<count<<endl;
#endif
    ::clReleaseProgram(program);
    if(count==1) {
      unregisterCLObj(this);
      program=0;
    }
  }
}

NAN_METHOD(Program::release)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());
  
  prog->Destructor();
  
  NanReturnUndefined();
}

NAN_METHOD(Program::retain)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());
  
  clRetainProgram(prog->getProgram());
  
  NanReturnUndefined();
}

NAN_METHOD(Program::getInfo)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());
  cl_program_info param_name = args[0]->Uint32Value();

  switch (param_name) {
  case CL_PROGRAM_REFERENCE_COUNT:
  case CL_PROGRAM_NUM_DEVICES: {
    cl_uint value=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(cl_uint), &value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(Integer::NewFromUnsigned(value));
  }
  case CL_PROGRAM_CONTEXT: {
    cl_context value=NULL;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), param_name, sizeof(cl_context), &value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    if(value) {
      WebCLObject *obj=findCLObj((void*)value, CLObjType::Context);
      if(obj) 
        NanReturnValue(NanObjectWrapHandle(obj));
      else
        NanReturnValue(NanObjectWrapHandle(Context::New(value)));
    }
    NanReturnUndefined();
  }
  case CL_PROGRAM_DEVICES: {
    size_t num_devices=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_DEVICES, 0, NULL, &num_devices);
    num_devices /= sizeof(cl_device_id);
    cl_device_id *devices=new cl_device_id[num_devices];
    ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_DEVICES, sizeof(cl_device_id)*num_devices, devices, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    Local<Array> deviceArray = Array::New((int)num_devices);
    for (int i=0; i<(int)num_devices; i++) {
      cl_device_id d = devices[i];
      WebCLObject *obj=findCLObj((void*)d, CLObjType::Device);
      if(obj) 
        deviceArray->Set(i, NanObjectWrapHandle(obj));
      else
        deviceArray->Set(i, NanObjectWrapHandle(Device::New(d)));
    }
    delete[] devices;
    NanReturnValue(deviceArray);
  }
  case CL_PROGRAM_SOURCE: {
    size_t size=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_SOURCE, 0, NULL, &size);
    char *source=new char[size];
    ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_SOURCE, sizeof(char)*size, source, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    Local<String> str=String::New(source, (int) size-1);
    delete[] source;
    NanReturnValue(str);
  }
  case CL_PROGRAM_BINARY_SIZES: {
    size_t nsizes=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_BINARY_SIZES, 0, NULL, &nsizes);
    nsizes /= sizeof(size_t);
    size_t *sizes=new size_t[nsizes];
    ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_BINARY_SIZES, sizeof(size_t)*nsizes, sizes, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    Local<Array> sizesArray = Array::New((int)nsizes);
    for (int i=0; i<(int)nsizes; i++) {
      sizesArray->Set(i, JS_INT(sizes[i]));
    }
    delete[] sizes;
    NanReturnValue(sizesArray);
  }
  case CL_PROGRAM_BINARIES: { // TODO
    return NanThrowError("PROGRAM_BINARIES not implemented");

    size_t nbins=0;
    cl_int ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_BINARIES, 0, NULL, &nbins);
    nbins /= sizeof(size_t);
    char* *binaries=new char*[nbins];
    ret=::clGetProgramInfo(prog->getProgram(), CL_PROGRAM_BINARIES, sizeof(char*)*nbins, binaries, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }

    // TODO create an array for Uint8Array to return each binary associated to each device
    // Handle<Value> abv = Context::GetCurrent()->Global()->Get(String::NewSymbol("ArrayBuffer"));
    // Handle<Value> argv[] = { Integer::NewFromUnsigned(size) };
    // Handle<Object> arrbuf = Handle<Function>::Cast(abv)->NewInstance(1, argv);
    // void *buffer = arrbuf->GetPointerFromInternalField(0);
    // memcpy(buffer, data, size);

    delete[] binaries;
    NanReturnUndefined();
  }
  default: {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }
  }
}

NAN_METHOD(Program::getBuildInfo)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());
  if(args[0]->IsUndefined() || args[0]->IsNull()) {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }
  
  Device *dev = ObjectWrap::Unwrap<Device>(args[0]->ToObject());
  if(args[1]->IsUndefined() || args[1]->IsNull()) {
    cl_int ret=CL_INVALID_VALUE;
    REQ_ERROR_THROW(INVALID_VALUE);
    NanReturnUndefined();
  }

  cl_program_info param_name = args[1]->Uint32Value();

  switch (param_name) {
  case CL_PROGRAM_BUILD_STATUS: {
    cl_build_status param_value;
    cl_int ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, sizeof(cl_build_status), &param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    NanReturnValue(JS_INT(param_value));
  }
  default: {
    size_t param_value_size_ret=0;
    cl_int ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, 0,
        NULL, &param_value_size_ret);
    char *param_value=new char[param_value_size_ret];
    ret=::clGetProgramBuildInfo(prog->getProgram(), dev->getDevice(), param_name, param_value_size_ret,
        param_value, NULL);
    if (ret != CL_SUCCESS) {
      REQ_ERROR_THROW(INVALID_DEVICE);
      REQ_ERROR_THROW(INVALID_VALUE);
      REQ_ERROR_THROW(INVALID_PROGRAM);
      REQ_ERROR_THROW(OUT_OF_RESOURCES);
      REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
      return NanThrowError("UNKNOWN ERROR");
    }
    Local<Value> obj = JS_STR(param_value,(int)param_value_size_ret);
    delete[] param_value;
    NanReturnValue(obj);
  }
  }
}

class ProgramWorker : public NanAsyncWorker {
 public:
  ProgramWorker(Baton *baton)
    : NanAsyncWorker(baton->callback), baton_(baton) {
    }

  ~ProgramWorker() {
    if(baton_) {
      NanScope();

      if (!baton_->data.IsEmpty()) NanDisposePersistent(baton_->data);
      delete baton_;
    }
  }

  // Executed inside the worker-thread.
  // It is not safe to access V8, or V8 data structures
  // here, so everything we need for input and output
  // should go on `this`.
  void Execute () {
    // SetErrorMessage("Error");
    // printf("[build] execute\n");
  }

  // Executed when the async work is complete
  // this function will be run inside the main event loop
  // so it is safe to use V8 again
  void HandleOKCallback () {
    NanScope();

    if(baton_->data.IsEmpty()) {
#ifdef LOGGING
      printf("Calling callback with 1 arg\n");
#endif
      Local<Value> argv[] = { 
        JS_INT(baton_->error)
      };
      callback->Call(1, argv);
    }
    else {
#ifdef LOGGING
      printf("Calling callback with 2 args\n");
#endif
      Local<Value> argv[] = {
        JS_INT(baton_->error),
        NanPersistentToLocal(baton_->data)     // user's message
      };
      callback->Call(2, argv);
    }
  }

  private:
    Baton *baton_;
};

void Program::callback (cl_program program, void *user_data)
{
  //cout<<"[Program::driver_callback] thread "<<pthread_self()<<endl<<flush;
  Baton *baton = static_cast<Baton*>(user_data);
  //cout<<"  baton: "<<hex<<baton<<dec<<endl<<flush;
  baton->error=0;

  int num_devices=0;
  ::clGetProgramInfo(program, CL_PROGRAM_NUM_DEVICES, sizeof(int), &num_devices, NULL);
  if(num_devices>0) {
    cl_device_id *devices=new cl_device_id[num_devices];
    ::clGetProgramInfo(program, CL_PROGRAM_DEVICES, sizeof(cl_device_id)*num_devices, devices, NULL);
    for(int i=0;i<num_devices;i++) {
      int err=CL_SUCCESS;
      ::clGetProgramBuildInfo(program, devices[i], CL_PROGRAM_BUILD_STATUS, sizeof(int), &err, NULL);
      baton->error |= err;
    }
    delete[] devices;
  }

#ifdef LOGGING
  printf("[build] calling async JS cb\n");
#endif
  NanAsyncQueueWorker(new ProgramWorker(baton));
}

NAN_METHOD(Program::build)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());
  if(!prog->getProgram()) {
    cl_int ret=CL_INVALID_PROGRAM;
    REQ_ERROR_THROW(INVALID_PROGRAM);
  }

  cl_device_id *devices=NULL;
  int num=0;
  if(args[0]->IsArray()) {
    Local<Array> deviceArray = Local<Array>::Cast(args[0]);
    //cout<<"Building program for "<<deviceArray->Length()<<" devices"<<endl;
    num=deviceArray->Length();
    devices=new cl_device_id[num];
    for (int i=0; i<num; i++) {
      Local<Object> obj = deviceArray->Get(i)->ToObject();
      Device *d = ObjectWrap::Unwrap<Device>(obj);
      //cout<<"Device "<<i<<": "<<d->getDevice()<<endl;
      devices[i] = d->getDevice();
    }
  }
  else if(args[0]->IsObject()) {
    Local<Object> obj = args[0]->ToObject();
    Device *d = ObjectWrap::Unwrap<Device>(obj);
    num=1;
    devices=new cl_device_id;
    *devices= d->getDevice();
  }
  //cout<<"[Program::build] #devices: "<<num<<" ptr="<<hex<<devices<<dec<<endl<<flush;

  // check for same device
  {
    cl_uint ndevs=0;
    clGetProgramInfo(prog->getProgram(),CL_PROGRAM_NUM_DEVICES,sizeof(cl_uint),&ndevs,NULL);
    cl_device_id *d1=new cl_device_id[ndevs];
    clGetProgramInfo(prog->getProgram(),CL_PROGRAM_DEVICES,sizeof(cl_device_id)*ndevs,d1, NULL);

    int nok=0;
    for(int i=0;i<num;i++) {
      for(cl_uint j=0;j<ndevs;j++) {
        if(devices[i]==d1[j])
          nok++;
      }
    }
    if(d1) delete[] d1;
    if(nok != num) {
      if(devices) delete[] devices;
      cl_int ret=CL_INVALID_DEVICE;
      REQ_ERROR_THROW(INVALID_DEVICE);
      NanReturnUndefined();
    }
  }

  char *options=NULL;
  if(!args[1]->IsUndefined() && !args[1]->IsNull() && args[1]->IsString()) {
    String::AsciiValue str(args[1]);
    // cout<<"str length: "<<str.length()<<endl;

    if(str.length()>0) {
      options = ::strdup(*str);
      // printf("options: %s\n",options);

      // Mac driver bug: make sure -D is not alone...or crash!
      char *pch=strtok(options," ");
      while (pch != NULL) {
        if(pch && strncmp(pch,"-D",2)==0) {
          if(*(pch+3)==' ') { 
            pch = strtok (NULL, " ");
            if(pch==NULL || *pch=='-') {
              cl_int ret=CL_INVALID_BUILD_OPTIONS;
              REQ_ERROR_THROW(INVALID_BUILD_OPTIONS);
              NanReturnUndefined();
            }
          }
        }
        pch = strtok (NULL, " ");
      }
      /////

      free(options);
      options = ::strdup(*str);
    }
  }

  Baton *baton=NULL;
  if(args.Length()>=3 && !args[2]->IsUndefined() && args[2]->IsFunction()) {

    baton=new Baton();
    baton->callback=new NanCallback(args[2].As<Function>());
    if(!args[3]->IsNull() && !args[3]->IsUndefined()) {
#ifdef LOGGING
      printf("Adding user data to callback baton\n");
#endif
      NanAssignPersistent(v8::Value, baton->data, args[3]);
    }
  }

  // printf("Build program with baton %p\n",baton);

  cl_int ret = ::clBuildProgram(prog->getProgram(), num, devices,
      options,
      baton ? Program::callback : NULL,
      baton);

  if(options) free(options);
  if(devices) delete[] devices;

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PROGRAM);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(INVALID_DEVICE);
    REQ_ERROR_THROW(INVALID_BINARY);
    REQ_ERROR_THROW(INVALID_BUILD_OPTIONS);
    REQ_ERROR_THROW(INVALID_OPERATION);
    REQ_ERROR_THROW(COMPILER_NOT_AVAILABLE);
    REQ_ERROR_THROW(BUILD_PROGRAM_FAILURE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnUndefined();
}

NAN_METHOD(Program::createKernel)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());

  Local<String> str = args[0]->ToString();
  String::AsciiValue astr(str);

  cl_int ret = CL_SUCCESS;
  cl_kernel kw = ::clCreateKernel(prog->getProgram(), (const char*) *astr, &ret);
  // printf("createKernel %p ret %d\n",kw,ret);

  if (ret != CL_SUCCESS) {
    REQ_ERROR_THROW(INVALID_PROGRAM);
    REQ_ERROR_THROW(INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(INVALID_KERNEL_NAME);
    REQ_ERROR_THROW(INVALID_KERNEL_DEFINITION);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  NanReturnValue(NanObjectWrapHandle(Kernel::New(kw, prog)));
}

NAN_METHOD(Program::createKernelsInProgram)
{
  NanScope();
  Program *prog = ObjectWrap::Unwrap<Program>(args.This());

  Local<String> str = args[0]->ToString();
  String::AsciiValue astr(str);

  cl_uint num_kernels=0;
  cl_kernel *kernels=NULL;
  cl_int ret = ::clCreateKernelsInProgram(prog->getProgram(), 0, NULL, &num_kernels);

  if(ret == CL_SUCCESS && num_kernels>0) {
    kernels=new cl_kernel[num_kernels];
    ret = ::clCreateKernelsInProgram(prog->getProgram(), num_kernels, kernels, NULL);
  }

  if (ret != CL_SUCCESS) {
    delete[] kernels;
    REQ_ERROR_THROW(INVALID_PROGRAM);
    REQ_ERROR_THROW(INVALID_PROGRAM_EXECUTABLE);
    REQ_ERROR_THROW(INVALID_VALUE);
    REQ_ERROR_THROW(OUT_OF_RESOURCES);
    REQ_ERROR_THROW(OUT_OF_HOST_MEMORY);
    return NanThrowError("UNKNOWN ERROR");
  }

  // build list of WebCLKernels
  Local<Array> jsKernels=Array::New(num_kernels);

  for(cl_uint i=0;i<num_kernels;i++) {
    jsKernels->Set(i, NanObjectWrapHandle( Kernel::New( kernels[i], prog ) ) );
  }

  delete[] kernels;
  NanReturnValue(jsKernels);
}

NAN_METHOD(Program::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();

  Program *p = new Program(args.This());
  p->Wrap(args.This());
  NanReturnValue(args.This());
}

Program *Program::New(cl_program pw, WebCLObject *parent)
{
  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  Program *p = ObjectWrap::Unwrap<Program>(obj);
  p->program = pw;
  p->setParent(parent);
  registerCLObj(pw, p);
 
  return p;
}

} // namespace
