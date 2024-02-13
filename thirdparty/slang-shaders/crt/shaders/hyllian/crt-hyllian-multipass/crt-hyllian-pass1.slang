#version 450

/*
   Hyllian's CRT Shader - pass1
  
   Copyright (C) 2011-2016 Hyllian - sergiogdb@gmail.com
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float OutputGamma;
	float PHOSPHOR;
	float COLOR_BOOST;
	float RED_BOOST;
	float GREEN_BOOST;
	float BLUE_BOOST;
	float SCANLINES_STRENGTH;
	float BEAM_MIN_WIDTH;
	float BEAM_MAX_WIDTH;
} params;

#pragma parameter OutputGamma "CRT - Output Gamma" 2.2 0.0 5.0 0.1
#pragma parameter PHOSPHOR "CRT - Phosphor ON/OFF" 1.0 0.0 1.0 1.0
#pragma parameter COLOR_BOOST "CRT - Color Boost" 1.5 1.0 2.0 0.05
#pragma parameter RED_BOOST "CRT - Red Boost" 1.0 1.0 2.0 0.01
#pragma parameter GREEN_BOOST "CRT - Green Boost" 1.0 1.0 2.0 0.01
#pragma parameter BLUE_BOOST "CRT - Blue Boost" 1.0 1.0 2.0 0.01
#pragma parameter SCANLINES_STRENGTH "CRT - Scanline Strength" 0.72 0.0 1.0 0.02
#pragma parameter BEAM_MIN_WIDTH "CRT - Min Beam Width" 0.86 0.0 1.0 0.02
#pragma parameter BEAM_MAX_WIDTH "CRT - Max Beam Width" 1.0 0.0 1.0 0.02

#define OutputGamma params.OutputGamma
#define PHOSPHOR params.PHOSPHOR
#define COLOR_BOOST params.COLOR_BOOST
#define RED_BOOST params.RED_BOOST
#define GREEN_BOOST params.GREEN_BOOST
#define BLUE_BOOST params.BLUE_BOOST
#define SCANLINES_STRENGTH params.SCANLINES_STRENGTH
#define BEAM_MIN_WIDTH params.BEAM_MIN_WIDTH
#define BEAM_MAX_WIDTH params.BEAM_MAX_WIDTH

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define GAMMA_OUT(color)   pow(color, vec3(1.0 / OutputGamma, 1.0 / OutputGamma, 1.0 / OutputGamma))

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
   vec2 texture_size = params.SourceSize.xy;

   vec3 color;
   vec2 dx = vec2(1.0/texture_size.x, 0.0);
   vec2 dy = vec2(0.0, 1.0/texture_size.y);
   vec2 pix_coord = vTexCoord*texture_size+vec2(0.0,0.5);

   vec2 tc = (floor(pix_coord)+vec2(0.0,0.5))/texture_size;

   vec2 fp = fract(pix_coord);

   vec3 color0 = texture(Source, tc - dy).xyz;
   vec3 color1 = texture(Source, tc    ).xyz;

   float pos0 = fp.y;
   float pos1 = 1 - fp.y;

   vec3 lum0 = mix(vec3(BEAM_MIN_WIDTH), vec3(BEAM_MAX_WIDTH), color0);
   vec3 lum1 = mix(vec3(BEAM_MIN_WIDTH), vec3(BEAM_MAX_WIDTH), color1);

   vec3 d0 = clamp(pos0/(lum0+0.0000001), 0.0, 1.0);
   vec3 d1 = clamp(pos1/(lum1+0.0000001), 0.0, 1.0);

   d0 = exp(-10.0*SCANLINES_STRENGTH*d0*d0);
   d1 = exp(-10.0*SCANLINES_STRENGTH*d1*d1);

   color = clamp(color0*d0+color1*d1, 0.0, 1.0);         

   color *= COLOR_BOOST*vec3(RED_BOOST, GREEN_BOOST, BLUE_BOOST);
   float mod_factor = vTexCoord.x * params.OutputSize.x;

   vec3 dotMaskWeights = mix(
                         vec3(1.0, 0.7, 1.0),
                         vec3(0.7, 1.0, 0.7),
                         floor(mod(mod_factor, 2.0))
                          );

   color.rgb *= mix(vec3(1.0, 1.0, 1.0), dotMaskWeights, PHOSPHOR);

   color  = GAMMA_OUT(color);
   FragColor = vec4(color, 1.0);
}