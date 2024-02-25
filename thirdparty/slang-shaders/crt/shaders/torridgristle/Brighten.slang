#version 450

// torridgristle CRT - Brighten pass
// by torridgristle
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float BrightenLevel;
   float BrightenAmount;
} params;

#pragma parameter BrightenLevel "Brighten Level" 2.0 1.0 10.0 1.0
#pragma parameter BrightenAmount "Brighten Amount" 0.1 0.0 1.0 0.1

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
    vec3 Picture = texture(Source, vTexCoord).xyz;
    Picture = clamp(Picture,0.0,1.0);

    FragColor = vec4(mix(Picture,1.0-pow(1.0-Picture,vec3(params.BrightenLevel)),params.BrightenAmount),1.0);
}