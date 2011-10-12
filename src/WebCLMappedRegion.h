/*
** This file contains proprietary software owned by Motorola Mobility, Inc. **
** No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder. **
**
** (c) Copyright 2011 Motorola Mobility, Inc.  All Rights Reserved.  **
*/

#ifndef MAPPEDREGION_H_
#define MAPPEDREGION_H_

#include "common.h"
#include <node_buffer.h>

namespace webcl {

typedef struct _MappedRegion {
  node::Buffer *buffer;
  ulong mapped_ptr;
} _MappedRegion;

class WebCLMappedRegion : public node::ObjectWrap
{

public:
  ~WebCLMappedRegion();

  static void Init(v8::Handle<v8::Object> target);

  static WebCLMappedRegion *New(_MappedRegion* region);
  static JS_METHOD(New);

  static JS_METHOD(getInfo);

  _MappedRegion *getMappedRegion() { return region; }

private:
  WebCLMappedRegion(v8::Handle<v8::Object> wrapper);

  static v8::Persistent<v8::FunctionTemplate> constructor_template;

  _MappedRegion *region;
};

} // namespace

#endif /* MAPPEDREGION_H_ */
