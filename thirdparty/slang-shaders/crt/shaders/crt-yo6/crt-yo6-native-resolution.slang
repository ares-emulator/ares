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
layout(location = 0) out float vU;
layout(location = 1) out float vY;

void main()
{
  gl_Position = global.MVP * Position;
  vU = TexCoord.x;
  vY = (TexCoord.y * params.OutputSize.y) - 
       round((params.OutputSize.y - params.SourceSize.y) / 2.0);
}

#pragma stage fragment
layout(location = 0) in float vU;
layout(location = 1) in float vY;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  float clip0 = (sign(vY) + 1.0) / 2.0;
  float clipH = (sign(params.SourceSize.y - vY) + 1.0) / 2.0;
  vec3 col = texture(Source, vec2(vU, vY / params.SourceSize.y)).rgb;
  FragColor = vec4(clip0 * clipH * col, 1.0); 
}
