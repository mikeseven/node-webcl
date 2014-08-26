#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include "common.h"

namespace webcl {

class WebCLException : public WebCLObject
{

public:
  static void Init(v8::Handle<v8::Object> exports);

  static WebCLException *New(const char *name, const char *desc, const int code);

  static NAN_METHOD(New);
  static NAN_GETTER(GetName);
  static NAN_GETTER(GetDescription);
  static NAN_GETTER(GetCode);
  
protected:
  WebCLException(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  const char *name_;
  const char *desc_;
  int code_;

private:
  DISABLE_COPY(WebCLException)
};

} // namespace

#endif // EXCEPTIONS_H_
