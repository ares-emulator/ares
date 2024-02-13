#version 450

layout(push_constant) uniform Push
{
  vec4 SourceSize;
  vec4 OriginalSize;
  vec4 OutputSize;
  uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
  mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
  gl_Position = global.MVP * Position;
  vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D TEX_CRT;


void main()
{
  const vec3 packed = texture(TEX_CRT, vTexCoord).rgb; 

  const vec2 dudv = ((packed.rg * 255.0) / 16.0) - 7.0;
  const vec2 uv0 = vTexCoord * params.SourceSize.xy;
  const vec2 uv = (uv0 + dudv) / params.SourceSize.xy;

  FragColor = vec4(packed.b * texture(Source, uv).rgb, 1.0);
}
