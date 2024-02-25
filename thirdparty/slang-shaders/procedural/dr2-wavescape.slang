#version 450
// Wavescape -  dr2 - 2015-01-05
// https://www.shadertoy.com/view/lls3z7

// Another wave renderer, from both above and below the waterline. See the source for where it all comes from.

// "Wavescape" by dr2 - 2015
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License

// Water waves, including what the fish sees.

// Acknowledgments: thanks for the following -
//  Dave_H's multilayered clouds with nimitz's variable layer spacing
//    (but depends on elevation rather than distance).
//  Wave shapes from TDM; they seem a little more "energetic" than TekF's.
//  Raymarching with binary subdivision, as used by Dave_H for mountains;
//    TekF and TDM use one or the other, not both.
//  Buoy based on TekF's, but raymarched for generality; shows aging effects
//    below waterline.
//  Foam idea from TekF.
//  Noise functions from iq.

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

vec2 Hashv2f (float p)
{
  return fract (sin (p + cHashA4.xy) * cHashM);
}

vec4 Hashv4f (float p)
{
  return fract (sin (p + cHashA4) * cHashM);
}

vec4 Hashv4v3 (vec3 p)
{
  const vec3 cHashVA3 = vec3 (37.1, 61.7, 12.4);
  const vec3 e = vec3 (1., 0., 0.);
  return fract (sin (vec4 (dot (p + e.yyy, cHashVA3), dot (p + e.xyy, cHashVA3),
     dot (p + e.yxy, cHashVA3), dot (p + e.xxy, cHashVA3))) * cHashM);
}

float Noiseff (float p)
{
  float i, f;
  i = floor (p);  f = fract (p);
  f = f * f * (3. - 2. * f);
  vec2 t = Hashv2f (i);
  return mix (t.x, t.y, f);
}

float Noisefv2 (vec2 p)
{
  vec2 i, f;
  i = floor (p);  f = fract (p);
  f = f * f * (3. - 2. * f);
  vec4 t = Hashv4f (dot (i, cHashA3.xy));
  return mix (mix (t.x, t.y, f.x), mix (t.z, t.w, f.x), f.y);
}

float Noisefv3a (vec3 p)
{
  vec3 i, f;
  i = floor (p);  f = fract (p);
  f *= f * (3. - 2. * f);
  vec4 t1 = Hashv4v3 (i);
  vec4 t2 = Hashv4v3 (i + vec3 (0., 0., 1.));
  return mix (mix (mix (t1.x, t1.y, f.x), mix (t1.z, t1.w, f.x), f.y),
              mix (mix (t2.x, t2.y, f.x), mix (t2.z, t2.w, f.x), f.y), f.z);
}

float Fbm1 (float p)
{
  float f, a;
  f = 0.;  a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * Noiseff (p);
    a *= 0.5;  p *= 2.;
  }
  return f;
}

float Fbm3 (vec3 p)
{
  const mat3 mr = mat3 (0., 0.8, 0.6, -0.8, 0.36, -0.48, -0.6, -0.48, 0.64);
  float f, a, am, ap;
  f = 0.;  a = 0.5;
  am = 0.5;  ap = 4.;
  p *= 0.5;
  for (int i = 0; i < 6; i ++) {
    f += a * Noisefv3a (p);
    p *= mr * ap;  a *= am;
  }
  return f;
}

float Fbmn (vec3 p, vec3 n)
{
  vec3 f = vec3 (0.);
  float a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * vec3 (Noisefv2 (p.yz), Noisefv2 (p.zx), Noisefv2 (p.xy));
    a *= 0.5;  p *= 2.;
  }
  return dot (f, abs (n));
}

vec3 VaryNf (vec3 p, vec3 n, float f)
{
  vec3 e = vec3 (0.2, 0., 0.);
  float s = Fbmn (p, n);
  vec3 g = vec3 (Fbmn (p + e.xyy, n) - s,
     Fbmn (p + e.yxy, n) - s, Fbmn (p + e.yyx, n) - s);
  return normalize (n + f * (g - n * dot (n, g)));
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

float PrSphDf (vec3 p, float s)
{
  return length (p) - s;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

int idObj;
mat3 ballMat;
vec3 qHit, ballPos, sunCol, sunDir, cloudDisp, waterDisp;
float tCur, fCloud;
const float dstFar = 300.;

vec3 SkyGrndCol (vec3 ro, vec3 rd)
{
  vec3 p, q, cSun, skyBg, clCol, col;
  float colSum, attSum, s, att, a, dDotS, ds;
  const vec3 cCol1 = 0.5 * vec3 (0.15, 0.2, 0.4),
     cCol2 = 0.5 * vec3 (0.25, 0.5, 0.7), gCol = 1.3 * vec3 (0.05, 0.08, 0.05);
  const float cloudLo = 100., cloudRngI = 1./50., atFac = 0.06;
  const int nLay = 30;
  if (rd.y < 0.015 * Fbm1 (16. * rd.x)) col = gCol * (0.5 + 0.5 *
     Noisefv2 (1000. * vec2 (5. * atan (rd.x, rd.z), rd.y)));
  else {
    fCloud = clamp (fCloud, 0., 1.);
    dDotS = max (dot (rd, sunDir), 0.);
    ro += cloudDisp;
    p = ro;
    p.xz += (cloudLo - p.y) * rd.xz / rd.y;
    p.y = cloudLo;
    ds = 1. / (cloudRngI * rd.y * (2. - rd.y) * float (nLay));
    colSum = 0.;  attSum = 0.;
    s = 0.;  att = 0.;
    for (int j = 0; j < nLay; j ++) {
      q = p + rd * s;
      q.z *= 0.7;
      att += atFac * max (fCloud - Fbm3 (0.02 * q), 0.);
      a = (1. - attSum) * att;
      colSum += a * (q.y - cloudLo) * cloudRngI;
      attSum += a;  s += ds;
      if (attSum >= 1.) break;
    }
    colSum += 0.5 * min ((1. - attSum) * pow (dDotS, 3.), 1.);
    clCol = vec3 (1.) * colSum + 0.05 * sunCol;
    cSun = sunCol * clamp ((min (pow (dDotS, 1500.) * 2., 1.) +
       min (pow (dDotS, 10.) * 0.75, 1.)), 0., 1.);
    skyBg = mix (cCol1, cCol2, 1. - rd.y);
    col = clamp (mix (skyBg + cSun, 1.6 * clCol, attSum), 0., 1.);
  }
  return col;
}

vec3 SeaFloorCol (vec3 rd)
{
  vec2 p;
  float w, f;
  p = 5. * rd.xz / rd.y;
  w = 1.;
  f = 0.;
  for (int j = 0; j < 4; j ++) {
    f += w * Noisefv2 (p);
    w *= 0.5;  p *= 2.;
  }
  return mix (vec3 (0.01, 0.04, 0.02), vec3 (0, 0.05, 0.05), 
     smoothstep (0.4, 0.7, f));
}

float WaveHt (vec3 p)
{
  const mat2 qRot = mat2 (1.6, -1.2, 1.2, 1.6);
  vec4 t4, ta4, v4;
  vec2 q2, t2, v2;
  float wFreq, wAmp, pRough, ht;
  wFreq = 0.16;  wAmp = 0.6;  pRough = 5.;
  q2 = p.xz + waterDisp.xz;
  ht = 0.;
  for (int j = 0; j < 5; j ++) {
    t2 = 1.1 * tCur * vec2 (1., -1.);
    t4 = vec4 (q2 + t2.xx, q2 + t2.yy) * wFreq;
    t2 = vec2 (Noisefv2 (t4.xy), Noisefv2 (t4.zw));
    t4 += 2. * vec4 (t2.xx, t2.yy) - 1.;
    ta4 = abs (sin (t4));
    v4 = (1. - ta4) * (ta4 + abs (cos (t4)));
    v2 = pow (1. - pow (v4.xz * v4.yw, vec2 (0.65)), vec2 (pRough));
    ht += (v2.x + v2.y) * wAmp;
    q2 *= qRot;  wFreq *= 1.9;  wAmp *= 0.22;
    pRough = 0.8 * pRough + 0.2;
  }
  return ht;
}

float WaveRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float dHit, h, s, sLo, sHi;
  s = 0.;
  sLo = 0.;
  dHit = dstFar;
  for (int j = 0; j < 150; j ++) {
    p = ro + s * rd;
    h = p.y - WaveHt (p);
    if (h < 0.) break;
    sLo = s;
    s += max (0.2, h) + 0.005 * s;
    if (s > dstFar) break;
  }
  if (h < 0.) {
    sHi = s;
    for (int j = 0; j < 7; j ++) {
      s = 0.5 * (sLo + sHi);
      p = ro + s * rd;
      h = step (0., p.y - WaveHt (p));
      sLo += h * (s - sLo);
      sHi += (1. - h) * (s - sHi);
    }
    dHit = sHi;
  }
  return dHit;
}

float WaveOutRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float dHit, h, s, sLo, sHi;
  s = 0.;
  sLo = 0.;
  dHit = dstFar;
  ro.y *= -1.;
  rd.y *= -1.;
  for (int j = 0; j < 150; j ++) {
    p = ro + s * rd;
    h = p.y + WaveHt (p);
    if (h < 0.) break;
    sLo = s;
    s += max (0.2, h) + 0.005 * s;
    if (s > dstFar) break;
  }
  if (h < 0.) {
    sHi = s;
    for (int j = 0; j < 7; j ++) {
      s = 0.5 * (sLo + sHi);
      p = ro + s * rd;
      h = step (0., p.y + WaveHt (p));
      sLo += h * (s - sLo);
      sHi += (1. - h) * (s - sHi);
    }
    dHit = sHi;
  }
  return dHit;
}

vec3 WaveNf (vec3 p, float d)
{
  vec2 e = vec2 (max (0.1, 5e-5 * d * d), 0.);
  float h = WaveHt (p);
  return normalize (vec3 (h - WaveHt (p + e.xyy), e.x, h - WaveHt (p + e.yyx)));
}

float ObjDf (vec3 p)
{
  vec3 q;
  float dHit, d;
  dHit = dstFar;
  q = p;
  q -= ballPos;
  q *= ballMat;
  d = PrSphDf (q, 2.);
  if (d < dHit) { dHit = d;  idObj = 1;  qHit = q; }
  q.y -= 3.;
  d = PrCylDf (q.xzy, 0.05, 1.);
  if (d < dHit) { dHit = d;  idObj = 2;  qHit = q; }
  q.y -= 1.3;
  d = PrCylDf (q.xzy, 0.15, 0.3);
  if (d < dHit) { dHit = d;  idObj = 3;  qHit = q; }
  return dHit;
}

float ObjRay (in vec3 ro, in vec3 rd)
{
  float d, dHit;
  dHit = 0.;
  for (int j = 0; j < 150; j ++) {
    d = ObjDf (ro + dHit * rd);
    dHit += d;
    if (d < 0.002 || dHit > dstFar) break;
  }
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  const vec3 e = vec3 (0.001, -0.001, 0.);
  vec4 v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy),
     ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

vec3 ObjCol (vec3 n)
{
  vec3 col;
  col = vec3 (0.);
  if (idObj == 1) {
    col = vec3 (1., 0.01, 0.);
    if (abs (qHit.y) < 0.21) col =
       (mod (floor (7. * (atan (qHit.x, qHit.z) / pi + 1.)), 2.) == 0.) ?
       vec3 (1., 0.8, 0.08) : vec3 (0.04);
    else if (qHit.y > 1.93) col = vec3 (0.15, 0.05, 0.);
    else if (abs (qHit.y) < 0.25) col = vec3 (1., 0.8, 0.08);
    else if (abs (abs (qHit.y) - 0.55) < 0.05) col = vec3 (1.);
    else if (abs (abs (qHit.y) - 0.65) < 0.05) col = vec3 (0.04);
    if (qHit.y < 0.) col = mix (col, vec3 (0.05, 0.2, 0.05), 
       min (- 2. * qHit.y, 0.9));
  } else if (idObj == 2) {
    col = vec3 (0.6, 0.4, 0.2);
  } else if (idObj == 3) {
    if (abs (qHit.y) < 0.2) 
       col = vec3 (0., 1., 0.) * (2. + 1.5 * cos (10. * tCur));
    else col = vec3 (0.6, 0.4, 0.2);
  }
  return col;
}

float ObjSShadow (vec3 ro, vec3 rd)
{
  float sh, d, h;
  sh = 1.;
  d = 0.1;
  for (int j = 0; j < 40; j ++) {
    h = ObjDf (ro + rd * d);
    sh = min (sh, 20. * h / d);
    d += 0.1;
    if (h < 0.001) break;
  }
  return clamp (sh, 0., 1.);
}

vec3 ObjRender (vec3 ro, vec3 rd)
{
  vec3 col, vn;
  float dif, bk, sh, cc;
  int idObjT;
  idObjT = idObj;
  vn = ObjNf (ro);
  idObj = idObjT;
  if (idObj == 1) {
    vn = VaryNf (20. * qHit, vn, 0.3);
    if (qHit.y < 0.) vn = mix (vn, VaryNf (12. * qHit, vn, 2.),
      min (- 5. * qHit.y, 1.));
  }
  col = ObjCol (rd);
  cc = 1. - smoothstep (0.3, 0.6, fCloud);
  sh = ObjSShadow (ro, sunDir);
  bk = max (dot (vn, - normalize (vec3 (sunDir.x, 0., sunDir.z))), 0.);
  dif = max (dot (vn, sunDir), 0.);
  return col * (0.2 + 0.1 * bk + max (0., dif) * (0.7 + 0.3 * cc * sh)) + 
     0.3 * cc * sh * sunCol * pow (max (0., dot (sunDir, reflect (rd, vn))), 32.);
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col, vn, rdd, refCol, uwatCol;
  float dstHit, dstWat, dif, bk, sh, foamFac;
  const float eta = 0.75, att = 0.5;
  int idObjT;
  bool doReflect;
  if (ro.y > WaveHt (ro)) {
    dstWat = WaveRay (ro, rd);
    idObj = -1;
    dstHit = ObjRay (ro, rd);
    if (idObj < 0) dstHit = dstFar;
    doReflect = (dstWat < dstFar && dstWat < dstHit);
    if (doReflect) {
      ro += rd * dstWat;
      vn = WaveNf (ro, dstWat);
      rdd = rd;
      rd = reflect (rd, vn);
      idObj = -1;
      dstHit = ObjRay (ro, rd);
      if (idObj < 0) dstHit = dstFar;
    }
    col = (dstHit < dstFar) ? ObjRender (ro + rd * dstHit, rd) :
       SkyGrndCol (ro, rd);
    if (doReflect) {
      refCol = col;
      rd = refract (rdd, vn, eta);
      idObj = -1;
      dstHit = ObjRay (ro, rd);
      if (idObj < 0) dstHit = dstFar;
      col = (dstHit < dstFar) ? ObjRender (ro + rd * dstHit, rd) *
         exp (- att * dstHit) : SeaFloorCol (rd);
      col = mix (col, 0.8 * refCol, pow (1. - abs (dot (rdd, vn)), 5.));
      foamFac = pow (clamp (WaveHt (ro) +
         0.004 * Fbm3 (256. * ro) - 0.65, 0., 1.), 8.);
      col = mix (col, vec3 (1.), foamFac);
    }
  } else {
    uwatCol = vec3 (0., 0.05, 0.05) +
       step (0.4, Fbm1 (20. * tCur)) * vec3 (0.02, 0.02, 0.03);
    col = uwatCol;
    dstWat = WaveOutRay (ro, rd);
    idObj = -1;
    dstHit = ObjRay (ro, rd);
    if (idObj < 0) dstHit = dstFar;
    if (dstWat < dstFar && dstWat < dstHit) {
      ro += rd * dstWat;
      vn = - WaveNf (ro, dstWat);
      rdd = refract (rd, vn, 1. / eta);
      if (length (rdd) > 0.) rd = rdd;
      else rd = reflect (rd, vn);
      idObj = -1;
      dstHit = ObjRay (ro, rd);
      if (idObj < 0) dstHit = dstFar;
      if (dstHit < dstFar) col = 0.9 * ObjRender (ro + rd * dstHit, rd);
      else if (rd.y > 0.) col = mix (uwatCol, 0.9 * SkyGrndCol (ro, rd),
         exp (- 0.07 * att * dstWat));
    } else if (dstHit < dstFar) col = mix (uwatCol,
       ObjRender (ro + rd * dstHit, rd), exp (- 0.07 * att * dstHit));
  }
  return col;
}

void BallPM ()
{
  const vec3 e = vec3 (1., 0., 0.);
  float h[5], b;
  ballPos = vec3 (0., 0., 0.);
  h[0] = WaveHt (ballPos);
  h[1] = WaveHt (ballPos + e.yyx);  h[2] = WaveHt (ballPos - e.yyx);
  h[3] = WaveHt (ballPos + e);  h[4] = WaveHt (ballPos - e);
  ballPos.y = 0.5 + (2. * h[0] + h[1] + h[2] + h[3] + h[4]) / 9.;
  b = (h[1] - h[2]) / (4. * e.x);
  ballMat[2] = normalize (vec3 (0., b, 1.));
  b = (h[3] - h[4]) / (4. * e.x);
  ballMat[1] = normalize (cross (ballMat[2], vec3 (1., b, 0.)));
  ballMat[0] = cross (ballMat[1], ballMat[2]);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = 2. * fragCoord.xy / iResolution.xy - 1.;
  uv.x *= iResolution.x / iResolution.y;
  tCur = iGlobalTime;
#ifdef MOUSE
  vec4 mPtr = iMouse;
  mPtr.xy = mPtr.xy / iResolution.xy - 0.5;
#endif
  mat3 vuMat;
  vec3 col, ro, rd;
  vec2 vEl, vAz;
  float el, az, zmFac, a, tPer, tSeq;
  cloudDisp = 10. * tCur * vec3 (1., 0., 1.);
  waterDisp = 0.5 * tCur * vec3 (-1., 0., 1.);
  sunDir = normalize (vec3 (0.2, 0.5, 0.5));
  sunCol = vec3 (1., 0.4, 0.3) + vec3 (0., 0.5, 0.2) * sunDir.y;
  fCloud = 0.5 + 0.2 * sin (0.022 * 2. * pi * tCur);
  zmFac = 3.5;
  tPer = 35.;
#ifdef MOUSE
  if (mPtr.z <= 0.) {
    az = 0.01 * tCur;
    el = 0.2 * pi;
    tSeq = mod (tCur, tPer);
    if (mod (floor (tCur / tPer), 2.) == 0.) {
      a = SmoothBump (10., 30., 5., tSeq);
      zmFac -= 0.1 * a;
      el -= 0.19 * pi * a;
    } else {
      a = SmoothBump (8., 26., 8., tSeq);
      zmFac -= 0.05 * a;
      el -= 0.55 * pi * a;
    }
  }
  else
  {
    az = 1.1 * pi * mPtr.x;
    el = 0.02 * pi - 0.7 * pi * mPtr.y;
  }
#else
  {
    az = 0.01 * tCur;
    el = 0.2 * pi;
    tSeq = mod (tCur, tPer);
    if (mod (floor (tCur / tPer), 2.) == 0.) {
      a = SmoothBump (10., 30., 5., tSeq);
      zmFac -= 0.1 * a;
      el -= 0.19 * pi * a;
    } else {
      a = SmoothBump (8., 26., 8., tSeq);
      zmFac -= 0.05 * a;
      el -= 0.55 * pi * a;
    }
  }
#endif
  vEl = vec2 (cos (el), sin (el));
  vAz = vec2 (cos (az), sin (az));
  rd = normalize (vec3 (uv, zmFac));
  vuMat = mat3 (1., 0., 0., 0., vEl.x, - vEl.y, 0., vEl.y, vEl.x) *
     mat3 (vAz.x, 0., vAz.y, 0., 1., 0., - vAz.y, 0., vAz.x);
  rd = rd * vuMat;
  ro = vec3 (0., 0., -20.) * vuMat;
  ro.y += 2.;
  BallPM ();
  col = ShowScene (ro, rd);
  fragColor = vec4 (sqrt (clamp (col, 0., 1.)), 1.);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
