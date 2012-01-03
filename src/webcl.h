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
}

#endif /* WEBCL_H_ */
