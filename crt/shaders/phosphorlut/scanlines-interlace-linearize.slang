#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float percent;
	float enable_480i;
	float top_field_first;
	float input_gamma;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter input_gamma "CRT Gamma" 2.5 0.0 5.0 0.1
#pragma parameter percent "Interlacing Scanline Bright %" 0.0 0.0 1.0 0.05
#pragma parameter enable_480i "Enable 480i Mode" 1.0 0.0 1.0 1.0
#pragma parameter top_field_first "Top Field First Enable" 0.0 0.0 1.0 1.0

/*
   Interlacing
   Author: hunterk
   License: Public domain
   
   Note: This shader is designed to work with the typical interlaced output from an emulator, which displays both even and odd fields twice.
   This shader will un-weave the image, resulting in a standard, alternating-field interlacing.
*/

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
   vec4 res = texture(Source, vTexCoord).rgba;
   float y = 0.0;
   float tick = registers.FrameCount;

   // assume anything with a vertical resolution greater than 400 lines is interlaced
   if (registers.SourceSize.y > 400.0)
   {y = registers.SourceSize.y * vTexCoord.y + (tick * registers.enable_480i) + registers.top_field_first;}
   else
   {y = 2.0 * registers.SourceSize.y * vTexCoord.y + registers.top_field_first;}

   if (mod(y, 1.99999) > 0.99999)
   {res = res;}
   else
   {res = vec4(registers.percent) * res;}
   FragColor = pow(res, vec4(registers.input_gamma));
}