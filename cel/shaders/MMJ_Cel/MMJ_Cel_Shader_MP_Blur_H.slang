#version 450

#pragma name MMJ_BlurPass_H

/*
----------------------------------------------------------------
MMJ's Cel Shader v2.0 - Multi-Pass 
----------------------------------------------------------------
Based on the original blur-gauss-h shader code.

Used to blur the outlines, which is helpful at higher internal
resolution settings to increase the line thickness.

Parameters:
-----------
Blur Weight - Horizontal = Adjusts horizontal blur factor.
----------------------------------------------------------------
*/

layout(push_constant) uniform Push
{
  vec4 SourceSize;
  float BlurWeightH;
} params;

#pragma parameter BlurWeightH "Blur Weight - Horizontal" 0.0 0.0 16.0 1.0

#define SourceSize params.SourceSize
#define BlurWeightH params.BlurWeightH

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

void main()
{
	vec2 PIXEL_SIZE = SourceSize.zw;
  vec4 C = texture(Source, vTexCoord);
  float L = 0.0, J = 0.0;
  for(int i = 1; i <= BlurWeightH; ++i) {
    L = 1.0 / i;
    J = 0.5 * i * PIXEL_SIZE.x;
    C = mix(C, mix(texture(Source, vTexCoord + vec2(J, 0.0)), texture(Source, vTexCoord - vec2(J, 0.0)), 0.5), L);
  }
  FragColor = C;
}