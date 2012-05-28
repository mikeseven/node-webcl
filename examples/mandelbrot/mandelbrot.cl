#ifdef cl_amd_fp64
  #pragma OPENCL EXTENSION cl_amd_fp64 : enable
#elif defined(cl_khr_fp64)
  #pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

__constant uint nc = 30;
#define nc2 (2*nc)
#define maxCol (nc*3)
#define st (255.0f/(float)nc)

/* This could be implemented as another kernel that would be executed after
the Mandelbrot rendering. By doing so we could use many color maps without
redoing the calculation of the Mandelbrot image.
In this case, we would have Mandelbrot 'image' are a uint16 to be mapped into
 a final RGBA image using a colormap.
*/
void colorMap(__global unsigned int* pix, unsigned int idx,
unsigned int ic)
{
  unsigned int r=0U, g=0U, b=0U;
  if(ic<nc) {
    unsigned int d = (unsigned int) (st*ic);
    r=255-d;
    g=d;
  }
  else if(ic<nc2) {
    unsigned int d = (unsigned int) (st*(ic-nc));
    g=255-d;
    b=d;
  }
  else if(ic<maxCol) {
    unsigned int d = (unsigned int) (st*(ic-nc2));
    r=d;
    b=255-d;
  }

  unsigned int uiPackedPix=0;
  uiPackedPix |= 0x000000FF & r;
  uiPackedPix |= 0x0000FF00 & (g << 8);
  uiPackedPix |= 0x00FF0000 & (b << 16);
  uiPackedPix |= 0xFF000000 ;
  pix[idx]= uiPackedPix;
}

__kernel void computeSet(
  __global unsigned int* pix,
  float cX, float cY, float scale, unsigned int nmax,
  unsigned int width, unsigned int height)
{
  int x = get_global_id(0);
  int y = get_global_id(1);

  unsigned int w2 = width >> 1, h2 = height >> 1;
  int x2 = x-w2, y2 = y-h2;
  float Cr = (x2/scale)+ cX;
  float Ci = (y2/scale)+ cY;
  float I = 0, R = 0, I2 = 0, R2 = 0;
  unsigned int n = 0U;

  while ( ((R2+I2)<4.0f) && (n<nmax)) {
    I= (R+R)*I +Ci;
    R= R2-I2+Cr;
    R2= R*R;
    I2= I*I;
    n++;
  };

  colorMap(pix, x+y*width, (n==nmax) ? maxCol : (n % maxCol));
}
