#version 450

/*
   Hyllian's jinc windowed-jinc 2-lobe with anti-ringing Shader
   
   Copyright (C) 2011-2014 Hyllian/Jararaca - sergiogdb@gmail.com

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

      /*
         This is an approximation of Jinc(x)*Jinc(x*r1/r2) for x < 2.5,
         where r1 and r2 are the first two zeros of jinc function.
         For a jinc 2-lobe best approximation, use A=0.5 and B=0.825.
      */  

// A=0.5, B=0.825 is the best jinc approximation for x<2.5. if B=1.0, it's a lanczos filter.
// Increase A to get more blur. Decrease it to get a sharper picture. 
// B = 0.825 to get rid of dithering. Increase B to get a fine sharpness, though dithering returns.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float JINC_SHARP;
} params;

#pragma parameter JINC_SHARP "Sharpness" 1.0 1.0 3.0 1.0
#define JINC_SHARP params.JINC_SHARP

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const float pi = 3.1415926535897932384626433832795;

// Calculates the distance between two points
float d(vec2 pt1, vec2 pt2)
{
  vec2 v = pt2 - pt1;
  return sqrt(dot(v,v));
}

vec3 min4(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return min(a, min(b, min(c, d)));
}

vec3 max4(vec3 a, vec3 b, vec3 c, vec3 d)
{
    return max(a, max(b, max(c, d)));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float wa;
layout(location = 2) out float wb;
layout(location = 3) out float JINC2_AR_STRENGTH;
layout(location = 4) out vec2 pc;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * vec2(1.0001);

   float JINC2_WINDOW_SINC, JINC2_SINC;

   if(int(floor(JINC_SHARP + 0.5)) == 1)
   {
      JINC2_WINDOW_SINC = 0.405;
      JINC2_SINC = 0.79;
   }
   if(int(floor(JINC_SHARP + 0.5)) == 2)
   {
      JINC2_WINDOW_SINC = 0.377;
      JINC2_SINC = 0.82;
   }
   if(int(floor(JINC_SHARP + 0.5)) == 3)
   {
      JINC2_WINDOW_SINC = 0.329;
      JINC2_SINC = 0.87;
   }
   JINC2_AR_STRENGTH = 0.8;
   wa = JINC2_WINDOW_SINC*pi;
   wb = JINC2_SINC*pi;
   pc = vTexCoord*params.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float wa;
layout(location = 2) in float wb;
layout(location = 3) in float JINC2_AR_STRENGTH;
layout(location = 4) in vec2 pc;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec4 resampler(vec4 x)
{
   vec4 res;

   res.x = (x.x==0.0) ?  wa*wb  :  sin(x.x*wa)*sin(x.x*wb)/(x.x*x.x);
   res.y = (x.y==0.0) ?  wa*wb  :  sin(x.y*wa)*sin(x.y*wb)/(x.y*x.y);
   res.z = (x.z==0.0) ?  wa*wb  :  sin(x.z*wa)*sin(x.z*wb)/(x.z*x.z);
   res.w = (x.w==0.0) ?  wa*wb  :  sin(x.w*wa)*sin(x.w*wb)/(x.w*x.w);

   return res;
}

void main()
{
    vec3 color;
    vec4 weights[4];

    vec2 dx = vec2(1.0, 0.0);
    vec2 dy = vec2(0.0, 1.0);

    vec2 tc = (floor(pc-vec2(0.5,0.5))+vec2(0.5,0.5));

    weights[0] = resampler(vec4(d(pc, tc    -dx    -dy), d(pc, tc           -dy), d(pc, tc    +dx    -dy), d(pc, tc+2.0*dx    -dy)));
    weights[1] = resampler(vec4(d(pc, tc    -dx       ), d(pc, tc              ), d(pc, tc    +dx       ), d(pc, tc+2.0*dx       )));
    weights[2] = resampler(vec4(d(pc, tc    -dx    +dy), d(pc, tc           +dy), d(pc, tc    +dx    +dy), d(pc, tc+2.0*dx    +dy)));
    weights[3] = resampler(vec4(d(pc, tc    -dx+2.0*dy), d(pc, tc       +2.0*dy), d(pc, tc    +dx+2.0*dy), d(pc, tc+2.0*dx+2.0*dy)));

    dx = dx * params.SourceSize.zw;
    dy = dy * params.SourceSize.zw;
    tc = tc * params.SourceSize.zw;

    vec3 c00 = texture(Source, tc    -dx    -dy).xyz;
    vec3 c10 = texture(Source, tc           -dy).xyz;
    vec3 c20 = texture(Source, tc    +dx    -dy).xyz;
    vec3 c30 = texture(Source, tc+2.0*dx    -dy).xyz;
    vec3 c01 = texture(Source, tc    -dx       ).xyz;
    vec3 c11 = texture(Source, tc              ).xyz;
    vec3 c21 = texture(Source, tc    +dx       ).xyz;
    vec3 c31 = texture(Source, tc+2.0*dx       ).xyz;
    vec3 c02 = texture(Source, tc    -dx    +dy).xyz;
    vec3 c12 = texture(Source, tc           +dy).xyz;
    vec3 c22 = texture(Source, tc    +dx    +dy).xyz;
    vec3 c32 = texture(Source, tc+2.0*dx    +dy).xyz;
    vec3 c03 = texture(Source, tc    -dx+2.0*dy).xyz;
    vec3 c13 = texture(Source, tc       +2.0*dy).xyz;
    vec3 c23 = texture(Source, tc    +dx+2.0*dy).xyz;
    vec3 c33 = texture(Source, tc+2.0*dx+2.0*dy).xyz;

    color = texture(Source, vTexCoord).xyz;

    //  Get min/max samples
    vec3 min_sample = min4(c11, c21, c12, c22);
    vec3 max_sample = max4(c11, c21, c12, c22);
/*
      color = mat4x3(c00, c10, c20, c30) * weights[0];
      color+= mat4x3(c01, c11, c21, c31) * weights[1];
      color+= mat4x3(c02, c12, c22, c32) * weights[2];
      color+= mat4x3(c03, c13, c23, c33) * weights[3];
      mat4 wgts = mat4(weights[0], weights[1], weights[2], weights[3]);
      vec4 wsum = wgts * vec4(1.0,1.0,1.0,1.0);
      color = color/(dot(wsum, vec4(1.0,1.0,1.0,1.0)));
*/


    color = vec3(dot(weights[0], vec4(c00.x, c10.x, c20.x, c30.x)), dot(weights[0], vec4(c00.y, c10.y, c20.y, c30.y)), dot(weights[0], vec4(c00.z, c10.z, c20.z, c30.z)));
    color+= vec3(dot(weights[1], vec4(c01.x, c11.x, c21.x, c31.x)), dot(weights[1], vec4(c01.y, c11.y, c21.y, c31.y)), dot(weights[1], vec4(c01.z, c11.z, c21.z, c31.z)));
    color+= vec3(dot(weights[2], vec4(c02.x, c12.x, c22.x, c32.x)), dot(weights[2], vec4(c02.y, c12.y, c22.y, c32.y)), dot(weights[2], vec4(c02.z, c12.z, c22.z, c32.z)));
    color+= vec3(dot(weights[3], vec4(c03.x, c13.x, c23.x, c33.x)), dot(weights[3], vec4(c03.y, c13.y, c23.y, c33.y)), dot(weights[3], vec4(c03.z, c13.z, c23.z, c33.z)));
    color = color/(dot(weights[0], vec4(1,1,1,1)) + dot(weights[1], vec4(1,1,1,1)) + dot(weights[2], vec4(1,1,1,1)) + dot(weights[3], vec4(1,1,1,1)));

    // Anti-ringing
    vec3 aux = color;
    color = clamp(color, min_sample, max_sample);
    color = mix(aux, color, JINC2_AR_STRENGTH);

    // final sum and weight normalization
    FragColor.xyz = color;
}
