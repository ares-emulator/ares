/*
   Fast Sharpen Shader
   
   Copyright (C) 2005 - 2023 guest(r) - guest.r@gmail.com

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
	vec4 DerezedPassSize;
	float SHARPEN_ON;
	float CSHARPEN, CCONTR, CDETAILS, DEBLUR;
} params;

#pragma parameter SHARPEN_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter SHARPEN_TITLE  	"[ FAST SHARPEN - GUEST.R ]:"         			0.00 0.0 2.00 0.05
#pragma parameter SHARPEN_ON  		"          FSharpen - ON"       		1 0 1 1

#pragma parameter CSHARPEN  		"          FSharpen - Sharpen strength"         0.00 0.0 5.00 0.10 
#pragma parameter CCONTR    		"          FSharpen - Sharpen Contrast/Ringing" 0.05 0.0 0.25 0.01 
#pragma parameter CDETAILS  		"          FSharpen - Details sharpened"        1.00 0.0 1.00 0.05 
#pragma parameter DEBLUR    		"          FSharpen - Deblur Strength (For NTSC Only)"          1.00 1.0 7.00 0.25 

#define SHARPEN_ON params.SHARPEN_ON
#define CSHARPEN params.CSHARPEN
#define CCONTR params.CCONTR
#define CDETAILS params.CDETAILS
#define DEBLUR params.DEBLUR

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

#ifdef IS_NTSC_PRESET
	layout(set = 0, binding = 3) uniform sampler2D PrePass0;
#else
	#define PrePass0 Source
#endif

const mat3 yiq2rgb_mat = mat3(
   1.0, 0.956, 0.6210,
   1.0, -0.2720, -0.6474,
   1.0, -1.1060, 1.7046);

vec3 yiq2rgb(vec3 yiq)
{
   return yiq * yiq2rgb_mat;
}

const mat3 yiq_mat = mat3(
      0.2989, 0.5870, 0.1140,
      0.5959, -0.2744, -0.3216,
      0.2115, -0.5229, 0.3114
);

vec3 rgb2yiq(vec3 col)
{
   return col * yiq_mat;
}

void main()
{
	if ( SHARPEN_ON < 0.5 )
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

	vec2 g01 = vec2(-0.5*params.DerezedPassSize.z, 0.0);
	vec2 g21 = vec2( 0.5*params.DerezedPassSize.z, 0.0); 

	vec3 c01 = texture(Source, vTexCoord + g01).rgb;
	vec3 c21 = texture(Source, vTexCoord + g21).rgb;
	vec3 c11 = texture(Source, vTexCoord      ).rgb;
	
	vec3 b11 = 0.5*(c01+c21); 
	
	float contrast = max(max(c11.r,c11.g),c11.b);
	contrast = mix(2.0*CCONTR, CCONTR, contrast);
	
	vec3 mn = min(min(c01,c21),c11); vec3 mn1 = min(mn,c11*(1.0-contrast));
	vec3 mx = max(max(c01,c21),c11); vec3 mx1 = max(mx,c11*(1.0+contrast));
	
	vec3 dif = pow(mx1-mn1+0.0001, vec3(0.75,0.75,0.75));
	vec3 sharpen = mix(vec3(CSHARPEN*CDETAILS), vec3(CSHARPEN), dif);
	
	vec3 res = clamp(mix(c11,b11,-sharpen), mn1,mx1);

#ifdef IS_NTSC_PRESET
if (DEBLUR > 1.125) 
{
	c01 = texture(PrePass0, vTexCoord + 2.0*g01).rgb;
	c21 = texture(PrePass0, vTexCoord + 2.0*g21).rgb;
	c11 = texture(PrePass0, vTexCoord      ).rgb;

	mn1 = sqrt(min(min(c01,c21), c11)*mn);
	mx1 = sqrt(max(max(c01,c21), c11)*mx);

	vec3 dif1 = max(res-mn1, 0.0) + 0.00001; dif1 = pow(dif1, DEBLUR.xxx);
	vec3 dif2 = max(mx1-res, 0.0) + 0.00001; dif2 = pow(dif2, DEBLUR.xxx);
	
	vec3 ratio = dif1/(dif1+dif2);
	sharpen  = min(mix(mn1, mx1, ratio), pow(res,mix(0.75.xxx,1.10.xxx, res)));
	res = rgb2yiq(res);
	res.x = dot(sharpen,vec3(0.2989, 0.5870, 0.1140));
	res = max(yiq2rgb(res),0.0);
}
#endif
	
	FragColor = vec4(res,1.0); 
}
