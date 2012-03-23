/*
 * webcl.h
 *
 *  Created on: Dec 18, 2011
 *      Author: ngk437
 */

#ifndef WEBCL_H_
#define WEBCL_H_

#include "common.h"

using namespace v8;

namespace webcl {

JS_METHOD(getPlatforms);
JS_METHOD(createContext);
JS_METHOD(waitForEvents);
JS_METHOD(unloadCompiler);

static void createContext_callback (const char *errinfo, const void *private_info, size_t cb, void *user_data);
static void createContext_After_cb(uv_async_t* handle, int status);

}

#endif /* WEBCL_H_ */
