#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#include "params.inc"

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
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D SamplerBloom4;

#include "frag_funcs.inc"

void main()
{
   vec4 bloom = vec4(0.);
   bloom.xyz = GaussBlur22(vTexCoord, SamplerBloom4, 16., 0., true).xyz * 2.5;   
   bloom.w = GaussBlur22(vTexCoord, SamplerBloom4, 32 * global.fAnamFlareWideness, 0., false).w * 2.5; // to have anamflare texture (bloom.w) avoid vertical blur
   FragColor = bloom;
}