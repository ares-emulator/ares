#version 450

/*
    Phosphor shader - Copyright (C) 2011 caligari.

    Ported by Hyllian.

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
	float SPOT_WIDTH;
	float SPOT_HEIGHT;
	float COLOR_BOOST;
	float InputGamma;
	float OutputGamma;
} params;

// 0.5 = the spot stays inside the original pixel
// 1.0 = the spot bleeds up to the center of next pixel
#pragma parameter SPOT_WIDTH "CRTCaligari Spot Width" 0.9 0.5 1.5 0.05
#pragma parameter SPOT_HEIGHT "CRTCaligari Spot Height" 0.65 0.5 1.5 0.05
// Used to counteract the desaturation effect of weighting.
#pragma parameter COLOR_BOOST "CRTCaligari Color Boost" 1.45 1.0 2.0 0.05
// Constants used with gamma correction.
#pragma parameter InputGamma "CRTCaligari Input Gamma" 2.4 0.0 5.0 0.1
#pragma parameter OutputGamma "CRTCaligari Output Gamma" 2.2 0.0 5.0 0.1

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 onex;
layout(location = 2) out vec2 oney;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   onex = vec2(params.SourceSize.z, 0.0);
   oney = vec2(0.0, params.SourceSize.w);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 onex;
layout(location = 2) in vec2 oney;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define GAMMA_IN(color)     pow(color,vec4(params.InputGamma))
#define GAMMA_OUT(color)    pow(color, vec4(1.0 / params.OutputGamma))

#define TEX2D(coords)	GAMMA_IN( texture(Source, coords) )

// Macro for weights computing
#define WEIGHT(w) \
   if(w>1.0) w=1.0; \
w = 1.0 - w * w; \
w = w * w;

void main()
{
   vec2 coords = ( vTexCoord * params.SourceSize.xy );
   vec2 pixel_center = floor( coords ) + vec2(0.5, 0.5);
   vec2 texture_coords = pixel_center * params.SourceSize.zw;

   vec4 color = TEX2D( texture_coords );

   float dx = coords.x - pixel_center.x;

   float h_weight_00 = dx / params.SPOT_WIDTH;
   WEIGHT( h_weight_00 );

   color *= vec4( h_weight_00, h_weight_00, h_weight_00, h_weight_00  );

   // get closest horizontal neighbour to blend
   vec2 coords01;
   if (dx>0.0) {
      coords01 = onex;
      dx = 1.0 - dx;
   } else {
      coords01 = -onex;
      dx = 1.0 + dx;
   }
   vec4 colorNB = TEX2D( texture_coords + coords01 );

   float h_weight_01 = dx / params.SPOT_WIDTH;
   WEIGHT( h_weight_01 );

   color = color + colorNB * vec4( h_weight_01 );

   //////////////////////////////////////////////////////
   // Vertical Blending
   float dy = coords.y - pixel_center.y;
   float v_weight_00 = dy / params.SPOT_HEIGHT;
   WEIGHT( v_weight_00 );
   color *= vec4( v_weight_00 );

   // get closest vertical neighbour to blend
   vec2 coords10;
   if (dy>0.0) {
      coords10 = oney;
      dy = 1.0 - dy;
   } else {
      coords10 = -oney;
      dy = 1.0 + dy;
   }
   colorNB = TEX2D( texture_coords + coords10 );

   float v_weight_10 = dy / params.SPOT_HEIGHT;
   WEIGHT( v_weight_10 );

   color = color + colorNB * vec4( v_weight_10 * h_weight_00, v_weight_10 * h_weight_00, v_weight_10 * h_weight_00, v_weight_10 * h_weight_00 );

   colorNB = TEX2D(  texture_coords + coords01 + coords10 );

   color = color + colorNB * vec4( v_weight_10 * h_weight_01, v_weight_10 * h_weight_01, v_weight_10 * h_weight_01, v_weight_10 * h_weight_01 );

   color *= vec4( params.COLOR_BOOST );

   FragColor = clamp( GAMMA_OUT(color), 0.0, 1.0 );
}
