#version 450
// Ocean Structure -  dr2 - 2017-11-01
// https://www.shadertoy.com/view/ltBczc

// Abstract construction with fire, smoke, reflections and aurora.
// Look around using mouse; mouse to lower-right corner for daylight view.

// "Ocean Structure" by dr2 - 2017
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

#define REAL_WAVE 0   // real (=1) or fake (=0, less work) water waves
#define N_REFL    3   // number of reflections (1-4, 0 = none)

float PrBoxDf (vec3 p, vec3 b);
float PrSphDf (vec3 p, float s);
float PrSphAnDf (vec3 p, float r, float w);
float PrCylDf (vec3 p, float r, float h);
float PrTorusDf (vec3 p, float ri, float rc);
float PrCapsDf (vec3 p, float r, float h);
float PrCapsAnDf (vec3 p, float r, float w, float h);
float PrFlatCylDf (vec3 p, float rhi, float rlo, float h);
float Noisefv2 (vec2 p);
float Noisefv3 (vec3 p);
float Fbm1 (float p);
float Fbm2 (vec2 p);
float Fbm3 (vec3 p);
vec3 VaryNf (vec3 p, vec3 n, float f);
vec3 HsvToRgb (vec3 c);
vec2 Rot2D (vec2 q, float a);

vec3 qHit, sunDir, smkPos;
float dstFar, tCur, smkRadEx, smkRadIn, smkPhs, smkHt, tWav;
int idObj;
bool isNite;
const int idBase = 1, idPlat = 2, isShel = 3, idFrm = 4, idDway = 5,
   idTwr = 6, idBrg = 7, idBrCab = 8, idRdw = 9;
const float pi = 3.14159;

float BridgeDf (vec3 p, float dMin)
{
  float cbRad = 0.06;
  vec3 q, qq;
  float d, cLen, wRd;
  wRd = 1.;
  q = p;  q.x = abs (q.x) - wRd;  q.y -= -0.3;
  d = PrBoxDf (q, vec3 (0.15, 5., 0.15));
  q = p;  q.y -= 4.85;
  d = min (d, PrBoxDf (q, vec3 (wRd + 0.15, 0.15, 0.15)));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idBrg; }
  qq = p;  qq.x = abs (qq.x) - wRd + 0.07;  qq.z = abs (qq.z);
  q = qq;  q.y -= 4.92;
  q.yz = Rot2D (q.yz, -0.1 * pi);
  q.z -= 9.5;
  d = PrCylDf (q, cbRad, 9.5);
  q = qq;  q.y -= 4.82;
  q.yz = Rot2D (q.yz, -0.15 * pi);
  q.z -= 6.4;
  d = min (d, PrCylDf (q, cbRad, 6.4));
  q = qq;  q.y -= 4.77;
  q.yz = Rot2D (q.yz, -0.26 * pi);
  q.z -= 4.;
  d = min (d, PrCylDf (q, cbRad, 4.));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idBrCab; }
  return dMin;
}

float CentStrucDf (vec3 p, float dMin, float ar)
{
  float cRad = 6., cLen = 8., ww = 0.03, wThk = 0.05,
     doorHt = 1.6, doorWd = 1.4;
  vec3 q;
  vec2 qo;
  float d, dd;
  q = p;  q.y -= -1.05;
  d = PrCylDf (q.xzy, 8.5, 0.15);
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idPlat; }
  d = PrTorusDf (vec3 (q.xz, abs (abs (q.y - 1.1) - 0.4) - 0.2), 0.03, 8.5);
  q.xz = Rot2D (q.xz, 2. * pi * (floor (16. * ar) + 0.5) / 16.);
  q.xy -= vec2 (-8.5, 0.9);
  d = min (d, PrCylDf (q.xzy, 0.05, 0.82));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idBrCab; }
  q = p;
  qo = Rot2D (q.xz, 2. * pi * (floor (4. * ar) + 0.5) / 4.);
  q.xz = qo;  q.y -= cLen + 1.2 * doorHt - 9.;
  dd = PrFlatCylDf (q.yzx, doorHt, doorWd, 0.);
  q = p;  q.y -= cLen - 9.;
  d = max (max (max (PrCapsAnDf (q.xzy, cRad, wThk, cLen), - q.y), q.y - 1.635 * cLen), - dd);
  if (d < dMin) { dMin = d;  qHit = q;  idObj = isShel; }
  q.xz = Rot2D (q.xz, 2. * pi * (floor (8. * ar) + 0.5) / 8.);
  d = max (max (max (PrCapsAnDf (q.xzy, cRad, 0.2, cLen),
     min (abs (mod (q.y, 2.) - 1.) - 0.1,
     dot (vec2 (q.x, abs (q.z)), vec2 (sin (0.04 * 2. * pi / 16.), cos (0.04 * 2. * pi / 16.))))),
     - q.y), - dd);
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idFrm; }
  q = p;  q.xz = qo;  q.xy -= vec2 (-0.98 * cRad, cLen + 1.2 * doorHt - 9.);
  d = max (max (max (PrFlatCylDf (q.yzx, doorHt, doorWd, 0.1 * cRad),
     - PrFlatCylDf (q.yzx, doorHt - ww, doorWd - ww, 0.)),
     - (q.y + 2. * doorHt - ww - wThk)), - (q.y + 1.2 * doorHt));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idDway; }
  return dMin;
}

float CornStrucDf (vec3 p, float dMin)
{
  vec3 q;
  float d, a;
  q = p;  q.y -= -1.2;
  d = PrCylDf (q.xzy, 3.2, 0.15);
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idPlat; }
  q = p;  q.y -= 1.;
  d = max (PrCapsAnDf  (q.xzy, 2.5, 0.1, 3.), -2.2 - q.y);
  q = p;  q.y -= 7.;
  d = min (d, max (PrCapsDf (q.xzy, 0.7, 2.), -1. - q.y));
  q = p;
  a = (length (q.xz) > 0.) ? atan (q.z, - q.x) / (2. * pi) : 0.;
  q.xz = Rot2D (q.xz, 2. * pi * (floor (4. * a + 0.5) / 4.));
  d = max (d, - PrFlatCylDf (q.yzx, 1.5, 1., 0.));
  q = p;
  q.xz = Rot2D (q.xz, 2. * pi * (floor (16. * a) + 0.5) / 16.);
  q.y -= 4.3;
  d = max (d, - (length (max (abs (q.yz) - vec2 (0.6, 0.08), 0.)) - 0.2));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idTwr; }
  return dMin;
}

float ObjDf (vec3 p)
{
  vec3 q;
  float d, dMin, hs, wRd, ar;
  dMin = dstFar;
  hs = 5.;
  q = p;  q.xz = abs (q.xz) - 4.;
  d = max (PrSphAnDf (q, 4.85, 0.15), - min (3.9 - q.y, q.y));
  q.y -= 0.5;
  d = max (d, 2.2 - min (length (q.yz), length (q.xy)));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idBase; }
  q = p;  q.xz = abs (q.xz) - 20.;
  d = max (PrSphAnDf (q, 4.85, 0.15), - min (3.9 - q.y, q.y));
  q.y -= 0.5;
  d = max (d, 2.2 - min (length (q.yz), length (q.xy)));
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idBase; }
  wRd = 1.;
  q = p;  q.xz = abs (q.xz) - 20.7 + wRd;
  d = max (max (length (max (q.xz - wRd, 0.)) - 0.3,
     - (length (max (q.xz + wRd, 0.)) - 0.3)), abs (q.y - hs + 1.) - 0.1);
  if (d < dMin) { dMin = d;  qHit = q;  idObj = idRdw; }
  q = p;
  ar = (length (p.xz) > 0.) ? atan (p.z, - p.x) / (2. * pi) : 0.;
  q.y -= hs;
  dMin = CentStrucDf (q, dMin, ar);
  q = p;  q.y -= hs;  q.xz = abs (q.xz) - vec2 (20.);
  dMin = CornStrucDf (q, dMin);
  q = p;  q.y -= hs;
  q.xz = Rot2D (q.xz, 2. * pi * (floor (4. * ar + 0.5) / 4.));
  q.x += 20.;
  dMin = BridgeDf (q, dMin);
  return dMin;
}

float ObjRay (vec3 ro, vec3 rd)
{
  float dHit, d;
  dHit = 0.;
  for (int j = 0; j < 120; j ++) {
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
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy), ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
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
  p *= 0.3;
  vn.xz = 0.4 * (WaveHt (p.xz) - vec2 (WaveHt (p.xz + e.xy),  WaveHt (p.xz + e.yx)));
  vn.y = e.x;
  return normalize (vn);
}

#if REAL_WAVE

float WaveRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float dHit, h, s, sLo, sHi, f1, f2;
  dHit = dstFar;
  f1 = 0.4;
  f2 = 0.3;
  s = max (- (ro.y - 1.2 * f1) / rd.y, 0.);
  sLo = s;
  for (int j = 0; j < 80; j ++) {
    p = ro + s * rd;
    h = p.y - f1 * WaveHt (f2 * p.xz);
    if (h < 0.) break;
    sLo = s;
    s += max (0.3, h) + 0.005 * s;
    if (s >= dstFar) break;
  }
  if (h < 0.) {
    sHi = s;
    for (int j = 0; j < 5; j ++) {
      s = 0.5 * (sLo + sHi);
      p = ro + s * rd;
      h = step (0., p.y - f1 * WaveHt (f2 * p.xz));
      sLo += h * (s - sLo);
      sHi += (1. - h) * (s - sHi);
    }
    dHit = sHi;
  }
  return dHit;
}

#endif

float SmokeDens (vec3 p)
{
  mat2 rMat;
  vec3 q, u;
  float f;
  f = PrTorusDf (p.xzy, smkRadIn, smkRadEx);
  if (f < 0.) {
    q = p.xzy / smkRadEx;
    u = normalize (vec3 (q.xy, 0.));
    q -= u;
    rMat = mat2 (vec2 (u.x, - u.y), u.yx);
    q.xy = rMat * q.xy;
    q.xz = Rot2D (q.xz, 2.5 * tCur);
    q.xy = q.xy * rMat;
    q += u;
    q.xy = Rot2D (q.xy, 0.1 * tCur);
    f = smoothstep (0., smkRadIn, - f) * Fbm3 (10. * q);
  } else f = 0.;
  return f;
}

vec4 SmokeCol (vec3 ro, vec3 rd, float dstObj)
{
  vec4 col4;
  vec3 q;
  float densFac, dens, d, h, sh;
  d = 0.;
  for (int j = 0; j < 150; j ++) {
    q = ro + d * rd;
    q.xz = abs (q.xz);
    q -= smkPos;
    h = PrTorusDf (q.xzy, smkRadIn, smkRadEx);
    d += h;
    if (h < 0.001 || d > dstFar) break;
  }
  col4 = vec4 (0.);
  if (d < min (dstObj, dstFar)) {
    densFac = 1.45 * (1.08 - pow (smkPhs, 1.5));
    for (int j = 0; j < 150; j ++) {
      q = ro + d * rd;
      q.xz = abs (q.xz);
      q -= smkPos;
      dens = SmokeDens (q);
      sh = 0.3 + 0.7 * smoothstep (-0.3, 0.1, dens - SmokeDens (q + 0.1 * sunDir));
      col4 += densFac * dens * (1. - col4.w) * vec4 (sh * vec3 (0.9) - col4.rgb, 0.3);
      d += 2.2 * smkRadEx / 150.;
      if (col4.w > 0.99 || d > dstFar) break;
    }
  }
  if (isNite) col4.rgb *= vec3 (0.3, 0.4, 0.3);
  return col4;
}

vec4 ObjCol (vec3 n)
{
  vec4 col;
  if (idObj == idBase) col = vec4 (0.3, 0.4, 0.1, 0.1);
  else if (idObj == idPlat) col = vec4 (0.4, 0.4, 0.3, 0.1);
  else if (idObj == isShel) col = vec4 (0.5, 0.5, 0.5, 0.3);
  else if (idObj == idFrm) col = vec4 (0.8, 0.8, 0.9, 0.5);
  else if (idObj == idDway) col = vec4 (0.7, 0.8, 0.7, 0.3);
  else if (idObj == idTwr) col = vec4 (0.7, 0.7, 0.6, 0.3);
  else if (idObj == idBrg) col = vec4 (1., 0.3, 0.3, 0.3);
  else if (idObj == idBrCab) col = vec4 (0.9, 0.9, 1., 0.5);
  else if (idObj == idRdw) col = vec4 (0.4, 0.3, 0.3, 0.1);
  return col;
}

vec4 AurCol (vec3 ro, vec3 rd)
{
  vec4 col, mCol;
  vec3 p, dp;
  float ar;
  dp = rd / rd.y;
  p = ro + (40. - ro.y) * dp;
  col = vec4 (0.);
  mCol = vec4 (0.);
  tWav = 0.05 * tCur;
  for (float ns = 0.; ns < 50.; ns ++) {
    p += dp;
    ar = 0.05 - clamp (0.06 * WaveHt (0.01 * p.xz), 0., 0.04);
    mCol = mix (mCol, ar * vec4 (HsvToRgb (vec3 (0.34 + 0.007 * ns, 1., 1. - 0.02 * ns)), 1.), 0.5);
    col += mCol;
  }
  return col;
}

vec3 NtSkyCol (vec3 rd)
{
  vec3 rds;
  rds = floor (2000. * rd);
  rds = 0.00015 * rds + 0.1 * Noisefv3 (0.0005 * rds.yzx);
  for (int j = 0; j < 19; j ++) rds = abs (rds) / dot (rds, rds) - 0.9;
  return 0.3 * vec3 (1., 1., 0.9) * min (1., 0.5e-3 * pow (min (6., length (rds)), 5.));
}

vec3 BgCol (vec3 ro, vec3 rd)
{
  vec3 col;
  float f, a;
  if (rd.y > 0.) {
    a = atan (rd.x, - rd.z);
    if (rd.y < 0.03 * Fbm1 (32. * a) + 0.005)
       col = (isNite ? vec3 (0.07, 0.1, 0.07) : vec3 (0.4, 0.5, 0.7)) * (1. - 0.3 * Fbm2 (128. * vec2 (a, rd.y)));
    else {
      if (isNite) {
        vec4 aCol = AurCol (ro, rd);
        col = (1. - 0.5 * aCol.a) * NtSkyCol (rd) + 0.6 * aCol.rgb;
      } else {
        ro.xz += 2. * tCur;
        col = vec3 (0.2, 0.3, 0.6) + 0.2 * (1. - max (rd.y, 0.)) +
           0.1 * pow (max (dot (rd, sunDir), 0.), 16.);
        f = Fbm2 (0.02 * (ro.xz + rd.xz * (100. - ro.y) / max (rd.y, 0.01)));
        col = mix (col, vec3 (1.), 0.2 + 0.8 * f * rd.y);
      }
    }
  } else {
    col = vec3 (0.6, 0.5, 0.3);
    if (- ro.y / rd.y < dstFar) {
      ro += - (ro.y / rd.y) * rd;
      col *= 1.1 - 0.2 * Noisefv2 (30. * ro.xz);
    }
    col = mix (col, 0.9 * (vec3 (0.4, 0.2, 0.1) + 0.2) + 0.1, pow (1. + rd.y, 5.));
  }
  return col;
}

vec4 GlowCol (vec3 ro, vec3 rd, float dstObj)
{
  vec3 gloDir;
  float gloDist, wGlow, s;
  wGlow = 0.;
  for (float j = 0.; j < 4.; j ++) {
    gloDir = vec3 (20., 9.3, 20.) * (1. - 2. * vec3 (floor (j / 2.), 0., mod (j, 2.))) - ro;
    gloDist = length (gloDir);
    s = dot (rd, normalize (gloDir));
    if (s > 0. && gloDist < dstObj) wGlow += 1. - smoothstep (1., 2., sqrt (1. - s * s) * gloDist);
  }
  gloDir = vec3 (0., 15.5, 0.) - ro;
  gloDist = length (gloDir);
  s = dot (rd, normalize (gloDir));
  if (s > 0. && gloDist < dstObj) wGlow += 1. - smoothstep (2., 3., sqrt (1. - s * s) * gloDist);
  return (0.6 + 0.4 * sin (0.3 * 2. * pi * tCur)) * clamp (wGlow, 0., 1.) * vec4 (1., 0.5, 0.3, 1.);
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec4 objCol, smkCol, smkColR, smkColW, glwCol, glwColR, glwColW;
  vec3 col, vn;
  float dstObj, dstWat, reflCol;
  bool wRefl;
  col = vec3 (0.2, 0.2, 0.);
  wRefl = false;
  dstObj = ObjRay (ro, rd);
  smkCol = SmokeCol (ro, rd, dstObj);
  glwCol = GlowCol (ro, rd, dstObj);
  glwColR = vec4 (0.);
  glwColW = vec4 (0.);
  smkColR = vec4 (0.);
  smkColW = vec4 (0.);
  tWav = 0.4 * tCur;
  reflCol = 1.;
  if (N_REFL >= 2 && dstObj < dstFar && idObj == isShel) {
    if (length (qHit.xz) > 6. || qHit.y >= 8.8) {
      ro += dstObj * rd;
      vn = ObjNf (ro);
      rd = reflect (rd, vn);
      ro += 0.01 * rd;
      dstObj = ObjRay (ro, rd);
      smkColR = SmokeCol (ro, rd, dstObj);
      glwColR = GlowCol (ro, rd, dstObj);
      reflCol *= 0.9;
    }
  }
  if (N_REFL >= 1 && rd.y < 0.) {
#if REAL_WAVE
    dstWat = WaveRay (ro, rd);
#else
    dstWat = - ro.y / rd.y;
#endif
    if (dstWat < min (dstObj, dstFar)) {
      wRefl = true;
      ro += dstWat * rd;
      vn = WaveNf (ro, dstWat);
      rd = reflect (rd, vn);
      ro += 0.01 * rd;
      dstObj = ObjRay (ro, rd);
      smkColW = SmokeCol (ro, rd, dstObj);
      glwColW = GlowCol (ro, rd, dstObj);
      if (N_REFL >= 3 && dstObj < dstFar && idObj == isShel) {
        ro += dstObj * rd;
        vn = ObjNf (ro);
        rd = reflect (rd, vn);
        if (N_REFL == 4) {
          ro += 0.01 * rd;
          dstObj = ObjRay (ro, rd);
        } else {
          dstObj = dstFar;
        }
      }
      reflCol *= 0.7;
    }
  }
  if (dstObj < dstFar) {
    ro += dstObj * rd;
    vn = ObjNf (ro);
    if (idObj == idRdw) vn = VaryNf (5. * qHit, vn, 1.);
    else if (idObj == idBase) vn = VaryNf (2. * floor (16. * qHit), vn, 2.);
    objCol = ObjCol (vn);
    if (isNite) col = objCol.rgb * vec3 (0.3, 0.35, 0.3) * (0.2 + 0.8 * max (0.,vn.y));
    else col = objCol.rgb * (0.2 + 0.8 * max (0., max (dot (vn, sunDir), 0.))) + 
       objCol.a * pow (max (dot (normalize (sunDir - rd), vn), 0.), 64.);
  } else if (rd.y > 0.) {
    col = BgCol (ro, rd);
  } else {
#if N_REFL == 0
    dstWat = - ro.y / rd.y;
#endif
    col = BgCol (ro + dstWat * rd, reflect (rd, vec3 (0., 1., 0.)));
    reflCol = 0.7;
  }
  col = clamp (reflCol * col, 0., 1.);
  col = mix (col, glwCol.rgb, glwCol.a);
  col = mix (col, glwColR.rgb, glwColR.a);
  col = mix (col, smkCol.rgb, smkCol.a);
  col = mix (col, smkColR.rgb, smkColR.a);
  if (wRefl) {
    col = mix (col, reflCol * glwColW.rgb, glwColW.a);
    col = mix (col, reflCol * smkColW.rgb, smkColW.a);
    col = mix (mix (vec3 (0., 0.1, 0.), vec3 (0., 0.05, 0.05),
       smoothstep (0.4, 0.6, Fbm2 (0.5 * ro.xz))), col, 1. - pow (abs (rd.y), 4.));
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
  vec2 uv, ori, ca, sa;
  float el, az;
  uv = 2. * fragCoord.xy / iResolution.xy - 1.;
  uv.x *= iResolution.x / iResolution.y;
  tCur = iGlobalTime;
#ifdef MOUSE
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / iResolution.xy - 0.5;
#endif
  smkPhs = mod (0.15 * tCur + 0.3, 1.);
  smkPos = vec3 (20., 9. + 10. * smkPhs, 20.);
  smkRadIn = 0.6 * (0.1 + 0.9 * smoothstep (0.01, 0.1, smkPhs));
  smkRadEx = smkRadIn + 2.5;
  dstFar = 140.;
  isNite = true;
  az = 0.33 * pi;
  el = -0.016 * pi;
#ifdef MOUSE
  if (mPtr.z > 0.) {
    if (mPtr.x > 0.45 && mPtr.y < -0.45) isNite = false;
    else {
      az += pi * mPtr.x;
      el += 0.05 * pi * mPtr.y;
    }
  } else {
    az += 0.002 * pi * tCur;
    el += 0.01 * pi * sin (0.01 * pi * tCur);
  }
#else
  az += 0.002 * pi * tCur;
  el += 0.01 * pi * sin (0.01 * pi * tCur);
#endif
  el = clamp (el, -0.4 * pi, -0.01 * pi);
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  ro = vuMat * vec3 (0., 10., -100.);
  rd = vuMat * normalize (vec3 (uv, 4.2));
  sunDir = vuMat * normalize (vec3 (1., 1., -1.));
  fragColor = vec4 (ShowScene (ro, rd), 1.);
}

float PrBoxDf (vec3 p, vec3 b)
{
  vec3 d;
  d = abs (p) - b;
  return min (max (d.x, max (d.y, d.z)), 0.) + length (max (d, 0.));
}

float PrSphDf (vec3 p, float s)
{
  return length (p) - s;
}

float PrSphAnDf (vec3 p, float r, float w)
{
  return abs (length (p) - r) - w;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrCapsDf (vec3 p, float r, float h)
{
  return length (p - vec3 (0., 0., h * clamp (p.z / h, -1., 1.))) - r;
}

float PrCapsAnDf (vec3 p, float r, float w, float h)
{
  p.z = abs (p.z);
  return max (length (p - vec3 (0., 0., min (p.z, h + w))) - r,
     - length (p - vec3 (0., 0., min (p.z, h - w))) + r) - w;
}

float PrFlatCylDf (vec3 p, float rhi, float rlo, float h)
{
  float d;
  d = length (p.xy - vec2 (rhi * clamp (p.x / rhi, -1., 1.), 0.)) - rlo;
  if (h > 0.) d = max (d, abs (p.z) - h);
  return d;
}

float PrTorusDf (vec3 p, float ri, float rc)
{
  return length (vec2 (length (p.xy) - rc, p.z)) - ri;
}

const float cHashM = 43758.54;

vec2 Hashv2f (float p)
{
  return fract (sin (p + vec2 (0., 1.)) * cHashM);
}

vec2 Hashv2v2 (vec2 p)
{
  vec2 cHashVA2 = vec2 (37., 39.);
  return fract (sin (vec2 (dot (p, cHashVA2), dot (p + vec2 (1., 0.), cHashVA2))) * cHashM);
}

vec4 Hashv4v3 (vec3 p)
{
  vec3 cHashVA3 = vec3 (37., 39., 41.);
  vec2 e = vec2 (1., 0.);
  return fract (sin (vec4 (dot (p + e.yyy, cHashVA3), dot (p + e.xyy, cHashVA3),
     dot (p + e.yxy, cHashVA3), dot (p + e.xxy, cHashVA3))) * cHashM);
}

float Noiseff (float p)
{
  vec2 t;
  float ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp = fp * fp * (3. - 2. * fp);
  t = Hashv2f (ip);
  return mix (t.x, t.y, fp);
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

float Noisefv3 (vec3 p)
{
  vec4 t;
  vec3 ip, fp;
  ip = floor (p);
  fp = fract (p);
  fp *= fp * (3. - 2. * fp);
  t = mix (Hashv4v3 (ip), Hashv4v3 (ip + vec3 (0., 0., 1.)), fp.z);
  return mix (mix (t.x, t.y, fp.x), mix (t.z, t.w, fp.x), fp.y);
}

float Fbm1 (float p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * Noiseff (p);
    a *= 0.5;
    p *= 2.;
  }
  return f * (1. / 1.9375);
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

float Fbm3 (vec3 p)
{
  float f, a;
  f = 0.;
  a = 1.;
  for (int i = 0; i < 5; i ++) {
    f += a * Noisefv3 (p);
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

vec3 HsvToRgb (vec3 c)
{
  vec3 p;
  p = abs (fract (c.xxx + vec3 (1., 2./3., 1./3.)) * 6. - 3.);
  return c.z * mix (vec3 (1.), clamp (p - 1., 0., 1.), c.y);
}

vec2 Rot2D (vec2 q, float a)
{
  return q * cos (a) + q.yx * sin (a) * vec2 (-1., 1.);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
