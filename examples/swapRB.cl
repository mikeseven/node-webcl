__kernel void swapRB(__global const uchar4* src,
                     __global uchar4* dst,
                     uint width,
                     uint height)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  int i = y * width + x;
  if(i>=width*height) return;

  uchar4 color=src[i];

  // monochrome
  //uchar lum = (uchar)(0.30f * color.x + 0.59f * color.y + 0.11f * color.z);
  //dst[i]=(uchar4)(lum,lum,lum,0xFF);

  // swap red and blue
  dst[i]=(uchar4)(color.z,color.y,color.x,0xFF);
}
