#include "exceptions.h"

using namespace v8;

namespace webcl {

const char* ErrorDesc(cl_int err) 
{
  switch (err) {
    case CL_SUCCESS:                            return "Success!";
    case CL_DEVICE_NOT_FOUND:                   return "Device not found.";
    case CL_DEVICE_NOT_AVAILABLE:               return "Device not available";
    case CL_COMPILER_NOT_AVAILABLE:             return "Compiler not available";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "Memory object allocation failure";
    case CL_OUT_OF_RESOURCES:                   return "Out of resources";
    case CL_OUT_OF_HOST_MEMORY:                 return "Out of host memory";
    case CL_PROFILING_INFO_NOT_AVAILABLE:       return "Profiling information not available";
    case CL_MEM_COPY_OVERLAP:                   return "Memory copy overlap";
    case CL_IMAGE_FORMAT_MISMATCH:              return "Image format mismatch";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "Image format not supported";
    case CL_BUILD_PROGRAM_FAILURE:              return "Program build failure";
    case CL_MAP_FAILURE:                        return "Map failure";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "Misaligned sub-buffer offset";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST: return "Execution status error for events in wait list";
    case CL_COMPILE_PROGRAM_FAILURE:            return "Compile program failure";
    case CL_LINKER_NOT_AVAILABLE:               return "Linker not available";
    case CL_LINK_PROGRAM_FAILURE:               return "Link program failure";
    case CL_DEVICE_PARTITION_FAILED:            return "Device partition failed";
    case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:      return "Kernel argument info not available";

    case CL_INVALID_VALUE:                      return "Invalid value";
    case CL_INVALID_DEVICE_TYPE:                return "Invalid device type";
    case CL_INVALID_PLATFORM:                   return "Invalid platform";
    case CL_INVALID_DEVICE:                     return "Invalid device";
    case CL_INVALID_CONTEXT:                    return "Invalid context";
    case CL_INVALID_QUEUE_PROPERTIES:           return "Invalid queue properties";
    case CL_INVALID_COMMAND_QUEUE:              return "Invalid command queue";
    case CL_INVALID_HOST_PTR:                   return "Invalid host pointer";
    case CL_INVALID_MEM_OBJECT:                 return "Invalid memory object";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "Invalid image format descriptor";
    case CL_INVALID_IMAGE_SIZE:                 return "Invalid image size";
    case CL_INVALID_SAMPLER:                    return "Invalid sampler";
    case CL_INVALID_BINARY:                     return "Invalid binary";
    case CL_INVALID_BUILD_OPTIONS:              return "Invalid build options";
    case CL_INVALID_PROGRAM:                    return "Invalid program";
    case CL_INVALID_PROGRAM_EXECUTABLE:         return "Invalid program executable";
    case CL_INVALID_KERNEL_NAME:                return "Invalid kernel name";
    case CL_INVALID_KERNEL_DEFINITION:          return "Invalid kernel definition";
    case CL_INVALID_KERNEL:                     return "Invalid kernel";
    case CL_INVALID_ARG_INDEX:                  return "Invalid argument index";
    case CL_INVALID_ARG_VALUE:                  return "Invalid argument value";
    case CL_INVALID_ARG_SIZE:                   return "Invalid argument size";
    case CL_INVALID_KERNEL_ARGS:                return "Invalid kernel arguments";
    case CL_INVALID_WORK_DIMENSION:             return "Invalid work dimension";
    case CL_INVALID_WORK_GROUP_SIZE:            return "Invalid work group size";
    case CL_INVALID_WORK_ITEM_SIZE:             return "Invalid work item size";
    case CL_INVALID_GLOBAL_OFFSET:              return "Invalid global offset";
    case CL_INVALID_EVENT_WAIT_LIST:            return "Invalid event wait list";
    case CL_INVALID_EVENT:                      return "Invalid event";
    case CL_INVALID_OPERATION:                  return "Invalid operation";
    case CL_INVALID_GL_OBJECT:                  return "Invalid OpenGL object";
    case CL_INVALID_BUFFER_SIZE:                return "Invalid buffer size";
    case CL_INVALID_MIP_LEVEL:                  return "Invalid mip-map level";
    case CL_INVALID_GLOBAL_WORK_SIZE:           return "Invalid global work size";
    case CL_INVALID_PROPERTY:                   return "Invalid property";
    case CL_INVALID_IMAGE_DESCRIPTOR:           return "Invalid image descriptor";
    case CL_INVALID_COMPILER_OPTIONS:           return "Invalid compiler options";
    case CL_INVALID_LINKER_OPTIONS:             return "Invalid linker options";
    case CL_INVALID_DEVICE_PARTITION_COUNT:     return "Invalid device partition count";
    case WEBCL_EXTENSION_NOT_ENABLED:           return "KHR_gl_sharing extension not enabled";
  }
  return "Unknown";
}

Persistent<FunctionTemplate> WebCLException::constructor_template;

void WebCLException::Init(Handle<Object> target)
{
  NanScope();

  // constructor
  Local<FunctionTemplate> ctor = FunctionTemplate::New(WebCLException::New);
  NanAssignPersistent(FunctionTemplate, constructor_template, ctor);
  ctor->InstanceTemplate()->SetInternalFieldCount(1);
  ctor->SetClassName(NanSymbol("WebCLException"));

  // prototype
  Local<ObjectTemplate> proto = ctor->PrototypeTemplate();
  proto->SetAccessor(JS_STR("name"), GetName, NULL);
  proto->SetAccessor(JS_STR("description"), GetDescription, NULL);
  proto->SetAccessor(JS_STR("code"), GetCode, NULL);

  target->Set(NanSymbol("WebCLException"), ctor->GetFunction());
}

WebCLException::WebCLException(Handle<Object> wrapper) : name_(NULL), desc_(NULL),code_(0)
{
  _type=CLObjType::Exception;
}

NAN_GETTER(WebCLException::GetName) {
  NanScope();
  WebCLException *ex = ObjectWrap::Unwrap<WebCLException>(args.This());
  if(ex->name_)
    NanReturnValue(JS_STR(ex->name_));
  NanReturnNull();
}

NAN_GETTER(WebCLException::GetDescription) {
  NanScope();
  WebCLException *ex = ObjectWrap::Unwrap<WebCLException>(args.This());
  if(ex->desc_)
    NanReturnValue(JS_STR(ex->desc_));
  NanReturnNull();
}

NAN_GETTER(WebCLException::GetCode) {
  NanScope();
  WebCLException *ex = ObjectWrap::Unwrap<WebCLException>(args.This());
  NanReturnValue(JS_INT(ex->code_));
}

NAN_METHOD(WebCLException::New)
{
  if (!args.IsConstructCall())
    return NanThrowTypeError("Constructor cannot be called as a function.");

  NanScope();
  WebCLException *ex = new WebCLException(args.This());
  ex->Wrap(args.This());

  NanReturnValue(args.This());
}

WebCLException *WebCLException::New(const char *name, const char *desc, const int code)
{

  NanScope();

  Local<Value> arg = Integer::NewFromUnsigned(0);
  Local<FunctionTemplate> constructorHandle = NanPersistentToLocal(constructor_template);
  Local<Object> obj = constructorHandle->GetFunction()->NewInstance(1, &arg);

  WebCLException *ex = ObjectWrap::Unwrap<WebCLException>(obj);
  ex->name_=name;
  ex->desc_=desc;
  ex->code_=code;

  return ex;
}

} // namespace webcl