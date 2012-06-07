// from http://www.iquilezles.org/apps/shadertoy/?p=704

typedef struct {
  float time;
  float stime;
  float ctime;
} Params;

inline float inObj(Params params, float3 p){
  float oP=length(p);
  p.x=native_sin(p.x)+params.stime;
  p.z=native_sin(p.z)+params.ctime;
  return (min(length(p)-1.5f-native_sin(oP-params.time*4.0f), p.y+3.0f));
}

inline float _fract(float v) {
  return v-floor(v);
}

__kernel
void compute(
    write_only image2d_t pix,
    const float time)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);
  const int width = get_global_size(0)-1;
  const int height = get_global_size(1)-1;

  const float2 pos=(float2)(x,y);
  const float2 resolution=(float2)(width,height);

  const float2 vPos= -1.0f+2.0f*pos/resolution;

  __local Params params;
  params.time=(float)(time);
  params.stime=native_sin(time);
  params.ctime=native_cos(time);

  //Camera animation
  const float3 vuv=(float3)(params.stime, 1, 0);//view up vector
  const float3 vrp=(float3)(native_sin(params.time*0.7f)*10.0f, 0, native_cos(params.time*0.9f)*10.0f); //view reference point
  const float3 prp=(float3)(native_sin(params.time*0.7f)*20.0f+vrp.x+20.0f,
                      params.stime*4.0f+4.0f+vrp.y+3.0f,
                      native_cos(params.time*0.6f)*20.0f+vrp.z+14.0f); //camera position

  //Camera setup
  const float3 vpn=normalize(vrp-prp);
  const float3 u=normalize(cross(vuv,vpn));
  const float3 v=cross(vpn,u);
  const float3 vcv=(prp+vpn);
  const float3 scrCoord=vcv+vPos.x*u*resolution.x/resolution.y+vPos.y*v;
  const float3 scp=normalize(scrCoord-prp);

  //Raymarching
  const float3 e=(float3)(0.1f,0,0);
  const float maxd=200.0f;

  float s=0.1f;
  float3 c,p,n;

  //speed optimization -advance ray (simple raytracing) until plane y=2.5
  float f=-(prp.y-2.5f)/scp.y;
  if (f>0.0f) p=prp+scp*f;
  else f=maxd;

  for(int i=0;i<256;i++){
    if (fabs(s)<.01f || f>maxd) break;
    f+=s;
    p=prp+scp*f;
    s=inObj(params,p);
  }

  float3 rgb;

  if (f<maxd){
    if(p.y<-2.5f){
      if (_fract(p.x*.5f)>.5f)
        if (_fract(p.z*.5f)>.5f)
          c=(float3)(0);
        else
          c=(float3)(1);
      else
        if (_fract(p.z*.5f)>.5f)
          c = (float3)(1);
        else
          c = (float3)(0);
      n=(float3)(0,1,0);
    }
    else{
      const float d=length(p);
      c=(float3)((native_sin(d*.25f-params.time*4.0f)+1.0f),
                 (params.stime+1.0f),
                 (native_sin(d-params.time*4.0f)+1.0f)); //color
      c *= 0.5f;
      n=normalize(
        (float3)(s-inObj(params,p-e.xyy),
                 s-inObj(params,p-e.yxy),
                 s-inObj(params,p-e.yyx)));
    }
    const float b=dot(n,normalize(prp-p));
    rgb=(b*c+native_powr(b,54.0f))*(1.0f-f*.005f);
    rgb=clamp(rgb,(float3)(0),(float3)(1));
  }

  write_imagef(pix,(int2)(x,y),(float4)(rgb,1.0f));
}
