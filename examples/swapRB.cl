__kernel void swapRB(__global const uchar* src,
                     __global uchar* dst,
                     uint width,
                     uint height)
{
  int x = get_global_id(0);
  int i = x*4;
  if(i>=width*height*4) return;

  //uchar4 color=src[i];

  // monochrome
  //uchar lum = (uchar)(0.30f * color.x + 0.59f * color.y + 0.11f * color.z);
  //dst[i]=(uchar4)(lum,lum,lum,0xFF);

  // swap red and blue
  //dst[i]=(uchar4)(src[i].x, src[i].y, src[i].z,0xFF);

  dst[i]=src[i+2];
  dst[i+1]=src[i+1];
  dst[i+2]=src[i];
  dst[i+3]=src[i+3];
}
