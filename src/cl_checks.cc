#include "cl_checks.h"

namespace webcl {

/*
 * Various checking methods to validate inputs before they hit CL drivers
 * and potentially crash machines badly due to poor drivers out there.
 */

/* number of bytes in the specified rectangle buffer.
 * @return -1 if error
 */
int bufferRectSize(const size_t offset[3], const size_t region[3], size_t row_pitch, size_t slice_pitch, size_t buffer_len) 
{
  size_t x= offset[0], y=offset[1], z=offset[2];
  size_t w=region[0], h=region[1], d=region[2];
  if(w==0 || h==0 || d==0)
    return -1;

  if(row_pitch==0) 
    row_pitch=w;
  else if (row_pitch<w || row_pitch>=buffer_len)
    return -1;

  if(slice_pitch==0) 
    slice_pitch=row_pitch*h;
  else if (slice_pitch<(row_pitch * h) || slice_pitch>=buffer_len)
    return -1;

  if((slice_pitch % row_pitch) !=0)
    return -1;

  return (int)(z * slice_pitch + y * row_pitch + x + (w * h * d));
}

/*
 * @return number of bytes in an image region
 * @return -1 if error
 */
int imageRectSize(const size_t origin[3], const size_t region[3], size_t row_pitch, size_t slice_pitch, cl_mem img, int buffer_len)
{
  size_t w = region[0], h=region[1], d=region[2];

  if ( w==0 || h==0 || d==0 )
    return -1;

  size_t imgW, imgH, bpp;
  cl_int ret = clGetImageInfo(img,CL_IMAGE_WIDTH,sizeof(size_t),&imgW,NULL);
  ret |= clGetImageInfo(img,CL_IMAGE_HEIGHT,sizeof(size_t),&imgH,NULL);
  ret |= clGetImageInfo(img,CL_IMAGE_ELEMENT_SIZE,sizeof(size_t),&bpp,NULL);
  if(ret!=CL_SUCCESS)
    return -1;

  if(buffer_len>=0 && buffer_len < (int)(region[0]*region[1]*region[2]*bpp))
    return -1;
  
  if(origin[0]+region[0]>imgW || origin[1]+region[1]>imgH)
    return -1;

  if ( row_pitch == 0 )
    row_pitch = w;
  else if ( row_pitch < w || row_pitch<(imgW*bpp) || (row_pitch % (imgW*bpp)) !=0)
    return -1;

  if ( slice_pitch == 0 )
    slice_pitch = row_pitch * h;
  else if ( slice_pitch < (row_pitch * h))
    return -1;

  if(origin[0]+region[0]>imgW || origin[1]+region[1]>imgH)
    return -1;

  return (int)(slice_pitch * d);
}

/*
 * @return number of bytes per elements in a TypedArray
 * @return -1 if wrong type
 */
int getTypedArrayBytes(ExternalArrayType type) 
{
  switch(type) {
    case kExternalByteArray:
    case kExternalUnsignedByteArray:
    case kExternalPixelArray:
      return 1;
    case kExternalShortArray:
    case kExternalUnsignedShortArray:
      return 2;
    case kExternalIntArray:
    case kExternalUnsignedIntArray:
    case kExternalFloatArray:
      return 4;
    case kExternalDoubleArray:
      return 8;
  }
  return -1;
}

void getPtrAndLen(const Local<Value> value, void* &ptr, int &len) 
{
	ptr=NULL;
	len=0;
  if(!value->IsUndefined() && !value->IsNull()) {
    if(value->IsArray()) {
      Local<Array> arr=Local<Array>::Cast(value);
      ptr = arr->GetIndexedPropertiesExternalArrayData();
      len = arr->GetIndexedPropertiesExternalArrayDataLength() * getTypedArrayBytes(arr->GetIndexedPropertiesExternalArrayDataType());
    }
    else if(value->IsObject()) {
      Local<Object> obj=value->ToObject();
      String::AsciiValue name(obj->GetConstructorName());
      if(!strcmp("Buffer",*name)) {
        ptr=node::Buffer::Data(obj);
        len=node::Buffer::Length(obj);
      }
      else {
        ptr = obj->GetIndexedPropertiesExternalArrayData();
        len = obj->GetIndexedPropertiesExternalArrayDataLength() * getTypedArrayBytes(obj->GetIndexedPropertiesExternalArrayDataType());
      }
    }
  }
}

/** 
 * @return number of channels in the specified cl_channel_order 
 * @return -1 if error
 */
int getChannelCount(const int channelOrder)
{
  switch ( channelOrder ) {
    case CL_R:
    case CL_A:
    case CL_INTENSITY:
    case CL_LUMINANCE:
    case CL_Rx:
    case CL_DEPTH:
    case CL_DEPTH_STENCIL:
      return 1;
    case CL_RG:
    case CL_RA:
    case CL_RGx:
      return 2;
    case CL_RGB:
    case CL_RGBx:
      return 3;
    case CL_RGBA:
    case CL_BGRA:
    case CL_ARGB:
      return 4;
  }
  return -1;
}

/**
 * @return the number of bytes in the specified cl_channel_type.
 * @return -1 if unknown
 */
int getChannelSize(int channelType) {
  switch (channelType) {
    case CL_SNORM_INT8:
    case CL_UNORM_INT8:
    case CL_SIGNED_INT8:
    case CL_UNSIGNED_INT8:
      return 1;
    case CL_SNORM_INT16:
    case CL_UNORM_INT16:
    case CL_UNORM_SHORT_565:
    case CL_UNORM_SHORT_555:
    case CL_SIGNED_INT16:
    case CL_UNSIGNED_INT16:
    case CL_HALF_FLOAT:
      return 2;
    case CL_UNORM_INT_101010:
    case CL_SIGNED_INT32:
    case CL_UNSIGNED_INT32:
    case CL_FLOAT:
      return 4;
  }
  return -1;
}

} // namespace webcl
