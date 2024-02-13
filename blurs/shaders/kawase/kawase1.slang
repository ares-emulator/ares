#version 450

// Kuwase Blur
// based on a shadertoy by Kubuxu
// https://www.shadertoy.com/view/Xl3XW7

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

vec4 reSample(float d, vec2 uv, sampler2D decal)
{
 
    vec2 step1 = (vec2(d) + 0.5) * params.SourceSize.zw;
    vec4 color = vec4(0.);
    color += texture(decal, uv + step1) / 4.;
    color += texture(decal,  uv - step1) / 4.;
  	vec2 step2 = step1;
    step2.x = -step2.x;
    color += texture(decal, uv + step2) / 4.;
    color += texture(decal,  uv - step2) / 4.;
    return color;
}

void main()
{
   FragColor = reSample(1., vTexCoord, Source);
}