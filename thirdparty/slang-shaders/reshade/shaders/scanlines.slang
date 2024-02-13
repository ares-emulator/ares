#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float opacity;
} params;

#pragma parameter opacity "Scanlines dark" 50.0 0.0 100.0 1.0
#define opacity params.opacity

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
   vec4 frame = texture(Source, vTexCoord);
   vec2 fragpos   = floor(vTexCoord.xy*params.OutputSize.xy);
   vec4 background;
   if (mod(fragpos.y,2.0) == 0.0) background = vec4(0.0, 0.0, 0.0, 1.0); else background = vec4(1.0, 1.0, 1.0, 0.0);
   if (params.OutputSize.y > 1600) { if (mod(fragpos.y,4.0) < 2.0) background = vec4(0.0, 0.0, 0.0, 1.0); else background = vec4(1.0, 1.0, 1.0, 0.0);} 
   background.a *= opacity / 100.0;
   FragColor = mix(frame, background, background.a);
}