#version 450

// CRT-torridgristle - Candy Bloom
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float GlowLevel;
   float GlowTightness;
} params;

#pragma parameter GlowLevel "Glow Level" 1.0 0.0 1.0 0.1
#pragma parameter GlowTightness "Glow Tightness" 0.5 0.0 1.0 0.1

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
layout(set = 0, binding = 3) uniform sampler2D candy_ref;

void main()
{
	vec3 Picture = texture(Source, vTexCoord).xyz;

    float MaxRGB       = max(Picture.x, max(Picture.y, Picture.z));
    float MinRGB       = min(Picture.x, min(Picture.y, Picture.z));

    float YIQLuminance = ((0.299*Picture.x) + (0.587*Picture.y) + (0.114*Picture.z));
    
    
    Picture /= MaxRGB;
    Picture = clamp(Picture,0.0,1.0);
    
    
    FragColor = vec4(mix(texture(candy_ref, vTexCoord).xyz, Picture, mix(1.-pow(1.-YIQLuminance,2.0),YIQLuminance*YIQLuminance,params.GlowTightness)*params.GlowLevel),1.0);
}