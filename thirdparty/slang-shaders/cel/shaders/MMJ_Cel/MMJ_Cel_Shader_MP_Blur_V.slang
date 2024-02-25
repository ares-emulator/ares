#version 450

#pragma name MMJ_BlurPass_V

/*
----------------------------------------------------------------
MMJ's Cel Shader v2.0 - Multi-Pass 
----------------------------------------------------------------
Based on the original blur-gauss-v shader code.

Used to blur the outlines, which is helpful at higher internal
resolution settings to increase the line thickness.

Parameters:
-----------
Blur Weight - Vertical = Adjusts vertical blur factor.
----------------------------------------------------------------
*/

layout(push_constant) uniform Push
{
  vec4 MMJ_BlurPass_HSize;
  vec4 OriginalSize;
  float BlurWeightV;
} params;

#pragma parameter BlurWeightV "Blur Weight - Vertical" 0.0 0.0 16.0 1.0

#define MMJ_BlurPass_HSize params.MMJ_BlurPass_HSize
#define OriginalSize params.OriginalSize
#define BlurWeightV params.BlurWeightV

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
layout(set = 0, binding = 2) uniform sampler2D MMJ_BlurPass_H;

void main()
{
	vec2 PIXEL_SIZE = OriginalSize.zw;
  vec4 C = texture(MMJ_BlurPass_H, vTexCoord);
  float L = 0.0, J = 0.0;
  for(int i = 1; i <= BlurWeightV; ++i) {
    L = 1.0 / i;
    J = 0.5 * i * PIXEL_SIZE.y;
    C = mix(C, mix(texture(MMJ_BlurPass_H, vTexCoord + vec2(0.0, J)), texture(MMJ_BlurPass_H, vTexCoord - vec2(0.0, J)), 0.5), L);
  }
  FragColor = C;
}