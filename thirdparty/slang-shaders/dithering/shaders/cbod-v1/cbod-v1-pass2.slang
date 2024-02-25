#version 450

/*
/   "Conditional Blending of Dither" Shader v1.0 - Pass 2
/   Copyright (c) 2013, Alexander Kulagin <coastkid3d@gmail.com>
/   All Rights reserved.
/ 
/   Redistribution and use in source and binary forms, with or without
/   modification, are permitted provided that the following conditions are met:
/ 
/     * Redistributions of source code must retain the above copyright notice,
/       this list of conditions and the following disclaimer.
/ 
/     * Redistributions in binary form must reproduce the above copyright
/       notice, this list of conditions and the following disclaimer in the
/       documentation and/or other materials provided with the distribution.
/ 
/   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
/   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
/   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
/   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
/   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
/   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
/   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
/   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
/   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
/   POSSIBILITY OF SUCH DAMAGE.
*/

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

void main()
{
  vec2 uv = vTexCoord - (params.SourceSize.zw) * 0.25;
  vec2 uv_shift = (params.SourceSize.zw);
	vec3 src = texture(Source, uv).rgb;
 
  // Searching for the Horizontal Dithering Zones
	vec3 dither_h_zone = vec3(texture(Source, uv + vec2(0., uv_shift.y)).rgb == texture(Source, uv - vec2(0., uv_shift.y)).rgb);
  dither_h_zone = vec3(smoothstep(0.2, 1.0, dot(dither_h_zone, vec3(0.33333))));
  
  // Searching for High Contrast "Safe" Zones 
  vec3 safe_zone = vec3(abs(dot(texture(Source, uv).rgb - texture(Source, uv - vec2(0., uv_shift.y)).rgb, vec3(0.3333))));
  safe_zone = vec3(lessThan(safe_zone , vec3(0.45)));
 
  // Vertical Bluring by 1 pixel
  vec3 blur_v = (texture(Source, uv).rgb + texture(Source, uv - vec2(0., uv_shift.y)).rgb) * 0.5;
  
  // Final Blend between Source and Blur using Dithering Zone and Safe Zone
  vec3 finalcolor = mix(src, blur_v, dither_h_zone * safe_zone);
  
   FragColor = vec4(finalcolor, 1.0);
}