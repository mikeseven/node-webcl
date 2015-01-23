#ifndef CL_CHECKS_
#define CL_CHECKS_

#include "common.h"
#include <node.h>
#include <nan.h>

using namespace v8;

namespace webcl {

int bufferRectSize(const size_t offset[3], const size_t region[3], size_t row_pitch, size_t slice_pitch, size_t buffer_len);
int imageRectSize(const size_t origin[3], const size_t region[3], size_t row_pitch, size_t slice_pitch, cl_mem img, int buffer_len=-1);
void getPtrAndLen(const Local<Value> value, void* &ptr, int &len);
int getChannelCount(const int channelOrder);
int getChannelSize(int channelType);
int getTypedArrayBytes(ExternalArrayType type);
inline bool validateMemFlags(int value) {
  return (value>=CL_MEM_READ_WRITE && value<=CL_MEM_HOST_NO_ACCESS && value!=(1<<6));
}

} // namespace webcl

#endif // CL_CHECKS_