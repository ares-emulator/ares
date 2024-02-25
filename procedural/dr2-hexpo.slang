#version 450
// "Hexpo" by dr2 - 2018
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
float PrCylDf (vec3 p, float r, float h);
float SmoothMin (float a, float b, float r);
float SmoothBump (float lo, float hi, float w, float x);
vec2 Rot2D (vec2 q, float a);
vec3 HueToRgb (float c);
float Hashfv2 (vec2 p);
float Fbm2 (vec2 p);
vec3 VaryNf (vec3 p, vec3 n, float f);
vec2 PixToHex (vec2 p);
vec2 HexToPix (vec2 h);
vec3 HexGrid (vec2 p);

vec3 sunDir, qHit;
vec2 gId;
float tCur, dstFar, gcRnd, hgSize;
int idObj;
const float pi = 3.14159, sqrt3 = 1.7320508;

#define DMINQ(id) if (d < dMin) { dMin = d;  idObj = id;  qHit = q; }

float ObjDf (vec3 p)
{
  vec3 q, qq;
  float dMin, d, w, s, r, rh, a, s3;
  dMin = dstFar;
  s3 = 0.5 * sqrt3 * hgSize;
  rh = 3.07;
  w = 0.1;
  p.xz -= HexToPix (gId * hgSize);
  r = length (p.xz);
  a = (r > 0.) ? atan (p.z, - p.x) / (2. * pi) : 0.;
  q = p;
  q.xz = Rot2D (q.xz, 2. * pi * ((floor (6. * a + 0.5)) / 6.));
  qq = q;
  s = 1. - sqrt (1. - smoothstep (0.65, 0.85, q.y));
  d = max (max (abs (q.x + 3.) - 0.07, abs (q.y - 0.5) - 0.5),
     - PrBox2Df (vec2 (mod (q.z, 0.25) - 0.125, q.y - 0.41), vec2 (0.08 - 0.08 * s, 0.45)));
  s = smoothstep (1., 2.2, q.y);
  d = min (d, max (max (mix (abs (q.x + 0.5 * rh) - 0.5 * rh, r - 2. - w, s),
     - mix (abs (q.x + 0.5 * rh - w) - 0.5 * rh + w, r - 2. + w, s)), abs (q.y - 1.6) - 0.6));
  d = min (d, max (abs (r - 2.) - w, abs (q.y - 2.25) - 0.05));
  s = smoothstep (2.3, rh, q.y);
  d = min (d, max (max (mix (r - 2. - w, r - 1.4 - w, s), - mix (r - 2. + w, r - 1.4 + w, s)), abs (q.y - 2.6) - 0.3));
  DMINQ (1);
  q.xy -= vec2 (- s3 + 0.06, 0.06);
  d = length (q.xy) - 0.05;
  DMINQ (2);
  q.y -= 1.15 - 0.2 * q.x * q.x;
  d = PrCylDf (vec3 (q.xy, abs (q.z) - 0.04).yzx, 0.03, s3 - rh);
  DMINQ (3);
  q = qq;  q.xy -= vec2 (- s3, 1.21);
  d = PrCylDf (q.xzy, 0.1, 0.04);
  q = qq;  q.z = abs (q.z);  q -= vec3 (- s3, 0.07, 0.5 * hgSize);
  d = min (d, PrCylDf (q.xzy, 0.15, 0.07));
  DMINQ (4);
  q = qq;  q.y -= 2.2;
  d = PrCylDf (q.xzy, 0.3, 2.2);
  DMINQ (5);
  q = qq;  q.xy -= vec2 (-0.6, 1.8);
  d = PrCylDf (q.xzy, 0.25, 1.8);
  DMINQ (6);
  return 0.6 * dMin;
}

void SetGrdConf ()
{
  gcRnd = Hashfv2 (17.3 * gId);
}

float ObjRay (vec3 ro, vec3 rd)
{
  vec3 vri, vf, hv, p;
  vec2 edN[3], pM, gIdP;
  float dHit, d, s;
  if (rd.x == 0.) rd.x = 0.0001;
  if (rd.y == 0.) rd.y = 0.0001;
  if (rd.z == 0.) rd.z = 0.0001;
  edN[0] = vec2 (1., 0.);
  edN[1] = 0.5 * vec2 (1., sqrt3);
  edN[2] = 0.5 * vec2 (1., - sqrt3);
  for (int k = 0; k < 3; k ++) edN[k] *= sign (dot (edN[k], rd.xz));
  vri = hgSize / vec3 (dot (rd.xz, edN[0]), dot (rd.xz, edN[1]), dot (rd.xz, edN[2]));
  vf = 0.5 * sqrt3 - vec3 (dot (ro.xz, edN[0]), dot (ro.xz, edN[1]),
     dot (ro.xz, edN[2])) / hgSize;
  pM = HexToPix (PixToHex (ro.xz / hgSize));
  hv = (vf + vec3 (dot (pM, edN[0]), dot (pM, edN[1]), dot (pM, edN[2]))) * vri;
  s = min (hv.x, min (hv.y, hv.z));
  gIdP = vec2 (-999.);
  dHit = 0.;
  for (int j = 0; j < 320; j ++) {
    p = ro + dHit * rd;
    gId = PixToHex (p.xz / hgSize);
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
    if (d < 0.0005 || dHit > dstFar || p.y < 0. || p.y > 5.) break;
  }
  if (d >= 0.0005) dHit = dstFar;
  return dHit;
}

vec3 ObjNf (vec3 p)
{
  vec4 v;
  vec2 e = vec2 (0.0001, -0.0001);
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
  r *= hgSize;
  r += vec2 (0.001, hgSize * 3. * floor (t / 4.));
  return r;
}

vec3 SkyCol (vec3 rd)
{
  rd.y = abs (rd.y);
  return 0.2 * mix (vec3 (0.2, 0.3, 0.5) + 0.3 * pow (max (dot (rd, sunDir), 0.), 8.), vec3 (1.),
     0.2 + 0.8 * rd.y * Fbm2 (2. * rd.xz / max (rd.y, 0.0001)));
}

vec3 ShowScene (vec3 ro, vec3 rd)
{
  vec3 col, gCol, vn, qc, qh;
  vec2 vf;
  float dstObj, dstGrnd, spec, f, s;
  bool isLit;
  dstGrnd = (rd.y < 0.) ? - ro.y / rd.y : dstFar;
  dstObj = ObjRay (ro, rd);
  isLit = true;
  if (min (dstObj, dstGrnd) < dstFar) {
    vf = vec2 (0.);
    ro += min (dstObj, dstGrnd) * rd;
    gId = PixToHex (ro.xz / hgSize);
    SetGrdConf ();
    gCol = HueToRgb (mod (77.7 * gcRnd, 1.));
    spec = 0.1;
    if (dstObj < dstGrnd) {
      vn = ObjNf (ro);
      if (idObj == 1) {
        qc = ro - vec3 (hgSize * HexToPix (gId).xy, 0.).xzy;
        f = dot (vn, normalize (qc));
        col = gCol;
        vf = vec2 (1024., 0.5);
        if (f < 0. || ro.y < 1. && f < 0.6) {
          col = 0.8 * mix (vec3 (1.), col, 0.5) * (0.55 - 0.45 * f) * (0.7 +
             0.3 * SmoothBump (0.25, 0.75, 0.1, mod (tCur + gcRnd, 1.)));
          isLit = false;
        } else {
          col = 0.2 * mix (vec3 (1.), col, 0.2) * (0.2 + 0.8 * f);
          s = atan (qc.z, - qc.x) / (2. * pi);
          if (ro.y < 2.6) {
            s = SmoothBump (1., 2.5, 0.1, ro.y) * SmoothBump (0.4, 0.6, 0.02, mod (128. * s, 1.)) *
               SmoothBump (0.4, 0.6, 0.02, mod (8. * qc.y - 2. * tCur, 1.));
            col *= 1. + 5. * s;
          } else {
            s = smoothstep (2.6, 2.85, ro.y) * SmoothBump (0.4, 0.6, 0.05, mod (32. * s +
               2. * (2. * step (gcRnd, 0.5) - 1.) * tCur, 1.));
            col *= 1. + 4. * s;
          }
          isLit = (s > 0.1);
        }
      } else if (idObj == 2) {
        col = 0.8 * gCol * (0.4 + 0.6 * SmoothBump (0.25, 0.75, 0.05, mod (2. * qHit.z + tCur, 1.)));
      } else if (idObj == 3) {
        col = 0.8 * HueToRgb (mod (0.1 * tCur, 1.)) * (0.4 +
           0.6 * SmoothBump (0.25, 0.75, 0.05, mod (4. * qHit.x + 2. * tCur * sign (qHit.z), 1.)));
      } else if (idObj == 4) {
        col = vec3 (0.7, 0.7, 0.2) * (0.3 + 0.7 * smoothstep (0., 0.4, - dot (vn, rd)));
        isLit = false;
      } else if (idObj == 5 || idObj == 6) {
        col = 0.8 * ((idObj == 5) ? gCol : HueToRgb (mod (mod (77.7 * gcRnd, 1.) + 0.5, 1.)));
        if (vn.y < 0.99) col *= (0.4 + 0.6 * SmoothBump (0.25, 0.75, 0.05, mod (8. * qHit.y +
           atan (vn.z, vn.x) / (2. * pi) + ((idObj == 5) ? - 4. : 4.) * tCur, 1.)));
        else col *= 0.5;
      }
    } else {
      qh = HexGrid (32. * sqrt3 * ro.zx / hgSize);
      f = max (length (qh.xy) - 0.5, 0.);
      vn = vec3 (0., Rot2D (vec2 (1., 0.), 4. * f * f));
      vn.zx = vn.z * normalize (qh.xy);
      s = HexGrid (ro.xz / hgSize).z;
      col = 0.3 * mix (vec3 (1.), gCol, 0.2) * (0.8 + 0.2 * smoothstep (0.025, 0.03, s)) *
         (0.8 + 0.2 * smoothstep (0.03, 0.06, qh.z));
      col = mix (col, 0.5 * gCol * (0.7 + 0.3 * SmoothBump (0.25, 0.75, 0.1, mod (tCur + gcRnd, 1.))),
         smoothstep (0.1, 0.35, s));
    }
    if (isLit) {
      if (vf.x > 0.) vn = VaryNf (vf.x * ro, vn, vf.y);
      col = col * (0.5 + 0.5 * max (dot (vn, sunDir), 0.)) +
         spec * pow (max (dot (normalize (sunDir - rd), vn), 0.), 32.);
    }
    s = min (dstObj, dstGrnd) / dstFar;
    col = mix (col * (1. - 0.95 * smoothstep (0.3, 1., s)), SkyCol (rd), smoothstep (0.4, 1., s));
  } else {
    col = SkyCol (rd);
  }
  return clamp (col, 0., 1.);
}

void mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  mat3 vuMat;
  vec4 mPtr, dateCur;
  vec3 ro, rd;
  vec2 canvas, uv, ori, ca, sa, vd, p1, p2;
  float el, az, zmFac, asp, vel, tCyc, tt, s;
  canvas = iResolution.xy;
  uv = 2. * fragCoord.xy / canvas - 1.;
  uv.x *= canvas.x / canvas.y;
  tCur = iGlobalTime;
  dateCur = iDate;
  mPtr = iMouse;
  mPtr.xy = mPtr.xy / canvas - 0.5;
  hgSize = 5.;
  vel = 0.3;
  tCyc = 4. / vel;
  tCur = mod (tCur, 36000.) + floor (2. + floor (dateCur.w / 600.) / tCyc + 1.) * tCyc;
  p1 = TrackPath (vel * tCur + 0.3);
  p2 = TrackPath (vel * tCur - 0.3);
  s = SmoothBump (0.25, 0.75, 0.02, mod (tCur / (4. * tCyc), 1.));
  ro = vec3 (0.5 * (p1 + p2), 0.7 + 3.3 * s).xzy;
  vd = p1 - p2;
  az = atan (vd.x, vd.y);
  el = -0.1 * pi * s;
  zmFac = 1.5 + s;
  if (mPtr.z > 0.) {
    az += 2. * pi * mPtr.x;
    el += 0.5 * pi * mPtr.y;
  }
  ori = vec2 (el, az);
  ca = cos (ori);
  sa = sin (ori);
  vuMat = mat3 (ca.y, 0., - sa.y, 0., 1., 0., sa.y, 0., ca.y) *
          mat3 (1., 0., 0., 0., ca.x, - sa.x, 0., sa.x, ca.x);
  asp = canvas.x / canvas.y;
  dstFar = 300.;
  uv.xy /= zmFac;
  rd = vuMat * normalize (vec3 (2. * tan (0.5 * atan (uv.x / asp)) * asp, uv.y, 1.));
  sunDir = normalize (vec3 (0.5, 1., -1.));
  fragColor = vec4 (ShowScene (ro, rd), 1.);
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
  p = p - HexToPix (PixToHex (p));
  q = abs (p);
  return vec3 (p, 0.5 * sqrt3 - q.x + 0.5 * min (q.x - sqrt3 * q.y, 0.));
}

vec3 HueToRgb (float c)
{
  return clamp (abs (fract (c + vec3 (1., 2./3., 1./3.)) * 6. - 3.) - 1., 0., 1.);
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
