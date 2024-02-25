#version 450
// Droplet -  dr2 - 2015-09-17
// https://www.shadertoy.com/view/4l2Szm

// Probably liquid mercury (change viewpoint using the mouse).

// "Droplet" by dr2 - 2015
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

const float pi = 3.14159;
const vec4 cHashA4 = vec4 (0., 1., 57., 58.);
const vec3 cHashA3 = vec3 (1., 57., 113.);
const float cHashM = 43758.54;

vec4 Hashv4f (float p)
{
  return fract (sin (p + cHashA4) * cHashM);
}

float Noisefv2 (vec2 p)
{
  vec2 i = floor (p);
  vec2 f = fract (p);
  f = f * f * (3. - 2. * f);
  vec4 t = Hashv4f (dot (i, cHashA3.xy));
  return mix (mix (t.x, t.y, f.x), mix (t.z, t.w, f.x), f.y);
}

float Fbm2 (vec2 p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * Noisefv2 (p);
    a *= 0.5;
    p *= 2.;
  }
  return f;
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
  float s;
  vec3 e = vec3 (0.1, 0., 0.);
  s = Fbmn (p, n);
  g = vec3 (Fbmn (p + e.xyy, n) - s,
     Fbmn (p + e.yxy, n) - s, Fbmn (p + e.yyx, n) - s);
  return normalize (n + f * (g - n * dot (n, g)));
}

float Length4 (vec2 p)
{
  p *= p;
  p *= p;
  return pow (p.x + p.y, 1. / 4.);
}

float Length6 (vec2 p)
{
  p *= p * p;
  p *= p;
  return pow (p.x + p.y, 1. / 6.);
}

float PrSphDf (vec3 p, float s)
{
  return length (p) - s;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrTorus4Df (vec3 p, float ri, float rc)
{
  return Length4 (vec2 (length (p.xz) - rc, p.y)) - ri;
}

vec3 sunDir;
float tCur;
int idObj;
const float dstFar = 100.;
const int idRing = 1, idWat = 2;

vec3 BgCol (vec3 ro, vec3 rd)
{
  vec3 col;
  float sd, f;
  if (rd.y > 0.) {
    col = vec3 (0.1, 0.2, 0.4) + 0.2 * pow (1. - max (rd.y, 0.), 8.);
    sd = max (dot (rd, sunDir), 0.);
    ro.xz += 2. * tCur;
    f = Fbm2 (0.1 * (rd.xz * (50. - ro.y) / rd.y + ro.xz));
    col += 0.35 * pow (sd, 6.) + 0.65 * min (pow (sd, 256.), 0.3);
    col = mix (col, vec3 (1.), clamp (0.8 * f * rd.y + 0.1, 0., 1.));
  } else {
    f = Fbm2 (0.4 * (ro.xz - ro.y * rd.xz / rd.y));
    col = mix ((1. + min (f, 1.)) * vec3 (0.05, 0.1, 0.05),
       vec3 (0.1, 0.15, 0.25), pow (1. + rd.y, 5.));
  }
  return col;
}

float StoneRingDf (vec3 p, float r, float w, float n)
{
  return Length6 (vec2 (length (p.xz) - r, p.y)) -
     w * (0.2 * pow (abs (sin (atan (p.x, p.z) * n)), 0.25) + 0.8);
}

float ObjDf (vec3 p)
{
  vec3 q;
  float dMin, d, db, r, s, t;
  bool up;
  dMin = dstFar;
  t = mod (tCur, 10.);
  r = abs (sin (2. * pi * 0.1 * t));
  q = p;
  up = (t < 5.);
  q.y -= up ? 2.5 : 0.55;
  d = PrTorus4Df (q, 1., r);
  q.y -= up ? -0.5 : 0.5;
  d = max (PrCylDf (q.xzy, r, 0.5), - d);
  if (up) d = max (d, q.y);
  q.y -= up ? -0.75 : 0.2;
  s = length (q.xz);
  q.y -= 0.02 * cos (15. * s - 7. * tCur) * clamp (1. - s / 2.5, 0., 1.) *
     clamp (s, 0., 1.);
  db = PrCylDf (q.xzy, 2.5, 0.25);
  d = up ? min (db, d) : max (db, - d);
  if (d < dMin) { dMin = d;  idObj = idWat; }
  q = p;
  s = 1. - sqrt (max (1. - r * r, 0.));
  q.y -= 1.2 + (up ? s : - s);
  d = PrSphDf (q, 0.3);
  d = max (d, 1. - p.y);
  if (d < dMin) { dMin = d;  idObj = idWat; }
  q = p;
  q.y -= 1.3;
  d = StoneRingDf (q, 2.8, 0.3, 16.);
  if (d < dMin) { dMin = d;  idObj = idRing; }
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
  const vec3 e = vec3 (0.001, -0.001, 0.);
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy),
     ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 objCol, col, vn;
  float dstHit, dif, bk;
  int idObjT;
  const int nRefl = 3;
  for (int k = 0; k < nRefl; k ++) {
    idObj = -1;
    dstHit = ObjRay (ro, rd);
    if (dstHit < dstFar && idObj == idWat) {
      ro += rd * dstHit;
      rd = reflect (rd, VaryNf (ro, ObjNf (ro), 0.1));
      ro += 0.02 * rd;
    } else break;
  }
  if (dstHit < dstFar) {
    ro += rd * dstHit;
    idObjT = idObj;
    vn = ObjNf (ro);
    idObj = idObjT;
    if (idObj == idRing) {
      objCol = vec3 (0.8, 0.6, 0.2);
      vn = VaryNf (40. * ro, vn, 2.);
    }
    bk = max (dot (vn, sunDir * vec3 (-1., 1., -1.)), 0.);
    dif = max (dot (vn, sunDir), 0.);
    col = objCol * (0.1 + 0.1 * bk + 0.8 * dif +
       0.3 * pow (max (0., dot (sunDir, reflect (rd, vn))), 64.));
  } else col = BgCol (ro, rd);
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  vec2 canvas = iResolution.xy;
  vec2 uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iGlobalTime;
  sunDir = normalize (vec3 (1., 1., 1.));
  float el = 0.6;
#ifdef MOUSE
  vec4 mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  if (mPtr.z > 0.) el = clamp (el - mPtr.y, 0.25, 0.8);
#endif
  float cEl = cos (el);
  float sEl = sin (el);
  mat3 vuMat = mat3 (1., 0., 0., 0., cEl, - sEl, 0., sEl, cEl);
  vec3 rd = normalize (vec3 (uv, 4.)) * vuMat;
  vec3 ro = vec3 (0., 0.7, -10.) * vuMat;
  fragColor = vec4 (ShowScene (ro, rd), 1.);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
