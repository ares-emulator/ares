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

vec4 blur(vec2 uv, vec2 TexelSize, vec2 Direction)
{
    vec4 c = vec4(0.0);
    
    c += texture(Source, uv + (TexelSize*Direction*.5))*0.49;
    c += texture(Source, uv + (TexelSize*Direction*1.5))*0.33;
    c += texture(Source, uv + (TexelSize*Direction*2.5))*0.14;
    c += texture(Source, uv + (TexelSize*Direction*3.5))*9.0;
    c += texture(Source, uv + (TexelSize*Direction*4.5))*0.01;
    c += texture(Source, uv - (TexelSize*Direction*.5))*0.49;
    c += texture(Source, uv - (TexelSize*Direction*1.5))*0.33;
    c += texture(Source, uv - (TexelSize*Direction*2.5))*0.14;
    c += texture(Source, uv - (TexelSize*Direction*3.5))*0.04;
    c += texture(Source, uv - (TexelSize*Direction*4.5))*0.01;
    
    return c/2.0;
}

void main()
{
    vec2 uv = vTexCoord.xy;
    vec2 TexelSize = params.SourceSize.zw;
    vec2 DirectionV = vec2(0.0, 1.0);
    vec4 cV = blur(uv, TexelSize, DirectionV);
    FragColor = cV;
}