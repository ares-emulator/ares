#version 450
// Blob Zoo -  dr2 - 2018-02-18
// https://www.shadertoy.com/view/4sdcWN

// Lots of strange critters

// "Blob Zoo" by dr2 - 2018
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

float PrBox2Df (vec2 p, vec2 b);
float PrBoxAn2Df (vec2 p, vec2 b, float w);
float PrSphDf (vec3 p, float r);
float PrCylDf (vec3 p, float r, float h);
float PrCylAnDf (vec3 p, float r, float w, float h);
float PrCapsDf (vec3 p, float r, float h);
float SmoothMin (float a, float b, float r);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);
vec3 HsvToRgb (vec3 c);
float Hashfv2 (vec2 p);
float Fbm2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);
vec2 PixToHex (vec2 p);
vec2 HexToPix (vec2 h);
vec3 HexGrid (vec2 p);

vec3 sunDir;
vec2 gId;
float tCur, dstFar, gcRnd, bHt;
int idObj;
const float pi = 3.14159, sqrt3 = 1.7320508;

#define DMIN(id) if (d < dMin) { dMin = d;  idObj = id; }

vec3 SMap (vec3 p, float t)
{
  float f;
  f = 2.;
  for (int k = 0; k < 5; k ++) {
    p += 0.4 * sin (1.7 * p.yzx / f + f * t);
    f *= 0.8;
  }
  return p;
}

float BlobDf (vec3 p)
{
  float t;
  t = tCur + 25. * gcRnd;
  p *= 2.;
  p.xz = Rot2D (p.xz, 0.2 * t);
  return max (0.1 * SmoothMin (PrSphDf (SMap (p - vec3 (0.7, 1.8, 0.), t + 2.), 1.1 + 0.31 * sin (t)),
     PrSphDf (SMap (p + vec3 (0.7, -1.8, 0.), 1.3 * t), 1. + 0.41 * sin (1.7 * t)), 0.5), - p.y);
}

float ObjDf (vec3 p)
{
  vec3 q, qq;
  float dMin, d, szFac, w, s;
  szFac = 0.1;
  dMin = dstFar / szFac;
  p.xz -= HexToPix (gId);
  p /= szFac;
  w = 0.05;
  q = p;
  d = max (min (max (PrBoxAn2Df (q.xz, vec2 (2., 7.5), w), - PrBox2Df (q.xz, vec2 (2.1 + w, 2. - w))),
     max (PrBoxAn2Df (q.xz, vec2 (7.5, 2.), w), - PrBox2Df (q.xz, vec2 (2. - w, 2.1 + w)))), abs (q.y - bHt) - bHt);
  qq = q;
  qq.xz = mod (qq.xz, 0.5) - 0.25;
  s = 1. - sqrt (1. - smoothstep (1.3, 1.7, qq.y));
  qq.y -= 0.82;
  d = max (d, - min (PrBox2Df (qq.xy, vec2 (0.2 - 0.2 * s, 0.9)),
     PrBox2Df (qq.zy, vec2 (0.2 - 0.2 * s, 0.9))));
  DMIN (1);
  q.xz = Rot2D (q.xz, pi / 6.);
  q.xz = Rot2D (q.xz, 2. * pi * ((floor (6. * atan (q.z, - q.x) / (2. * pi) + 0.5)) / 6.));
  q.xy -= vec2 (-10., 1.6);
  d = PrCylAnDf (q.xzy, 1.15, 0.08, 0.015);
  DMIN (2);
  q.xz = Rot2D (vec2 (q.x - 0.65, abs (q.z)), - pi / 3.);
  q.x = abs (q.x);
  q.xy -= vec2 (0.7, -0.8);
  d = PrCylDf (q.xzy, 0.03, 0.8);
  DMIN (3);
  q = p;
  q.y -= 0.05;
  d = PrCylAnDf (q.xzy, 1.7, 0.07, 0.05);
  DMIN (2);
  q = p;
  d = length (q.xz) - 1.7;
  if (d < 0.1) {
    d = BlobDf (q);
    DMIN (4);
  } else dMin = min (dMin, d);
  return dMin * szFac;
}

void SetGrdConf ()
{
  gcRnd = Hashfv2 (17.3 * gId);
  bHt =  (1./24.) * floor (22. + 5. * gcRnd);
}

float ObjRay (vec3 ro, vec3 rd)
{
  vec3 vri, vf, hv, p;
  vec2 edN[3], pM, gIdP;
  float dHit, d, s;
  edN[0] = vec2 (1., 0.);
  edN[1] = 0.5 * vec2 (1., sqrt3);
  edN[2] = 0.5 * vec2 (1., - sqrt3);
  for (int k = 0; k < 3; k ++) edN[k] *= sign (dot (edN[k], rd.xz));
  vri = 1. / vec3 (dot (rd.xz, edN[0]), dot (rd.xz, edN[1]), dot (rd.xz, edN[2]));
  vf = 0.5 * sqrt3 - vec3 (dot (ro.xz, edN[0]), dot (ro.xz, edN[1]),
     dot (ro.xz, edN[2]));
  pM = HexToPix (PixToHex (ro.xz));
  hv = (vf + vec3 (dot (pM, edN[0]), dot (pM, edN[1]), dot (pM, edN[2]))) * vri;
  s = min (hv.x, min (hv.y, hv.z));
  gIdP = vec2 (-999.);
  dHit = 0.;
  for (int j = 0; j < 220; j ++) {
    p = ro + dHit * rd;
    gId = PixToHex (p.xz);
    if (gId.x != gIdP.x || gId.y != gIdP.y) {
      gIdP = gId;
      SetGrdConf ();
    }
    d = ObjDf (p);
    if (dHit + d < s) dHit += d;
    else {
      dHit = s + 0.001;
      pM += sqrt3 * ((s == hv.x) ? edN[0] : ((s == hv.y) ? edN[1] : edN[2]));
      hv = (vf + vec3 (dot (pM, edN[0]), dot (pM, edN[1]), dot (pM, edN[2]))) * vri;
      s = min (hv.x, min (hv.y, hv.z));
    }
    if (d < 0.0002 || dHit > dstFar || p.y < 0. || p.y > 2.) break;
  }
  if (d >= 0.0002) dHit = dstFar;
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  vec4 v;
  vec2 e = vec2 (0.00002, -0.00002);
  v = vec4 (ObjDf (p + e.xxx), ObjDf (p + e.xyy), ObjDf (p + e.yxy), ObjDf (p + e.yyx));
  return normalize (vec3 (v.x - v.y - v.z - v.w) + 2. * v.yzw);
}

vec2 TrackPath (float t)
{
  vec2 r;
  float tt;
  tt = mod (t, 4.);
  if (tt < 1.) r = mix (vec2 (sqrt3 * 0.5, -0.5), vec2 (sqrt3 * 0.5, 0.5), tt);
  else if (tt < 2.) r = mix (vec2 (sqrt3 * 0.5, 0.5), vec2 (0., 1.), tt - 1.);
  else if (tt < 3.) r = mix (vec2 (0., 1.), vec2 (0., 2.), tt - 2.);
  else r = mix (vec2 (0., 2.), vec2 (sqrt3 * 0.5, 2.5), tt - 3.);
  r += vec2 (0.005, 3. * floor (t / 4.));
  return r;
}

float ObjSShadow (vec3 ro, vec3 rd)
{
  vec3 p;
  vec2 gIdP;
  float sh, d, h;
  sh = 1.;
  gIdP = vec2 (-999.);
  d = 0.001;
  for (int j = 0; j < 40; j ++) {
    p = ro + d * rd;
    gId = PixToHex (p.xz);
    if (gId.x != gIdP.x || gId.y != gIdP.y) {
      gIdP = gId;
      SetGrdConf ();
    }
    h = ObjDf (p);
    sh = min (sh, smoothstep (0., 0.03 * d, h));
    d += min (0.005, 3. * h);
    if (h < 0.001) break;
  }
  return 0.8 + 0.2 * sh;
}

vec3 SkyCol (vec3 rd)
{
  return mix (vec3 (0.2, 0.3, 0.5) + 0.3 * pow (max (dot (rd, sunDir), 0.), 8.), vec3 (1.),
     0.2 + 0.8 * rd.y * Fbm2 (2. * rd.xz / rd.y));
}

vec3 ShStagGrid (vec2 p, vec2 g)
{
  vec2 q, sq, ss;
  q = p * g;
  if (2. * floor (0.5 * floor (q.y)) != floor (q.y)) q.x += 0.5;
  sq = smoothstep (0.03, 0.07, abs (fract (q + 0.5) - 0.5));
  q = fract (q) - 0.5;
  ss = 0.3 * smoothstep (0.35, 0.5, abs (q.xy)) * sign (q.xy);
  if (abs (q.x) < abs (q.y)) ss.x = 0.;
  else ss.y = 0.;
  return vec3 (ss.x, 0.9 + 0.1 * sq.x * sq.y, ss.y);
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col, vn, qh, rg;
  vec2 g, vf;
  float dstObj, dstGrnd, spec, sh, f;
  bool fxz;
  dstGrnd = (rd.y < 0.) ? - ro.y / rd.y : dstFar;
  dstObj = ObjRay (ro, rd);
  if (min (dstObj, dstGrnd) < dstFar) {
    sh = 1.;
    vf = vec2 (0.);
    if (dstObj < dstGrnd) {
      ro += dstObj * rd;
      gId = PixToHex (ro.xz);
      SetGrdConf ();
      vn = ObjNf (ro);
      if (idObj == 1) {
        col = HsvToRgb (vec3 (0.04 + 0.12 * gcRnd, 0.7, 0.7));
        spec = 0.05;
        if (ro.y > 0.2 * bHt - 0.0005) col*= 0.7;
        if (abs (vn.y) < 0.01) {
          rg = 10. * ro;
          fxz = (abs (vn.x) > 0.99);
          rg = ShStagGrid ((fxz ? rg.zy : rg.xy), 6. * vec2 (1., 2.));
          col *= rg.y;
          rg.xz *= sign (fxz ? vn.x : vn.z);
          if (fxz) {
            if (rg.x == 0.) vn.xy = Rot2D (vn.xy, rg.z);
            else vn.xz = Rot2D (vn.xz, rg.x);
          } else {
            if (rg.x == 0.) vn.zy = Rot2D (vn.zy, rg.z);
            else vn.zx = Rot2D (vn.zx, rg.x);
          }
        }
      } else if (idObj == 2) {
        col = vec3 (0.6, 0.6, 0.7);
        spec = 0.1;
        vf = vec2 (1024., 0.5);
      } else if (idObj == 3) {
        f = mod (256. * ro.y + atan (vn.z, vn.x) / (2. * pi), 1.);
        if (abs (f - 0.5) < 0.4) {
          col = vec3 (0.6, 0.6, 0.7);
          spec = 0.1;
          vf = vec2 (1024., 0.5);
        } else {
          col = vec3 (0.5, 0.7, 0.2);
          vn.y = sin (2. * pi * (abs (f - 0.5) - 0.4) * sign (f));
          vn.xz *= sqrt (1. - vn.y * vn.y);
          spec = 0.3;
        }
      } else if (idObj == 4) {
        col = HsvToRgb (vec3 (mod (13. * gcRnd, 1.), 1., 0.9));
        spec = 0.3;
        vf = vec2 (256., 0.5);
      }
      if (idObj != 3) sh = ObjSShadow (ro, sunDir);
    } else {
      ro += dstGrnd * rd;
      gId = PixToHex (ro.xz);
      SetGrdConf ();
      vn = vec3 (0., 1., 0.);
      g = ro.xz - HexToPix (gId);
      if (length (g) < 0.17) {
        col = vec3 (0.4, 0.4, 0.5);
        vf = vec2 (256., 0.2);
      } else if (abs (g.x) < 0.2 && abs (g.y) < 0.75 || abs (g.x) < 0.75 && abs (g.y) < 0.2) {
        col = HsvToRgb (vec3 (0.3 + 0.15 * gcRnd, 1., 0.5));
        vf = vec2 (256., 2.);
      } else {
        qh = HexGrid (32. * sqrt3 * ro.zx);
        f = max (length (qh.xy) - 0.5, 0.);
        vn = vec3 (0., Rot2D (vec2 (1., 0.), 4. * f * f));
        vn.zx = vn.z * normalize (qh.xy);
        col = vec3 (0.5, 0.45, 0.45);
        col = mix (vec3 (0.5, 0.5, 0.3), col, smoothstep (0.036, 0.038, HexGrid (ro.xz).z));
        col *= 0.8 + 0.2 * smoothstep (0.03, 0.06, qh.z);
        g = Rot2D (g, pi / 6.);
        g = Rot2D (g, 2. * pi * ((floor (6. * atan (g.y, - g.x) / (2. * pi) + 0.5)) / 6.));
        g = Rot2D (vec2 (g.x + 0.935, abs (g.y)), - pi / 3.);
        col *= 0.8 + 0.2 * smoothstep (0.003, 0.006, length (vec2 (abs (g.x) - 0.07, g.y)));
      }
      spec = 0.1;
      sh = ObjSShadow (ro, sunDir);
    }
    if (vf.x > 0.) vn = VaryNf (vf.x * ro, vn, vf.y);
    col = col * (0.2 + 0.2 * max (dot (normalize (- sunDir.xz), vn.xz), 0.) +
       0.7 * sh * max (dot (vn, sunDir), 0.)) +
       spec * sh * pow (max (dot (normalize (sunDir - rd), vn), 0.), 32.);
  } else {
    col = (rd.y > 0.) ? SkyCol (rd) : vec3 (0.5, 0.45, 0.45);
  }
  return clamp (col, 0., 1.);
}

// hack for libretro spec
vec4 iDate = vec4(0.0, 0.0, 0.0, 0.0);

#ifndef MOUSE
// hack for libretro spec
vec4 iMouse = vec4(0.0, 0.0, 0.0, 0.0);
#endif

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr, dateCur;
  vec3 ro, rd;
  vec2 canvas, uv, ori, ca, sa, vd, p1, p2;
  float el, az, asp, vel, tCyc, tt, a;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iGlobalTime;
  dateCur = iDate;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  vel = 0.12;
  tCyc = 4. / vel;
  tCur = mod (tCur, 36000.) + floor (2. + floor (dateCur.w / 600.) / tCyc) * tCyc;
  p1 = TrackPath (vel * tCur + 0.11);
  p2 = TrackPath (vel * tCur - 0.11);
  ro.xz = 0.5 * (p1 + p2);
  ro.y = 0.09;
  vd = p1 - p2;
  az = atan (vd.x, vd.y);
  el = 0.;
#ifdef MOUSE
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += 0.2 * pi * mPtr.y;
  }
  else
  {
    a = 0.45 * pi * SmoothBump (0.25, 0.75, 0.12, mod (tCur / (0.25 * tCyc), 1.));
    tt = mod (tCur / tCyc, 1.) - 0.375;
    az += a * (step (abs (tt + 0.25), 0.125) - step (abs (tt - 0.25), 0.125));
  }
#else
  a = 0.45 * pi * SmoothBump (0.25, 0.75, 0.12, mod (tCur / (0.25 * tCyc), 1.));
  tt = mod (tCur / tCyc, 1.) - 0.375;
  az += a * (step (abs (tt + 0.25), 0.125) - step (abs (tt - 0.25), 0.125));
#endif
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  asp = canvas.x / canvas.y;
  uv.xy /= 2.5;
  rd = vuMat * normalize (vec3 (2. * tan (0.5 * atan (uv.x / asp)) * asp, uv.y, 1.));
  dstFar = 50.;
  sunDir = normalize (vec3 (0.5, 3., -1.));
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

float PrBoxAn2Df (vec2 p, vec2 b, float w)
{
  return max (PrBox2Df (p, vec2 (b + w)), - PrBox2Df (p, vec2 (b - w)));
}

float PrSphDf (vec3 p, float r)
{
  return length (p) - r;
}

float PrCylDf (vec3 p, float r, float h)
{
  return max (length (p.xy) - r, abs (p.z) - h);
}

float PrCylAnDf (vec3 p, float r, float w, float h)
{
  return max (abs (length (p.xy) - r) - w, abs (p.z) - h);
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
  return q * cos (a) * vec2 (1., 1.) + q.yx * sin (a) * vec2 (-1., 1.);
}

vec2 PixToHex (vec2 p)
{
  vec3 c, r, dr;
  c.xz = vec2 ((1./sqrt3) * p.x - (1./3.) * p.y, (2./3.) * p.y);
  c.y = - c.x - c.z;
  r = floor (c + 0.5);
  dr = abs (r - c);
  r -= step (dr.yzx, dr) * step (dr.zxy, dr) * dot (r, vec3 (1.));
  return r.xz;
}

vec2 HexToPix (vec2 h)
{
  return vec2 (sqrt3 * (h.x + 0.5 * h.y), (3./2.) * h.y);
}

vec3 HexGrid (vec2 p)
{
  vec2 q;
  p -= HexToPix (PixToHex (p));
  q = abs (p);
  return vec3 (p, 0.5 * sqrt3 - q.x + 0.5 * min (q.x - sqrt3 * q.y, 0.));
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
  for (int j = 0; j < 5; j ++) {
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
  for (int j = 0; j < 5; j ++) {
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
