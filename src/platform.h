/*
 * platform.h
 *
 *  Created on: Dec 18, 2011
 *      Author: ngk437
 */

#ifndef PLATFORM_H_
#define PLATFORM_H_

#include "common.h"

namespace webcl {

class Platform : public WebCLObject
{

public:
  static void Init(v8::Handle<v8::Object> target);

  static Platform *New(cl_platform_id pid);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);
  static JS_METHOD(getDevices);
  static JS_METHOD(getExtension);

  cl_platform_id getPlatformId() const { return platform_id; };

private:
  Platform(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_platform_id platform_id;
};

}

#endif /* PLATFORM_H_ */
