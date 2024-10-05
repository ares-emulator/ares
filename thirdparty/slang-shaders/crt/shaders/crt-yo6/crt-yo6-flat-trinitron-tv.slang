#version 450

layout(push_constant) uniform Push
{
  vec4 SourceSize;
  vec4 OriginalSize;
  vec4 OutputSize;
  uint FrameCount;
  float TVL;
  float VSIZE;
  float VZOOM;
  float VOFF;
  float VOFFC;
  float GAMMA;
} params;

#pragma parameter TVL   "TVL (0:Auto)"      0.0      0.0 16384.0 1.0
#pragma parameter VSIZE "V-Size (0:Auto)"   0.0      0.0 16384.0 1.0
#pragma parameter VOFF  "V-Offset"          0.0 -16384.0 16384.0 1.0
#pragma parameter VOFFC "V-Offset Centered" 1.0      0.0     1.0 1.0
#pragma parameter VZOOM "V-Zoom (2:Auto)"   2.0      2.0    15.0 1.0
#pragma parameter GAMMA "Gamma"             1.8      0.0     4.0 0.1

#define TEX_SIZE vec2(2560.0, 117.0)
#define NB_ZOOM_LEVEL 13
#define PAT_NFO_A const vec4 pat_nfo_a[NB_ZOOM_LEVEL] = { \
  vec4( 2.0,  3.0, 0.0,   0.0), \
  vec4( 3.0,  4.0, 1.0,   3.0), \
  vec4( 3.0,  5.0, 1.0,   7.0), \
  vec4( 4.0,  6.0, 1.0,  12.0), \
  vec4( 5.0,  7.0, 1.0,  18.0), \
  vec4( 5.0,  8.0, 1.0,  25.0), \
  vec4( 6.0,  9.0, 2.0,  33.0), \
  vec4( 7.0, 10.0, 2.0,  42.0), \
  vec4( 7.0, 11.0, 2.0,  52.0), \
  vec4( 8.0, 12.0, 2.0,  63.0), \
  vec4( 9.0, 13.0, 3.0,  75.0), \
  vec4( 9.0, 14.0, 3.0,  88.0), \
  vec4(10.0, 15.0, 3.0, 102.0)  \
}

layout(std140, set = 0, binding = 0) uniform UBO
{
  mat4 MVP;
} global;


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vXY;
layout(location = 1) out vec4 patNFO;
layout(location = 2) out vec4 crtSize;
layout(location = 3) out float vOff;

void main()
{
  PAT_NFO_A;
  float vsize = int(params.VSIZE) <= 0 ? params.SourceSize.y : params.VSIZE;
  float tvl = int(params.TVL) <= 0 ? (vsize * 2.0) : params.TVL;
  vec2 crt_sz = vec2(tvl, vsize);

  int zoom = int(params.VZOOM) - 3;
  if (zoom < 0) {
    zoom = 0;
    for (int level = 1; level < NB_ZOOM_LEVEL; level++) {
       vec2 out_sz = pat_nfo_a[level].xy * crt_sz;
       if (out_sz.x > params.OutputSize.x || out_sz.y > params.OutputSize.y) {
         break;
       }
       zoom = level;
    }
  }
  vec2 out_sz = pat_nfo_a[zoom].xy * crt_sz;

  vXY = (TexCoord * params.OutputSize.xy) - floor((params.OutputSize.xy - out_sz) / 2.0);
  patNFO = pat_nfo_a[zoom];
  crtSize = vec4(out_sz, crt_sz);
  vOff = params.VOFF +
    ((int(params.VOFFC) == 1) ? (floor((params.SourceSize.y - crt_sz.y) / 2.0)) : 0.0);
  gl_Position = global.MVP * Position;
}


#pragma stage fragment
layout(location = 0) in vec2 vXY;
layout(location = 1) in vec4 patNFO;
layout(location = 2) in vec4 crtSize;
layout(location = 3) in float vOff;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D TEX;

float clip_px(vec2 pos) {
  vec2 s = (sign(pos) + 1.0) / 2.0;
  return s.x * s.y;
} 

void main()
{
  float row = floor(vXY.y / patNFO.y) + vOff + 0.5;
  vec3 dst = vec3(0.0);
  for (int ch = 0; ch < 3; ch++) {
    vec2 xy = vXY + vec2(float(1 - ch) * patNFO.z, 0.0);
    float col = floor(xy.x / patNFO.x) + 0.5;
    vec3 src = texture(Source, vec2(col / crtSize.z, row / params.SourceSize.y)).rgb;
    vec2 uv = vec2(floor(pow(src[ch], params.GAMMA) * 255.0) * patNFO.x, patNFO.w) + mod(xy, patNFO.xy);
    dst[ch] = texture(TEX, uv / TEX_SIZE).r;
  }
  float clip = clip_px(vec2(row, params.SourceSize.y - row)) * clip_px(vXY) * clip_px(crtSize.xy - vXY);
  FragColor = vec4(dst * clip, 1.0);
}
