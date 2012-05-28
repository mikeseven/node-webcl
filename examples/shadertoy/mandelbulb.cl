// forward declarations
bool isphere( float4 sph, float3 ro, float3 rd, float2 *t );
bool iterate( float3 q, float *resPot, float4 *resColor );
bool ifractal( float3 ro, float3 rd, float *rest, float maxt, float3 *resnor, float4 *rescol, float fov );

inline bool isphere( float4 sph, float3 ro, float3 rd, float2 *t )
{
  float3 oc = ro - sph.xyz;
  float b = dot(oc,rd);
  float c = dot(oc,oc) - sph.w*sph.w;

  float h = b*b - c;
  if( h<0 )
  return false;

  float g = sqrt( h );
  (*t).x = - b - g;
  (*t).y = - b + g;

  return true;
}

#define NumIte 7
#define Bailout 100

inline bool iterate( float3 q, float *resPot, float4 *resColor )
{
  float4 trap = (float4)(100);
  float3 zz = q;
  float m = dot(zz,zz);
  if( m > Bailout )
  {
    *resPot = 0.5f*log(m); ///pow(8.0f,0.0f);
    *resColor = (float4)(1);
    return false;
  }

  for( int i=1; i<NumIte; i++ )
  {
    float x = zz.x; float x2 = x*x; float x4 = x2*x2;
    float y = zz.y; float y2 = y*y; float y4 = y2*y2;
    float z = zz.z; float z2 = z*z; float z4 = z2*z2;

    float k3 = x2 + z2;
    float k2 = rsqrt( k3*k3*k3*k3*k3*k3*k3 );
    float k1 = x4 + y4 + z4 - 6*y2*z2 - 6*x2*y2 + 2*z2*x2;
    float k4 = x2 - y2 + z2;

    zz.x = q.x + 64*x*y*z*(x2-z2)*k4*(x4-6.0*x2*z2+z4)*k1*k2;
    zz.y = q.y + -16*y2*k3*k4*k4 + k1*k1;
    zz.z = q.z + -8*y*k4*(x4*x4 - 28*x4*x2*z2 + 70*x4*z4 - 28*x2*z2*z4 + z4*z4)*k1*k2;

    m = dot(zz,zz);

    trap = min( trap, (float4)(zz.xyz*zz.xyz,m) );

    if( m > Bailout )
    {
      *resColor = trap;
      *resPot = 0.5f*log(m)/pow(8.0f,i);
      return false;
    }
  }

  *resColor = trap;
  *resPot = 0;
  return true;
}

inline bool ifractal( float3 ro, float3 rd, float *rest, float maxt,
    float3 *resnor, float4 *rescol, float fov )
{
  float4 sph = (float4)( 0.0, 0.0, 0.0, 1.25 );
  float2 dis;

  // bounding sphere
  if( !isphere(sph,ro,rd,&dis) )
  return false;

  // early skip
  if( dis.y<0.001f ) return false;

  // clip to near!
  if( dis.x<0.001f ) dis.x = 0.001f;

  if( dis.y>maxt) dis.y = maxt;

  float dt;
  float3 gra;
  float4 color;
  float4 col2;
  float pot1;
  float pot2, pot3, pot4;

  float fovfactor = 1.0f/sqrt(1+fov*fov);

  // raymarch!
  for( float t=dis.x; t<dis.y; )
  {
    float3 p = ro + rd*t;

    float Surface = clamp( 0.001f*t*fovfactor, 0.000001f, 0.005f );

    float eps = Surface*0.1f;

    if( iterate(p,&pot1,&color) ) {
      *rest = t;
      *resnor=normalize(gra);
      *rescol = color;
      return true;
    }

    iterate(p+(float3)(eps,0.0,0.0),&pot2,&col2);
    iterate(p+(float3)(0.0,eps,0.0),&pot3,&col2);
    iterate(p+(float3)(0.0,0.0,eps),&pot4,&col2);

    gra = (float3)( pot2-pot1, pot3-pot1, pot4-pot1 );
    dt = 0.5f*pot1*eps/length(gra);

    if( dt<Surface )
    {
      *rescol = color;
      *resnor = normalize( gra );
      *rest = t;
      return true;
    }

    t+=dt;
  }

  return false;
}

__kernel
void compute(__write_only image2d_t pix, float time)
{
  int x=get_global_id(0), y=get_global_id(1);
  const int width = get_global_size(0);
  const int height = get_global_size(1);
  float2 resolution=(float2)(width,height);
  float2 gl_FragCoord=(float2)(x,y);

  float2 p = (float2)(-1.f + 2.f * gl_FragCoord.xy / resolution.xy);
  float2 s = p*(float2)(1.33,1.0);

  float3 light1 = (float3)( 0.577f, 0.577f, 0.577f );
  float3 light2 = (float3)( -0.707f, 0, 0.707f );

  float fov = 1;
  float r = 1.4f+0.2f*cospi(2.f*time/20.f);
  float3 campos = (float3)( r*sinpi(2.f*time/20.f),
                            0.3f-0.4f*sinpi(2.f*time/20.f),
                            r*cospi(2.f*time/20.f) );
  float3 camtar = (float3)(0,0.1,0);

  //camera matrix
  float3 cw = normalize(camtar-campos);
  float3 cp = (float3)(0,1,0);
  float3 cu = normalize(cross(cw,cp));
  float3 cv = normalize(cross(cu,cw));

  // ray dir
  float3 rd;
  float3 nor, rgb;
  float4 col;
  float t;

  rd = normalize( s.x*cu + s.y*cv + 1.5f*cw );

  bool res=ifractal(campos,rd,&t,1e20f,&nor,&col,fov);

  if( !res )
  {
    rgb = 1.3f*(float3)(1,.98,0.9)*(0.7f+0.3f*rd.y);
  }
  else
  {
    float3 xyz = campos + t*rd;

    // sun light
    float dif1 = clamp( 0.2f + 0.8f*dot( light1, nor ), 0.f, 1.f );
    dif1=dif1*dif1;

    // back light
    float dif2 = clamp( 0.3f + 0.7f*dot( light2, nor ), 0.f, 1.f );

    // ambient occlusion
    float ao = clamp(1.25f*col.w-.4f,0.f,1.f);
    ao=0.5f*ao*(ao+1);

    // shadow
    if( dif1>0.001f ) {
      float lt1;
      float3 ln;
      float4 lc;
      if( ifractal(xyz,light1,&lt1,1e20,&ln,&lc,fov) )
        dif1 = 0.1f;
    }

    // material color
    rgb = (float3)(1);
    rgb = mix( rgb, (float3)(0.8,0.6,0.2), (float3)(sqrt(col.x)*1.25f) );
    rgb = mix( rgb, (float3)(0.8,0.3,0.3), (float3)(sqrt(col.y)*1.25f) );
    rgb = mix( rgb, (float3)(0.7,0.4,0.3), (float3)(sqrt(col.z)*1.25f) );

    // lighting
    rgb *= (0.5f+0.5f*nor.y)*
           (float3)(.14,.15,.16)*0.8f +
           dif1*(float3)(1.0,.85,.4) +
           0.5f*dif2*(float3)(.08,.10,.14);
    rgb *= (float3)( pow(ao,0.8f), pow(ao,1.00f), pow(ao,1.1f) );

    // gamma
    rgb = 1.5f*(rgb*0.15f + 0.85f*sqrt(rgb));
  }

  float2 uv = 0.5f*(p+1.f);
  rgb *= 0.7f + 0.3f*16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y);
  rgb = clamp( rgb, (float3)(0), (float3)(1) );

  write_imagef(pix,(int2)(x,y),(float4)(rgb,1.0f));
}
