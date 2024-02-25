#version 450
// Rainbow Cavern -  dr2 - 2017-06-01
// https://www.shadertoy.com/view/XsfBWM

// Underground boat ride (mouseable)

// "Rainbow Cavern" by dr2 - 2017
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
const vec2 madd = vec2(0.5, 0.5);
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = gl_Position.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
float iGlobalTime = float(global.FrameCount)*0.025;
vec2 iResolution = global.OutputSize.xy;

#define USE_BMAP true
//#define USE_BMAP false  // for weaker GPUs

float PrSphDf (vec3 p, float r);
float PrCapsDf (vec3 p, float r, float h);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);
float Hashfv3 (vec3 p);
vec3 Hashv3f (float p);
vec3 VaryNf (vec3 p, vec3 n, float f);

vec4 vcId;
vec3 ltPos[2], boatPos[2];
float boatAng[2], dstFar, tCur, htWat, dstBMap;
int idObj;
bool uWat, hitWat;
const int idBoat = 1, idBLamp = 2, idFLamp = 3;
const float pi = 3.14159;

float ObjDf (vec3 p)
{
  vec3 q;
  float dMin, d;
  dMin = dstFar;
  for (int k = 0; k < 2; k ++) {
    q = p - boatPos[k];
    q.xz = Rot2D (q.xz, boatAng[k]);
    d = max (PrCapsDf (q, 0.11, 0.25),
       - PrCapsDf (q + vec3 (0., -0.02, 0.), 0.1, 0.24));
    if (d < dMin) { dMin = d;  idObj = idBoat; }
    q.y -= 0.1;
    q.z -= 0.3;
    d = PrSphDf (q, 0.01);
    if (d < dMin) { dMin = d;  idObj = idFLamp; }
    q.z -= -0.6;
    d = PrSphDf (q, 0.01);
    if (d < dMin) { dMin = d;  idObj = idBLamp; }
  }
  return dMin;
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d;
  dHit = 0.;
  for (int j = 0; j < 100; j ++) {
    d = ObjDf (ro + dHit * rd);
    dHit += d;
    if (d < 0.001 || dHit > dstFar) break;
  }
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  vec4 v;
  vec3 e = vec3 (0.001, -0.001, 0.);
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy),
     ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

float VPoly (vec3 p)
{
  vec3 ip, fp, g, w, a;
  ip = floor (p);
  fp = fract (p);
  a = vec3 (2.);
  for (float gz = -1.; gz <= 1.; gz ++) {
    for (float gy = -1.; gy <= 1.; gy ++) {
      for (float gx = -1.; gx <= 1.; gx ++) {
        g = vec3 (gx, gy, gz);
        w = g + 0.7 * Hashfv3 (ip + g) - fp;
        a.x = dot (w, w);
        if (a.x < a.y) {
          vcId = vec4 (ip + g, a.y - a.x);
          a = a.zxy;
        } else a.z = min (a.z, a.x);
      }
    }
  }
  return a.y;
}

vec3 TrackPath (float t)
{
  return vec3 (4.7 * sin (t * 0.15) + 2.7 * cos (t * 0.19), 0., t);
}

float CaveDf (vec3 p)
{
  vec3 hv;
  float s, d;
  s = p.y - htWat;
  p.xy -= TrackPath (p.z).xy;
  p += 0.1 * (1. - cos (2. * pi * (p + 0.2 * (1. - cos (2. * pi * p.zxy)))));
  hv = cos (0.6 * p - 0.5 * sin (1.4 * p.zxy + 0.4 * cos (2.7 * p.yzx)));
  if (USE_BMAP && dstBMap < 10.) hv *= 1. + 0.01 *
     (1. - smoothstep (0., 10., dstBMap)) *
     smoothstep (0.05, 0.4, VPoly (10. * p)) / length (hv);
  d = 0.9 * (length (hv) - 1.1);
  if (! uWat) d = min (d, s);
  return d;
}

float CaveRay (vec3 ro, vec3 rd)
{
  float d, dHit;
  dHit = 0.;
  for (int j = 0; j < 200; j ++) {
    dstBMap = dHit;
    d = CaveDf (ro + dHit * rd);
    dHit += d;
    if (d < 0.001 || dHit > dstFar) break;
  }
  return dHit;
}

vec3 CaveNf (vec3 p)
{
  vec4 v;
  const vec3 e = vec3 (0.001, -0.001, 0.);
  v = vec4 (CaveDf (p + e.xxx), CaveDf (p + e.xyy),
     CaveDf (p + e.yxy), CaveDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

float CaveSShadow (vec3 ro, vec3 rd)
{
  float sh, d, h;
  sh = 1.;
  d = 0.1;
  for (int j = 0; j < 16; j ++) {
    h = CaveDf (ro + rd * d);
    sh = min (sh, smoothstep (0., 0.05 * d, h));
    d += max (0.2, 0.1 * d);
    if (sh < 0.05) break;
  }
  return 0.4 + 0.6 * sh;
}

vec3 CaveCol (vec3 ro, vec3 rd, vec3 ltDir, float atten)
{
  vec3 col, vn, q, vno;
  float glit;
  VPoly (10. * ro);
  q = ro;
  if (! USE_BMAP) q = 0.004 * floor (250. * q);
  vn = VaryNf (10. * q, CaveNf (q), 1.);
  col = (vec3 (0.3, 0.1, 0.) + vec3 (0.3, 0.2, 0.1) * Hashv3f (Hashfv3 (vcId.xyz))) *
     (1.2 - 0.4 * Hashfv3 (100. * ro)) *
     (0.4 + 0.6 * smoothstep (0.05, 1., sqrt (vcId.w))) *
     (0.2 + 0.8 * max (dot (vn, ltDir), 0.) +
     2. * pow (max (dot (normalize (ltDir - rd), vn), 0.), 256.));
  if (! hitWat) {
    vno = CaveNf (ro);
    glit = 20. * pow (max (0., dot (ltDir, reflect (rd, vno))), 4.) *
       pow (1. - 0.6 * abs (dot (normalize (ltDir - rd),
       VaryNf (100. * ro, vno, 5.))), 8.);
    col += vec3 (1., 1., 0.5) * glit;
  }
  col *= atten * CaveSShadow (ro, ltDir);
  return col;
}

vec3 ObjCol (vec3 ro, vec3 rd, vec3 vn, vec3 ltDir, float atten)
{
  vec4 col4;
  if (idObj == idBoat) col4 = vec4 (0.3, 0.3, 0.6, 0.2);
  else if (idObj == idFLamp) col4 = vec4 (0., 1., 0., -1.);
  else if (idObj == idBLamp) col4 = vec4 (1., 0., 0., -1.);
  if (col4.a >= 0.)
    col4.rgb = col4.rgb * (0.2 + 0.8 * CaveSShadow (ro, ltDir)) *
       (0.1 + 0.9 * atten * max (dot (ltDir, vn), 0.)) +
       col4.a * atten * pow (max (dot (normalize (ltDir - rd), vn), 0.), 64.);
  return col4.rgb;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col, colR, bgCol, ltVec, vn, roo, rdo, row, vnw;
  float dstCave, dstObj, atten, frFac;
  roo = ro;
  rdo = rd;
  bgCol = (abs (rd.y) < 0.5) ? vec3 (0., 0.05, 0.08) : vec3 (0.01);
  uWat = false;
  hitWat = false;
  dstCave = CaveRay (ro, rd);
  dstObj = ObjRay (ro, rd);
  if (dstCave < min (dstObj, dstFar) && ro.y + rd.y * dstCave < htWat + 0.001) {
    hitWat = true;
    ro += rd * dstCave;
    row = ro;
    vnw = VaryNf (1.5 * ro, vec3 (0., 1., 0.), 0.1);
    rd = reflect (rd, vnw);
    ro += 0.01 * rd;
    dstCave = CaveRay (ro, rd);
    dstObj = ObjRay (ro, rd);
  }
  if (min (dstCave, dstObj) < dstFar) {
    ltVec = roo + ltPos[0] - ro;
    atten = 1. / (0.1 + dot (ltVec, ltVec));
    if (hitWat) atten *= 3.;
    ltVec = normalize (ltVec);
    ro += min (dstCave, dstObj) * rd;
    if (dstCave < dstObj) col = mix (CaveCol (ro, rd, ltVec, atten), bgCol,
       smoothstep (0.45, 0.99, dstCave / dstFar));
    else col = ObjCol (ro, rd, ObjNf (ro), ltVec, atten);
  } else col = bgCol;
  if (hitWat) {
    frFac = rdo.y * rdo.y;
    frFac *= frFac;
    if (frFac > 0.005) {
      rd = refract (rdo, vnw, 1./1.333);
      ro = row + 0.01 * rd;
      uWat = true;
      dstCave = CaveRay (ro, rd);
      if (min (dstCave, dstObj) < dstFar) {
        ltVec = roo + ltPos[1] - ro;
        atten = 1. / (0.1 + dot (ltVec, ltVec));
        ltVec = normalize (ltVec);
        ro += rd * dstCave;
        hitWat = false;
        colR = mix (CaveCol (ro, rd, ltVec, atten), bgCol,
           smoothstep (0.45, 0.99, dstCave / dstFar));
      } else colR = bgCol;
      col = mix (col, colR * vec3 (0.4, 1., 0.6) * exp (0.02 * ro.y), frFac);
    }
  }
  return pow (clamp (col, 0., 1.), vec3 (0.8));
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
#ifdef MOUSE
  vec4 mPtr;
#endif
  vec3 ro, rd, fpF, fpB, vd;
  vec2 canvas, uv, ori, ca, sa;
  float el, az, t, tt, a;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iGlobalTime;
#ifdef MOUSE
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
#endif
  t = 1. * tCur;
  az = 0.;
  el = 0.;
#ifdef MOUSE
  if (mPtr.z > 0.) {
    az = az + 2. * pi * mPtr.x;
    el = el + 0.95 * pi * mPtr.y;
  } else {
    tt = mod (floor (0.05 * tCur), 4.);
    a = 0.45 * pi * SmoothBump (0.75, 0.95, 0.05, mod (0.05 * tCur, 1.));
    if (tt < 2.) el = (2. * tt - 1.) * a;
    else az = (2. * tt - 5.) * a;
  }
#else
    tt = mod (floor (0.05 * tCur), 4.);
    a = 0.45 * pi * SmoothBump (0.75, 0.95, 0.05, mod (0.05 * tCur, 1.));
    if (tt < 2.) el = (2. * tt - 1.) * a;
    else az = (2. * tt - 5.) * a;
#endif
  htWat = -0.5;
  for (int k = 0; k < 2; k ++) {
    fpF = TrackPath (t + 3. + 3. * float (k) + 0.1);
    fpB = TrackPath (t + 3. + 3. * float (k) - 0.1);
    boatPos[k] = 0.5 * (fpF + fpB);
    boatPos[k].y = htWat + 0.01;
    vd = fpF - fpB;
    boatAng[k] = (length (vd.xz) > 0.) ? atan (vd.x, vd.z) : 0.5 * pi;
  }
  fpF = TrackPath (t + 0.1);
  fpB = TrackPath (t - 0.1);
  ro = 0.5 * (fpF + fpB);
  vd = fpF - fpB;
  ori = vec2 (el, az + ((length (vd.xz) > 0.) ? atan (vd.x, vd.z) : 0.5 * pi));
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  rd = vuMat * normalize (vec3 (uv, 2.));
  ltPos[0] = 0.5 * vuMat * vec3 (0., 1., -1.);
  ltPos[1] = 0.5 * vuMat * vec3 (0., -1., -1.);
  dstFar = 50.;
  fragColor = vec4 (ShowScene (ro, rd) , 1.);
}

float PrSphDf (vec3 p, float r)
{
  return length (p) - r;
}

float PrCapsDf (vec3 p, float r, float h)
{
  return length (p - vec3 (0., 0., clamp (p.z, - h, h))) - r;
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

vec2 Rot2D (vec2 q, float a)
{
  return q * cos (a) + q.yx * sin (a) * vec2 (-1., 1.);
}

const vec4 cHashA4 = vec4 (0., 1., 57., 58.);
const vec3 cHashA3 = vec3 (1., 57., 113.);
const float cHashM = 43758.54;

float Hashfv2 (vec2 p)
{
  return fract (sin (dot (p, cHashA3.xy)) * cHashM);
}

float Hashfv3 (vec3 p)
{
  return fract (sin (dot (p, cHashA3)) * cHashM);
}

vec3 Hashv3f (float p)
{
  return fract (sin (vec3 (p, p + 1., p + 2.)) *
     vec3 (cHashM, cHashM * 0.43, cHashM * 0.37));
}

vec4 Hashv4f (float p)
{
  return fract (sin (p + cHashA4) * cHashM);
}

float Noisefv2 (vec2 p)
{
  vec4 t;
  vec2 ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = Hashv4f (dot (ip, cHashA3.xy));
  return mix (mix (t.x, t.y, fp.x), mix (t.z, t.w, fp.x), fp.y);
}

float Fbmn (vec3 p, vec3 n)
{
  vec3 s;
  float a;
  s = vec3 (0.);
  a = 1.;
  for (int i = 0; i < 5; i ++) {
    s += a * vec3 (Noisefv2 (p.yz), Noisefv2 (p.zx), Noisefv2 (p.xy));
    a *= 0.5;
    p *= 2.;
  }
  return dot (s, abs (n));
}

vec3 VaryNf (vec3 p, vec3 n, float f)
{
  vec3 g;
  const vec3 e = vec3 (0.1, 0., 0.);
  g = vec3 (Fbmn (p + e.xyy, n), Fbmn (p + e.yxy, n), Fbmn (p + e.yyx, n)) -
     Fbmn (p, n);
  return normalize (n + f * (g - n * dot (n, g)));
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
