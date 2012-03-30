__kernel void swapRB(__global const uchar* src,
                     __global uchar* dst,
                     unsigned int width,
                     unsigned int height)
{
  int x = get_global_id(0);
  int i = x*4;
  if(i>=width*height*4) return;

  dst[i]=src[i+2];
  dst[i+1]=src[i+1];
  dst[i+2]=src[i];
  dst[i+3]=src[i+3];
}
