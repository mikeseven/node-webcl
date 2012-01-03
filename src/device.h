/*
 * device.h
 *
 *  Created on: Dec 19, 2011
 *      Author: ngk437
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include "common.h"

namespace webcl {

class Device : public node::ObjectWrap, public WebCLObject
{

public:
  static void Init(v8::Handle<v8::Object> target);

  static Device *New(cl_device_id did);
  static JS_METHOD(New);

  static JS_METHOD(getDeviceInfo);

  cl_device_id getDevice() const { return device_id; };

private:
  Device(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  cl_device_id device_id;
};

} // namespace


#endif /* DEVICE_H_ */
