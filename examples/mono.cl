/**
 * Color to grayscale conversion in OpenCL C.
 *
 * @param {uchar3*} src source image (RGB)
 * @param {uchar3*} dst destination image (RGB)
 * @param {uint} width source and destination image width in pixels
 * @param {uint} height source and destination image height in pixels
 *
 * @author Tomi Aarnio, Nokia Research Tampere, 2011
 */

__kernel void mono(__global const uchar3* src,
                           __global uchar3* dst,
                           uint width, 
                           uint height)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  if (x >= width || y >= height) return;

  int i = y * width + x;

  uchar3 color = src[i];
  uchar lum = (uchar)(0.30f * color.x + 0.59f * color.y + 0.11f * color.z);
  dst[i] = (uchar3)(lum, lum, lum);
}
        