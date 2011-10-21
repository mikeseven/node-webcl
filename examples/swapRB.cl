/**
 * swap r and b values
**/
__kernel void swapRB(__global const uchar3* src, __global uchar3* dst,
                     uint width, uint height)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  int i = y * width + x;

  uchar3 color = src[i];
  dst[i] = color;
}
