#version 450

/*
   Fast Sharpen Shader (Custom)
   
   Copyright (C) 2005 - 2022 guest(r) - guest.r@gmail.com

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float CSHARPEN, CCONTR, CDETAILS;
} params;

#pragma parameter CSHARPEN  "Sharpen strength"       0.00 0.0 5.00 0.10 
#pragma parameter CCONTR    "Ammount of sharpening"  0.05 0.0 0.25 0.01 
#pragma parameter CDETAILS  "Details sharpened "     1.00 0.0 1.00 0.05 

#define CSHARPEN params.CSHARPEN
#define CCONTR params.CCONTR
#define CDETAILS params.CDETAILS

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
	vec2 g01 = vec2(-1.0, 0.0)*params.SourceSize.zw;
	vec2 g21 = vec2( 1.0, 0.0)*params.SourceSize.zw; 

	vec3 c01 = texture(Source, vTexCoord + g01).rgb;
	vec3 c21 = texture(Source, vTexCoord + g21).rgb;
	vec3 c11 = texture(Source, vTexCoord      ).rgb;
	
	vec3 b11 = 0.5*(c01+c21); 
	
	float contrast = max(max(c11.r,c11.g),c11.b);
	contrast = mix(2.0*CCONTR, CCONTR, contrast);
	
	vec3 mn1 = min(c01,c21); mn1 = min(mn1,c11*(1.0-contrast));
	vec3 mx1 = max(c01,c21); mx1 = max(mx1,c11*(1.0+contrast));
	
	vec3 dif = pow(mx1-mn1+0.0001, vec3(0.75,0.75,0.75));
	vec3 sharpen = mix(vec3(CSHARPEN*CDETAILS), vec3(CSHARPEN), dif);
	
	c11 = clamp(mix(c11,b11,-sharpen), mn1,mx1);
	
	FragColor = vec4(c11,1.0); 
}
