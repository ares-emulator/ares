#version 450
// White Folly -  dr2 - 2017-11-07
// https://www.shadertoy.com/view/ll2cDG

// Folly (architectural) with spiral stairways (in a pond filled with Voronoi stones); mouse enabled

// "White Folly" by dr2 - 2017
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

float PrBoxDf (vec3 p, vec3 b);
float PrBox2Df (vec2 p, vec2 b);
float PrCylDf (vec3 p, float r, float h);
float PrCylAnDf (vec3 p, float r, float w, float h);
void HexVorInit ();
vec4 HexVor (vec2 p);
float SmoothMin (float a, float b, float r);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);
vec3 HsvToRgb (vec3 c);
float Hashfv2 (vec2 p);
vec2 Hashv2v2 (vec2 p);
float Noisefv2 (vec2 p);
float Fbm2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);

const float pi = 3.14159;

vec3 sunDir, qHit;
float dstFar, tCur, tWav;
int idObj;
bool inWat;
const int idStr = 1, idBal = 2, idPlat = 3, idBalc = 4, idPil = 5, idWl = 6, idFlr = 7;

float ObjDfS (vec3 p, float dMin)
{
  vec3 q;
  float d, db, s, a;
  q = p;
  q.xz = abs (Rot2D (q.xz, pi)) - 6.5;
  db = PrBox2Df (q.xz, vec2 (4.));
  q.xz += 6.5;
  q.xz = Rot2D (q.xz, 0.75 * pi);
  q.x += 4.;
  a = (length (q.xz) > 0.) ? atan (q.z, - q.x) / (2. * pi) : 0.;
  q.xz = vec2 (24. * a, length (q.xz) - 6.);
  q.xy = Rot2D (q.xy, -0.25 * pi);
  s = mod (q.x, sqrt (0.5));
  d = max (0.3 * max (q.y - min (s, sqrt (0.5) - s), max (-0.1 - q.y, abs (q.z) - 1.5)),
     abs (p.y) - 3.5);
  d = max (d, db);
  if (d < dMin) { dMin = d;  idObj = idStr; }
  q.xy -= vec2 (1.5, 1.4);
  q.z = abs (q.z) - 1.43;
  d = PrBoxDf (q, vec3 (4.7, 0.07, 0.07));
  q.x = 0.5 * mod (96. * a + 0.5, 1.) - 0.35;
  q.y += 0.7;
  d = min (d,  PrCylDf (q.xzy, 0.05, 0.7));
  d = max (0.3 * d, db);
  if (d < dMin) { dMin = d;  idObj = idBal; }
  return dMin;
}

/*
  This function is called twice, for the two orientations of the horizontal 
  walkway. Could be replaced by a single call, with orientation dependent on 
  position along ray path; this is faster (good) but there are visual artifacts for 
  certain view directions (bad). Artifacts can be removed by using cells in the 
  vertical direction (good), but this is slower (bad).
*/

float ObjDfB (vec3 p, float dMin)
{
  vec3 q;
  float d;
  q = p;
  d = max (PrBoxDf (q, vec3 (10.35, 0.26, 2.85)),
    - max (length (vec2 (mod (q.x + 2., 4.) - 2., q.z)) - 1.5, 0.3 - abs (q.z)));
  if (d < dMin) { dMin = d;  idObj = idPlat;  qHit = q; }
  q = p;  q.y -= 2.05;  q.z = abs (q.z) - 2.45;
  d = PrBoxDf (q, vec3 (7.45, 0.08, 0.07));
  q.x = mod (q.x + 0.25, 0.5) - 0.25;
  q.y += 0.95;
  d = min (d, max (PrCylDf (q.xzy, 0.06, 0.9), abs (p.x) - 7.45));
  q = p;  q.y -= 1.06;  q.x = abs (q.x) - 10.23;  q.y -= 0.95;
  d = min (d, PrBoxDf (q, vec3 (0.07, 0.08, 2.5)));
  q.y += 0.95;  q.z = mod (q.z + 0.25, 0.5) - 0.25;
  d = min (d, max (PrCylDf (q.xzy, 0.06, 0.9), abs (p.z) - 2.45));
  if (d < dMin) { dMin = d;  idObj = idBalc; }
  q = p;  q.xz = abs (q.xz) - vec2 (8.8, 2.4);  q.x = abs (q.x) - 1.45;  q.y -= 1.3;
  d = PrCylDf (q.xzy, 0.2, 1.05);
  if (d < dMin) { dMin = d;  idObj = idPil;  qHit = q; }
  return dMin;
}

float ObjDf (vec3 p)
{
  vec3 q;
  float dMin, d;
  dMin = dstFar;
  p.y -= 3.;
  if (! inWat) {
    dMin = ObjDfS (p, dMin);
    q = p;  q.y -= 3.25;
    dMin = ObjDfB (q, dMin);
    q = p;  q.y -= -3.25;  q.xz = vec2 (- q.z, q.x);
    dMin = ObjDfB (q, dMin);
    q = p;  q.y -= 9.;
    d = max (PrBoxDf (q, vec3 (2.5, 0.15, 2.5)),
       - max (length (q.xz) - 1., max (0.1 - abs (q.x), 0.1 - abs (q.z))));
    if (d < dMin) { dMin = d;  idObj = idPlat;  qHit = q; }
  }
  q = p;  q.xz = abs (q.xz) - 1.8;  q.y -= 1.;
  d = PrCylDf (q.xzy, 0.2, 8.);
  if (d < dMin) { dMin = d;  idObj = idPil;  qHit = q; }
  q = p;  q.y -= -5.2;
  d = PrCylAnDf (q.xzy, 20., 0.3, 2.3);
  if (d < dMin) { dMin = d;  idObj = idWl; }
  q = p;  q.y -= -7.4;
  d = PrCylDf (q.xzy, 20., 0.01);
  if (d < dMin) { dMin = d;  idObj = idFlr; }
  return dMin;
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d, eps;
  eps = 0.001;
  dHit = 0.;
  for (int j = 0; j < 160; j ++) {
    d = ObjDf (ro + dHit * rd);
    dHit += d;
    if (d < eps || dHit > dstFar) break;
  }
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  vec4 v;
  vec3 e = vec3 (0.001, -0.001, 0.);
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy), ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

float ObjSShadow (vec3 ro, vec3 rd)
{
  float sh, d, h;
  sh = 1.;
  d = 0.05;
  for (int j = 0; j < 40; j ++) {
    h = ObjDf (ro + rd * d);
    sh = min (sh, smoothstep (0., 0.05 * d, h));
    d += h;
    if (sh < 0.05) break;
  }
  return sh;
}

vec3 BgCol (vec3 ro, vec3 rd)
{
  vec3 col;
  if (rd.y >= 0.) col = mix (vec3 (0.1, 0.2, 0.4), vec3 (1.), 0.1 + 0.8 * rd.y);
  else {
    ro -= ((ro.y + 0.5) / rd.y) * rd;
    col = mix (0.7 * mix (vec3 (0.3, 0.4, 0.1), vec3 (0.4, 0.5, 0.2), Fbm2 (ro.xz)) *
         (1. - 0.15 * Noisefv2 (330. * ro.xz)), vec3 (0.18, 0.28, 0.48), pow (1. + rd.y, 5.));
  }
  return col;
}

float WaveHt (vec2 p)
{
  mat2 qRot = mat2 (0.8, -0.6, 0.6, 0.8);
  vec4 t4, v4;
  vec2 t;
  float wFreq, wAmp, ht;
  wFreq = 1.;
  wAmp = 1.;
  ht = 0.;
  for (int j = 0; j < 3; j ++) {
    p *= qRot;
    t = tWav * vec2 (1., -1.);
    t4 = (p.xyxy + t.xxyy) * wFreq;
    t = vec2 (Noisefv2 (t4.xy), Noisefv2 (t4.zw));
    t4 += 2. * t.xxyy - 1.;
    v4 = (1. - abs (sin (t4))) * (abs (sin (t4)) + abs (cos (t4)));
    ht += wAmp * dot (pow (1. - sqrt (v4.xz * v4.yw), vec2 (8.)), vec2 (1.));
    wFreq *= 2.;
    wAmp *= 0.5;
  }
  return ht;
}

vec3 WaveNf (vec3 p, float d)
{
  vec3 vn;
  vec2 e;
  e = vec2 (max (0.01, 0.005 * d * d), 0.);
  p *= 0.5;
  vn.xz = 3. * (WaveHt (p.xz) - vec2 (WaveHt (p.xz + e.xy),  WaveHt (p.xz + e.yx)));
  vn.y = e.x;
  return normalize (vn);
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 vc;
  vec3 vn, vnw, row, rdw, col;
  float dstObj, dstWat, s, a, sh;
  bool isRefl;
  HexVorInit ();
  inWat = false;
  isRefl = false;
  tWav = 0.3 * tCur;
  dstObj = ObjRay (ro, rd);
  dstWat = - (ro.y + 0.6) / rd.y;
  if (dstWat < min (dstObj, dstFar) && length ((ro + dstWat * rd).xz) < 20.) {
    ro += dstWat * rd;
    row = ro;
    rdw = rd;
    vnw = WaveNf (1.5 * ro, dstWat);;
    rd = refract (rd, vnw, 1./1.333);
    ro += 0.01 * rd;
    inWat = true;
    dstObj = ObjRay (ro, rd);
    inWat = false;
    isRefl = true;
  }
  if (dstObj < dstFar) {
    ro += rd * dstObj;
    if (ro.y < -0.5 && length (ro.xz) > 20.3) col = BgCol (ro, rd);
    else {
      vn = ObjNf (ro);
      if (idObj == idStr) {
        col = vec3 (0.95, 0.95, 1.);
      } else if (idObj == idBal || idObj == idBalc) {
        col = vec3 (0.8, 0.8, 1.);
      } else if (idObj == idPlat) {
        col = vec3 (1.);
        if (vn.y > 0.99) {
          if (ro.y > 7.5) s = mod (3. * length (qHit.xz), 1.);
          else s = mod (3. * qHit.x, 1.);
          col *= 0.8 + 0.2 * SmoothBump (0.1, 0.9, 0.03, s);
        } else if (abs (vn.y) < 0.01) {
          s = mod (8. * ro.y, 1.);
          col *= 0.8 + 0.2 * SmoothBump (0.1, 0.9, 0.03, s);
        }
        vn = VaryNf (100. * ro, vn, 0.2); 
      } else if (idObj == idPil) {
        if (abs (vn.y) < 0.01) {
          a = (length (qHit.xz) > 0.) ? atan (qHit.z, - qHit.x) / pi : 0.;
          s = mod (3. * qHit.y + a, 1.);
          vn.y = 0.2 * (1. - SmoothBump (0.2, 0.8, 0.1, s)) * sign (s - 0.5);
          vn.xz *= sqrt (1. - vn.y * vn.y);
        }
        col = vec3 (0.9, 0.9, 0.3);
      } else if (idObj == idWl) {
        a = (length (ro.xz) > 0.) ? atan (ro.z, - ro.x) / pi : 0.;
        col = vec3 (0.6, 0.4, 0.3) * (0.5 +
           0.5 * SmoothBump (0.05, 0.95, 0.02, mod (64. * a, 1.))) *
           (0.5 + 0.5 * SmoothBump (0.03, 0.97, 0.01, mod (ro.y + 0.5, 1.)));
        vn = VaryNf (20. * ro, vn, 1.);   
      } else if (idObj == idFlr) {
        vc = HexVor (ro.xz);
        vn.xz = - 0.7 * vc.yz;
        vn = normalize (vn);
        s = mod (10. * vc.w, 1.);
        col = HsvToRgb (vec3 (0.1 + 0.3 * step (2. * s, 1.) + 0.1 * mod (5. * s, 1.),
           0.5 + 0.5 * mod (17. * s, 1.), 0.7 + 0.3 * mod (12. * s, 1.))) *
           (0.6 + 0.4 * smoothstep (0., 0.2, vc.x)) * (1. - 0.2 * Noisefv2 (128. * ro.xz));
      }
      sh = 0.4 + 0.6 * ObjSShadow (ro, sunDir);
      col = col * (0.2 + sh * max (dot (sunDir, vn), 0.) +
         0.1 * max (dot (- sunDir.xz, vn.xz), 0.)) +
         0.1 * sh * pow (max (dot (normalize (sunDir - rd), vn), 0.), 64.);
    }
  } else {
    if (isRefl) sh = ObjSShadow (row, sunDir);
    col = BgCol (ro, rd);
  }
  if (isRefl) {
    col = mix (0.9 * col, vec3 (1., 1., 0.9), sh *
       pow (max (0., dot (sunDir, reflect (rdw, vnw))), 64.));
  }
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
#ifdef MOUSE
  vec4 mPtr;
#endif
  vec3 ro, rd;
  vec2 canvas, uv, ori, ca, sa;
  float el, az, zmFac;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iGlobalTime;
#ifdef MOUSE
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
#endif
  dstFar = 120.;
  az = 0.;
  el = -0.2 * pi;
#ifdef MOUSE
  if (mPtr.z > 0.) {
    az += 3. * pi * mPtr.x;
    el += 1. * pi * mPtr.y;
  } else {
    az -= 0.1 * tCur;
    el -= 0.1 * pi * cos (0.03 * pi * tCur);
  }
#else
  az -= 0.1 * tCur;
  el -= 0.1 * pi * cos (0.03 * pi * tCur);
#endif
  el = clamp (el, -0.4 * pi, -0.05 * pi);
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  zmFac = 7. - 2. * cos (az);
  rd = vuMat * normalize (vec3 (uv, zmFac));
  ro = vuMat * vec3 (0., 1., -70.);
  sunDir = vuMat * normalize (vec3 (1., 1., -1.));
  fragColor = vec4 (ShowScene (ro, rd), 1.);
}

float PrBoxDf (vec3 p, vec3 b)
{
  vec3 d;
  d = abs (p) - b;
  return min (max (d.x, max (d.y, d.z)), 0.) + length (max (d, 0.));
}

float PrBox2Df (vec2 p, vec2 b)
{
  vec2 d;
  d = abs (p) - b;
  return min (max (d.x, d.y), 0.) + length (max (d, 0.));
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrCylAnDf (vec3 p, float r, float w, float h)
{
  return max (abs (length (p.xy) - r) - w, abs (p.z) - h);
}

vec2 gVec[7], hVec[7];
#define SQRT3 1.7320508

vec2 PixToHex (vec2 p)
{
  vec3 c, r, dr;
  c.xz = vec2 ((1./SQRT3) * p.x - (1./3.) * p.y, (2./3.) * p.y);
  c.y = - c.x - c.z;
  r = floor (c + 0.5);
  dr = abs (r - c);
  r -= step (dr.yzx, dr) * step (dr.zxy, dr) * dot (r, vec3 (1.));
  return r.xz;
}

vec2 HexToPix (vec2 h)
{
  return vec2 (SQRT3 * (h.x + 0.5 * h.y), (3./2.) * h.y);
}

void HexVorInit ()
{
  vec3 e = vec3 (1., 0., -1.);
  gVec[0] = e.yy;
  gVec[1] = e.xy;
  gVec[2] = e.yx;
  gVec[3] = e.xz;
  gVec[4] = e.zy;
  gVec[5] = e.yz;
  gVec[6] = e.zx;
  for (int k = 0; k < 7; k ++) hVec[k] = HexToPix (gVec[k]);
}

vec4 HexVor (vec2 p)
{
  vec4 sd, udm;
  vec2 ip, fp, d, u;
  float amp, a;
  amp = 0.7;
  ip = PixToHex (p);
  fp = p - HexToPix (ip);
  sd = vec4 (4.);
  udm = vec4 (4.);
  for (int k = 0; k < 7; k ++) {
    u = Hashv2v2 (ip + gVec[k]);
    a = 2. * pi * (u.y - 0.5);
    d = hVec[k] + amp * (0.4 + 0.6 * u.x) * vec2 (cos (a), sin (a)) - fp;
    sd.w = dot (d, d);
    if (sd.w < sd.x) {
      sd = sd.wxyw;
      udm = vec4 (d, u);
    } else sd = (sd.w < sd.y) ? sd.xwyw : ((sd.w < sd.z) ? sd.xyww : sd);
  }
  sd.xyz = sqrt (sd.xyz);
  return vec4 (SmoothMin (sd.y, sd.z, 0.3) - sd.x, udm.xy, Hashfv2 (udm.zw));
}

float SmoothMin (float a, float b, float r)
{
  float h;
  h = clamp (0.5 + 0.5 * (b - a) / r, 0., 1.);
  return mix (b, a, h) - r * h * (1. - h);
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

vec2 Rot2D (vec2 q, float a)
{
  return q * cos (a) + q.yx * sin (a) * vec2 (-1., 1.);
}

vec3 HsvToRgb (vec3 c)
{
  vec3 p;
  p = abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.);
  return c.z * mix (vec3 (1.), clamp (p - 1., 0., 1.), c.y);
}

const float cHashM = 43758.54;

float Hashfv2 (vec2 p)
{
  return fract (sin (dot (p, vec2 (37., 39.))) * cHashM);
}

vec2 Hashv2v2 (vec2 p)
{
  vec2 cHashVA2 = vec2 (37., 39.);
  return fract (sin (vec2 (dot (p, cHashVA2), dot (p + vec2 (1., 0.), cHashVA2))) * cHashM);
}

float Noisefv2 (vec2 p)
{
  vec2 t, ip, fp;
  ip = floor (p);  
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = mix (Hashv2v2 (ip), Hashv2v2 (ip + vec2 (0., 1.)), fp.y);
  return mix (t.x, t.y, fp.x);
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
  return f * (1. / 1.9375);
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
  vec2 e = vec2 (0.1, 0.);
  g = vec3 (Fbmn (p + e.xyy, n), Fbmn (p + e.yxy, n), Fbmn (p + e.yyx, n)) - Fbmn (p, n);
  return normalize (n + f * (g - n * dot (n, g)));
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
