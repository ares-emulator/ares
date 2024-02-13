#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  crt-maximus-royale: A fully customizable extension for crt-royale shader,
//  inside a TV / MONITOR BOX with backlight and some other cool stuff.
//  Copyright (C) 2022 DigiDwrf
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
//  Place, Suite 330, Boston, MA 02111-1307 USA


layout(push_constant) uniform Push
{
   float crtFrame;
   float crtReflection;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   vec4 OutputSize;
   mat4 MVP;
} global;

#pragma parameter crtFrame "CRT Frame" 1.0 1.0 4.0 1.0
#pragma parameter crtReflection "CRT Reflection" 1.0 0.0 1.0 1.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 light_coord;
layout(location = 2) out vec2 frame_coord;

void main()
{
   gl_Position = global.MVP * Position;

   vec2 middle = vec2(0.5);
   vec2 center =  TexCoord.xy - middle;

   float s4_3_ratio = (global.OutputSize.y/3.0 * 4.0) / global.OutputSize.x;
   float s16_9_ratio = (global.OutputSize.y/9.0 * 16.0) / global.OutputSize.x;

   light_coord = middle + center / vec2(s4_3_ratio / 0.986 , 1.0);
   frame_coord = middle + center / vec2(s16_9_ratio , 1.0);

   vTexCoord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 light_coord;
layout(location = 2) in vec2 frame_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D TVLIGHTS1;
layout(set = 0, binding = 5) uniform sampler2D TVLIGHTS3;
layout(set = 0, binding = 6) uniform sampler2D MONITORLIGHTS;
layout(set = 0, binding = 7) uniform sampler2D BORDER_REFLECTION;

void main()
{

vec3 color = texture(Source, vTexCoord).rgb;

if (params.crtReflection == 1.0)
{

   ////DRAWING////

   vec3 lights;

   if (params.crtFrame <= 2.0) lights = texture(TVLIGHTS1, frame_coord).rgb;
   else if (params.crtFrame == 3.0) lights = texture(TVLIGHTS3, frame_coord).rgb;
   else lights = texture(MONITORLIGHTS, frame_coord).rgb;

   ////GAUSSIAN BLUR////

   const float PI = 3.14159265f;
   const float PI2 = PI*2;
   vec2 Radius;
   vec3 lightSource = vec3(0.0);

   if (lights.r > 0.01 && lights.g > 0.01 && lights.b > 0.01)
   {
      for(float d = 0.0 ; d < PI2 ; d += PI2/14)
      {
         Radius = vec2(cos(d),sin(d)); //Calculate Radius

         for(float e = 0.005 ; e < 0.03 ; e += 0.005)
            lightSource += texture(BORDER_REFLECTION, light_coord + Radius * e).rgb;
      }
   }

   lightSource /= 220;
   color += lightSource * lights;
}

////OUTPUT////

FragColor = vec4(color,1.0); //final output
}