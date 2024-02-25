#version 450
// River Flight -  dr2 - 2014-11-06
// https://www.shadertoy.com/view/4sSXDG

// Fasten your seatbelts for a wild ride.

// "River Flight" by dr2 - 2014
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

float Noisefv3 (vec3 p)
{
  vec3 i = floor (p);
  vec3 f = fract (p);
  f = f * f * (3. - 2. * f);
  float q = dot (i, cHashA3);
  vec4 t1 = Hashv4f (q);
  vec4 t2 = Hashv4f (q + cHashA3.z);
  return mix (mix (mix (t1.x, t1.y, f.x), mix (t1.z, t1.w, f.x), f.y),
     mix (mix (t2.x, t2.y, f.x), mix (t2.z, t2.w, f.x), f.y), f.z);
}

vec3 Noisev3v2 (vec2 p)
{
  vec2 i = floor (p);
  vec2 f = fract (p);
  vec2 ff = f * f;
  vec2 u = ff * (3. - 2. * f);
  vec2 uu = 30. * ff * (ff - 2. * f + 1.);
  vec4 h = Hashv4f (dot (i, cHashA3.xy));
  return vec3 (h.x + (h.y - h.x) * u.x + (h.z - h.x) * u.y +
     (h.x - h.y - h.z + h.w) * u.x * u.y, uu * (vec2 (h.y - h.x, h.z - h.x) +
     (h.x - h.y - h.z + h.w) * u.yx));
}

float SmoothMin (float a, float b, float r)
{
  float h = clamp (0.5 + 0.5 * (b - a) / r, 0., 1.);
  return mix (b, a, h) - r * h * (1. - h);
}

float SmoothBump (float lo, float hi, float w, float x)
{
  return (1. - smoothstep (hi - w, hi + w, x)) * smoothstep (lo - w, lo + w, x);
}

float PrCapsDf (vec3 p, vec2 b)
{
  return length (p - vec3 (0., 0., b.x * clamp (p.z / b.x, -1., 1.))) - b.y;
}

float PrCylDf (vec3 p, vec2 b)
{
  return max (length (p.xy) - b.x, abs (p.z) - b.y);
}

float PrConeDf (vec3 p, vec3 b)
{
  return max (dot (vec2 (length (p.xy), p.z), b.xy), abs (p.z) - b.z);
}

int idObj;
mat3 flyerMat;
vec3 flyerPos, engPos, qHit, qHitTransObj, sunDir, sunCol;
vec2 trkOffset;
float szFac, wSpan, tCur;
const float dstFar = 400.;
const float pi = 3.14159;

vec3 TrackPath (float t)
{
  return vec3 (24. * sin (0.035 * t) * sin (0.012 * t) * cos (0.01 * t) +
     19. * sin (0.0032 * t) + 100. * trkOffset.x, 0., t);
}

float GrndHt (vec2 p, int hiRes)
{
  const vec2 vRot = vec2 (1.4624, 1.6721);
  vec2 q = p * 0.06;
  float w = 0.75 * Noisefv2 (0.25 * q) + 0.15;
  w *= 36. * w;
  vec2 vyz = vec2 (0.);
  float ht = 0.;
  for (int j = 0; j < 10; j ++) {
    vec3 v = Noisev3v2 (q);
    vyz += v.yz;
    ht += w * v.x / (1. + dot (vyz, vyz));
    if (j == 4) {
      if (hiRes == 0) break;
    }
    w *= -0.37;      
    q *= mat2 (vRot.x, vRot.y, - vRot.y, vRot.x);
  }
  vec3 pt = TrackPath (p.y);
  pt.y -= 2.;
  float g = smoothstep (1.5, 4.5, sqrt (abs (p.x - pt.x)));
  return min (ht, pt.y * (1. - g) + ht * g);
}

vec3 GrndNf (vec3 p, float d)
{
  float ht = GrndHt (p.xz, 1);
  vec2 e = vec2 (max (0.01, 0.00001 * d * d), 0.);
  return normalize (vec3 (ht - GrndHt (p.xz + e.xy, 1), e.x,
     ht - GrndHt (p.xz + e.yx, 1)));
}

vec4 GrndCol (vec3 p, vec3 n)
{
  const vec3 gCol1 = vec3 (0.6, 0.7, 0.7), gCol2 = vec3 (0.2, 0.1, 0.1),
     gCol3 = vec3 (0.4, 0.3, 0.3), gCol4 = vec3 (0.1, 0.2, 0.1),
     gCol5 = vec3 (0.7, 0.7, 0.8), gCol6 = vec3 (0.05, 0.3, 0.03),
     gCol7 = vec3 (0.1, 0.08, 0.);
  vec2 q = p.xz;
  float f, d;
  float cSpec = 0.;
  f = 0.5 * (clamp (Noisefv2 (0.1 * q), 0., 1.) +
      0.8 * Noisefv2 (0.2 * q + 2.1 * n.xy + 2.2 * n.yz));
  vec3 col = f * mix (f * gCol1 + gCol2, f * gCol3 + gCol4, 0.65 * f);
  if (n.y < 0.5) {
    f = 0.4 * (Noisefv2 (0.4 * q + vec2 (0., 0.57 * p.y)) +
       0.5 * Noisefv2 (6. * q));
    d = 4. * (0.5 - n.y);
    col = mix (col, vec3 (f), clamp (d * d, 0.1, 1.));
    cSpec += 0.1;
  }
  if (p.y > 22.) {
    if (n.y > 0.25) {
      f = clamp (0.07 * (p.y - 22. - Noisefv2 (0.2 * q) * 15.), 0., 1.);
      col = mix (col, gCol5, f);
      cSpec += f;
    }
  } else {
    if (n.y > 0.45) {
      vec3 c = (n.y - 0.3) * (gCol6 * vec3 (Noisefv2 (0.4 * q),
         Noisefv2 (0.34 * q), Noisefv2 (0.38 * q)) + vec3 (0.02, 0.1, 0.02));
      col = mix (col, c, smoothstep (0.45, 0.65, n.y) *
         (1. - smoothstep (15., 22., p.y - 1.5 + 1.5 * Noisefv2 (0.2 * q))));
    }
    if (p.y < 0.65 && n.y > 0.4) {
      d = n.y - 0.4;
      col = mix (col, d * d + gCol7, 2. * clamp ((0.65 - p.y -
         0.35 * (Noisefv2 (0.4 * q) + 0.5 * Noisefv2 (0.8 * q) +
         0.25 * Noisefv2 (1.6 * q))), 0., 0.3));
      cSpec += 0.1;
    }
  }
  return vec4 (col, cSpec);
}

float GrndRay (vec3 ro, vec3 rd)
{
  vec3 p;
  float h, s, sLo, sHi;
  s = 0.;
  sLo = 0.;
  float dHit = dstFar;
  for (int j = 0; j < 150; j ++) {
    p = ro + s * rd;
    h = p.y - GrndHt (p.xz, 0);
    if (h < 0.) break;
    sLo = s;
    s += max (0.15, 0.4 * h) + 0.008 * s;
    if (s > dstFar) break;
  }
  if (h < 0.) {
    sHi = s;
    for (int j = 0; j < 10; j ++) {
      s = 0.5 * (sLo + sHi);
      p = ro + s * rd;
      h = step (0., p.y - GrndHt (p.xz, 0));
      sLo += h * (s - sLo);
      sHi += (1. - h) * (s - sHi);
    }
    dHit = sHi;
  }
  return dHit;
}

float WaterHt (vec3 p)
{
  p *= 0.1;
  float ht = 0.;
  const float wb = 1.414;
  float w = 0.2 * wb;
  for (int j = 0; j < 7; j ++) {
    w *= 0.5;
    p = wb * vec3 (p.y + p.z, p.z - p.y, 2. * p.x);
    ht += w * abs (Noisefv3 (p) - 0.5);
  }
  return ht;
}

vec3 WaterNf (vec3 p, float d)
{
  float ht = WaterHt (p);
  vec2 e = vec2 (max (0.01, 0.001 * d * d), 0.);
  return normalize (vec3 (ht - WaterHt (p + e.xyy), e.x, ht - WaterHt (p + e.yyx)));
}

vec3 SkyBg (vec3 rd)
{
  const vec3 sbCol = vec3 (0.15, 0.2, 0.65);
  vec3 col;
  col = sbCol + 0.2 * sunCol * pow (1. - max (rd.y, 0.), 5.);
  return col;
}

vec3 SkyCol (vec3 ro, vec3 rd)
{
  const float skyHt = 200.;
  vec3 col;
  float cloudFac;
  if (rd.y > 0.) {
    ro.x += 0.5 * tCur;
    vec2 p = 0.01 * (rd.xz * (skyHt - ro.y) / rd.y + ro.xz);
    float w = 0.65;
    float f = 0.;
    for (int j = 0; j < 4; j ++) {
      f += w * Noisefv2 (p);
      w *= 0.5;
      p *= 2.3;
    }
    cloudFac = clamp (5. * (f - 0.5) * rd.y - 0.1, 0., 1.);
  } else cloudFac = 0.;
  float s = max (dot (rd, sunDir), 0.);
  col = SkyBg (rd) + sunCol * (0.35 * pow (s, 6.) +
     0.65 * min (pow (s, 256.), 0.3));
  col = mix (col, vec3 (0.85), cloudFac);
  return col;
}

float GrndSShadow (vec3 ro, vec3 rd)
{
  float sh = 1.;
  float d = 0.01;
  for (int i = 0; i < 80; i++) {
    vec3 p = ro + rd * d;
    float h = p.y - GrndHt (p.xz, 0);
    sh = min (sh, 20. * h / d);
    d += 0.5;
    if (h < 0.001) break;
  }
  return clamp (sh, 0., 1.);
}

struct WingParm
{
  float span, sRad, trans, thck, leCut, leRad;
};

float WingDf (vec3 p, WingParm wg)
{
  vec2 q = p.yz;
  float w = max (length (q - vec2 (wg.sRad, 0.)),
     length (q + vec2 (wg.sRad, 0.)));
  w = max (max (w - wg.thck, abs (p.x - wg.trans) - wg.span),
     p.z - wg.leCut);
  return min (w, max (length (q - vec2 (0., wg.leCut)) - wg.leRad,
     abs (p.x - wg.trans) - wg.span));
}

float PropelDf (vec3 p, float dHit)
{
  vec3 q;
  float d;
  dHit /= szFac;
  p /= szFac;
  q = p;
  q.x = abs (q.x);
  q -= engPos;
  d = PrCylDf (q - vec3 (0., 0., 3.65), vec2 (1.9, 0.05));
  if (d < dHit) {
    dHit = d;
    qHitTransObj = q;
  }
  return dHit * szFac;
}

float TransObjDf (vec3 p)
{
  float dHit = dstFar;
  dHit = PropelDf (flyerMat * (p - flyerPos), dHit);
  return dHit;
}

float TransObjRay (vec3 ro, vec3 rd)
{
  const float dTol = 0.001;
  float d;
  float dHit = 0.;
  for (int j = 0; j < 150; j ++) {
    d = TransObjDf (ro + dHit * rd);
    dHit += d;
    if (d < dTol || dHit > dstFar) break;
  }
  return dHit;
}

float FlyerDf (vec3 p, float dHit)
{
  vec3 q;
  WingParm wg;
  float d, wr, ws, cLen;
  const float wSweep = 0.2;
  const float fusLen = 11.;
  const float taPos = 12.5;
  dHit /= szFac;
  p /= szFac;
  q = p;
  wr = q.z / fusLen;
  d = PrCapsDf (q - fusLen * vec3 (0., 0.045 + 0.08 * wr, 0.),
     fusLen * vec2 (0.46, 0.11));
  if (d < dHit) {
    dHit = d;  idObj = 1;  qHit = q;
  }
  d = PrCapsDf (q - fusLen * vec3 (0., 0., -0.32),
     fusLen * vec2 (1., 0.15 - 0.051 * wr * wr));
  if (d < dHit + 0.1) {
    dHit = SmoothMin (dHit, d, 0.1);  idObj = 2;  qHit = q;
  }
  ws = wSweep * abs (p.x) / wSpan;
  q = p + vec3 (0., 0.054 * fusLen - 6. * ws, 12. * ws);
  wg = WingParm (wSpan, 13.7, 0., 14., 1.72, 0.195);
  d = WingDf (q, wg);
  if (d < dHit + 0.2) {
    dHit = SmoothMin (dHit, d, 0.2);  idObj = 3;  qHit = q;
   }
  q = p + vec3 (0., -0.1 - 6. * ws, taPos + 12. * ws);
  wg = WingParm (0.4 * wSpan, 6.8, 0., 7., 1.2, 0.095);
  d = WingDf (q, wg);
  if (d < dHit + 0.1) {
    dHit = SmoothMin (dHit, d, 0.1);  idObj = 4;  qHit = q;
  }
  ws = wSweep * abs (p.y) / wSpan;
  q = p.yxz + vec3 (-0.2, 0., taPos + 12. * ws);
  wg = WingParm (0.15 * wSpan, 6.8, 2.2, 7., 1.2, 0.095);
  d = WingDf (q, wg);
  if (d < dHit + 0.1) {
    dHit = SmoothMin (dHit, d, 0.1);  idObj = 5;  qHit = q;
  }
  q = p;
  q.x = abs (q.x);
  cLen = 3.5;
  wr = q.z / cLen;
  d = PrCylDf (q - engPos, cLen * vec2 (0.2  - 0.07 * wr * wr, 1.));
  float d2 = PrCylDf (q - engPos, cLen * vec2 (0.04, 1.02));
  d = max (d, - d2);
  if (d < dHit) {
    dHit = d;  idObj = 6;  qHit = q;
  }
  q = p;
  q.x = abs (q.x);
  d = PrConeDf (q - engPos - vec3 (0., 0., 4.2), vec3 (0.8, 0.6, 0.7));
  if (d < dHit) {
    dHit = d;  idObj = 7;  qHit = q;
  }
  q = p;
  cLen = 2.8;
  q.z += wSweep * wSpan - 0.025 * cLen;
  q.x = abs (q.x);
  wr = q.z / cLen;
  d = PrCapsDf (q - vec3 (wSpan + 0.1, 0.5 * wSweep * wSpan - 0.6, -0.5),
     cLen * vec2 (1., 0.15 - 0.055 * wr * wr));
  if (d < dHit) {
    dHit = d;  idObj = 8;  qHit = q;
  }
  return 0.8 * dHit * szFac;
}

float ObjDf (vec3 p)
{
  float dHit = dstFar;
  dHit = FlyerDf (flyerMat * (p - flyerPos), dHit);
  return dHit;
}

float ObjRay (vec3 ro, vec3 rd)
{
  const float dTol = 0.001;
  float d;
  float dHit = 0.;
  for (int j = 0; j < 150; j ++) {
    d = ObjDf (ro + dHit * rd);
    dHit += d;
    if (d < dTol || dHit > dstFar) break;
  }
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  const vec3 e = vec3 (0.001, -0.001, 0.);
  float v0 = ObjDf (p + e.xxx);
  float v1 = ObjDf (p + e.xyy);
  float v2 = ObjDf (p + e.yxy);
  float v3 = ObjDf (p + e.yyx);
  return normalize (vec3 (v0 - v1 - v2 - v3) + 2. * vec3 (v1, v2, v3));
}

float ObjSShadow (vec3 ro, vec3 rd)
{
  float sh = 1.;
  float d = 0.07 * szFac;
  for (int i = 0; i < 50; i++) {
    float h = ObjDf (ro + rd * d);
    sh = min (sh, 20. * h / d);
    d += 0.07 * szFac;
    if (h < 0.001) break;
  }
  return clamp (sh, 0., 1.);
}

vec3 ObjCol (vec3 p, vec3 n)
{
  vec3 bCol = vec3 (0.8, 0.8, 0.85), wCol = vec3 (0.3, 0.3, 0.7),
     tCol = vec3 (0.9, 0.7, 0.), uCol = vec3 (0.9, 0.1, 0.);
  float cFac = 1.;
  if (idObj >= 3 && idObj <= 5) {
    float s1, s2;
    if (idObj == 3) {
      s1 = 2.2;  s2 = 6.;
    } else if (idObj == 4) {
      s1 = 1.2;  s2 = 1.7;
    } else if (idObj == 5) {
      s1 = 1.;  s2 = 1.;
    }
    if (abs (qHit.x) > s2 - 0.03)
       cFac = 1. - 0.9 * SmoothBump (- 0.08, 0.08, 0.02, qHit.z + s1);
    if (qHit.z < - s1)
       cFac = 1. - 0.9 * SmoothBump (- 0.05, 0.05, 0.02, abs (qHit.x) - s2);
  }
  vec3 col;
  vec3 nn;
  if (idObj >= 1 && idObj <= 5) nn = flyerMat * n;
  if (idObj == 1 || idObj == 2) {
    col = mix (uCol, bCol, 1. - smoothstep (-0.6, 0., nn.y));
    if (idObj == 2 && nn.y < 0.)
       col = mix (bCol, wCol, SmoothBump (-0.8, 0.8, 0.3, qHit.z + 0.28));         
  } else if (idObj == 3 || idObj == 4) {
    col = mix (bCol, wCol, SmoothBump (-0.8, 0.8, 0.3, qHit.z));
  } else if (idObj == 5) {
    col = mix (bCol, tCol, SmoothBump (-0.6, 0.8, 0.3, qHit.z));
  } else if (idObj == 6) {
    col = bCol;
  } else if (idObj == 7 || idObj == 8) {
    col = tCol;
  }
  if (idObj == 1) {
    if (qHit.z > 4.5 && abs (qHit.x) > 0.07) idObj = 10;
  } else if (idObj == 2) {
    float s = - qHit.z;
    if (s > 0. && s < 9.) {
      vec2 ws = vec2 (qHit.y - 0.5, mod (s + 1.5, 1.5) - 0.75);
      ws *= ws;
      if (dot (ws, ws) < 0.02) idObj = 10;
    }
  }
  return col * cFac;
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  const float eps = 0.01;
  vec4 col4;
  vec3 objCol, col, vn;
  float dstHit, dstGrnd, dstObj, dstPropel, f;
  int idObjT;
  vec3 roo = ro;
  dstHit = dstFar;
  dstGrnd = GrndRay (ro, rd);
  wSpan = 12.;
  engPos = vec3 (0.35 * wSpan, -0.2, -1.5);
  idObj = 0;
  dstObj = ObjRay (ro, rd);
  idObjT = idObj;
  dstPropel = TransObjRay (ro, rd);
  if (dstObj < dstPropel) dstPropel = dstFar;
  float refFac = 1.;
  if (dstGrnd < dstObj && ro.y + dstGrnd * rd.y < 0.) {
    float dw = - ro.y / rd.y;
    ro += dw * rd;
    rd = reflect (rd, WaterNf (ro, dw));
    ro += eps * rd;
    dstGrnd = GrndRay (ro, rd);
    idObj = 0;
    dstObj = ObjRay (ro, rd);
    idObjT = idObj;
    refFac *= 0.6;
  }
  bool isGrnd = false;
  if (dstObj < dstGrnd) {
    ro += dstObj * rd;
    vn = ObjNf (ro);
    idObj = idObjT;
    objCol = ObjCol (ro, vn);
    if (idObj == 10) objCol = vec3 (0.2) + 0.5 * SkyCol (ro, reflect (rd, vn));
    float dif = max (dot (vn, sunDir), 0.);
    col = sunCol * objCol * (0.2 * (1. +
       max (dot (vn, - normalize (vec3 (sunDir.x, 0., sunDir.z))), 0.)) +
       max (0., dif) * ObjSShadow (ro, sunDir) *
       (dif + 0.5 * pow (max (0., dot (sunDir, reflect (rd, vn))), 100.)));
    dstHit = dstObj;
  } else {
    vec3 rp = ro + dstGrnd * rd;
    if (refFac < 1.) dstHit = length (rp - roo);
    else dstHit = dstGrnd;
    if (dstHit < dstFar) {
      ro = rp;
      isGrnd = true;
    } else {
      col = refFac * SkyCol (ro, rd);
    }
  }
  if (isGrnd) {
    vn = GrndNf (ro, dstHit);
    col4 = GrndCol (ro, vn);
    col = col4.xyz * refFac;
    float dif = max (dot (vn, sunDir), 0.);
    col *= sunCol * (0.2 * (1. +
       max (dot (vn, - normalize (vec3 (sunDir.x, 0., sunDir.z))), 0.)) +
       max (0., dif) * GrndSShadow (ro, sunDir) *
       (dif + col4.w * pow (max (0., dot (sunDir, reflect (rd, vn))), 100.)));
  }
  if (dstPropel < dstFar) col = 0.7 * col + 0.1 -
     0.04 * SmoothBump (1.5, 1.7, 0.02, length (qHitTransObj.xy));
  if (dstHit < dstFar) {
    f = dstHit / dstFar;
    col = mix (col, refFac * SkyBg (rd), clamp (1.03 * f * f, 0., 1.));
  }
  col = sqrt (clamp (col, 0., 1.));
  return clamp (col, 0., 1.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = 2. * fragCoord.xy / iResolution.xy - 1.;
  vec2 uvs = uv;
  uv.x *= iResolution.x / iResolution.y;
  trkOffset = vec2 (0.);
  float zmFac = 2.7;
  tCur = 15. * iGlobalTime + 100. * trkOffset.y;
  sunDir = normalize (vec3 (0.9, 1., 0.4));
  sunCol = vec3 (1., 0.9, 0.8);
  vec3 ro, rd, vd, fpF, fpB, vDir;
  szFac = 0.25;
  float dt = 1.;
  fpF = TrackPath (tCur + dt);
  flyerPos = TrackPath (tCur);
  fpB = TrackPath (tCur - dt);
  vec3 vel = (fpF - fpB) / (2. * dt);
  vel.y = 0.;
  vec3 acc = (fpF - 2. * flyerPos + fpB) / (dt * dt);
  acc.y = 0.;
  vec3 va = cross (acc, vel) / length (vel);
  float roll = 12. * length (va);
  if (va.y < 0.) roll *= -1.;
  vDir = normalize (vel);
  float cRl = cos (roll);
  float sRl = sin (roll);
  flyerMat = mat3 (cRl, - sRl, 0., sRl, cRl, 0., 0., 0., 1.) *
     mat3 (vDir.z, 0., vDir.x, 0., 1., 0., - vDir.x, 0., vDir.z);
  float vuPeriod = 500.;
  flyerPos.y = 3. + 1.5 * sin (2.5 * tCur / vuPeriod);
  float lookDir = 2. * mod (floor (tCur / vuPeriod), 2.) - 1.;
  ro = TrackPath (tCur - 40. * lookDir * (1. -
     0.8 * abs (sin (pi * mod (tCur, vuPeriod) / vuPeriod))));
  ro.y = 3. + 0.6 * sin (0.16 * tCur / vuPeriod);
  vd = flyerPos - ro;
  vd.y = 0.;
  vd = normalize (lookDir * vDir + 0.3 * normalize (vd));
  mat3 scMat = mat3 (vd.z, 0., - vd.x, 0., 1., 0., vd);
  rd = scMat * normalize (vec3 (uv, zmFac));
  vec3 col = ShowScene (ro, rd);
  uvs *= uvs * uvs;
  col = mix (vec3 (0.7), col, pow (max (0., 0.95 - length (uvs * uvs * uvs)), 0.3));
  fragColor = vec4 (col, 1.);
}

void main(void)
{
  //just some shit to wrap shadertoy's stuff
  vec2 FragmentCoord = vTexCoord.xy*global.OutputSize.xy;
  FragmentCoord.y = -FragmentCoord.y;
  mainImage(FragColor,FragmentCoord);
}
