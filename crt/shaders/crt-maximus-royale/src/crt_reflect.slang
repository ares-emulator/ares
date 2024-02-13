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
   float Xscale;
   float Yscale;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter Xscale "Horizontal Scale %" 100.0 75.0 150.0 0.1
#pragma parameter Yscale "Vertical Scale %" 100.0 75.0 150.0 0.1
#pragma parameter crtFrame "CRT Frame" 1.0 1.0 4.0 1.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 screen_coord;
layout(location = 2) out vec2 resize_coord;

void main()
{
   gl_Position = global.MVP * Position;
   vec2 center =  TexCoord.xy - 0.5;
   screen_coord = center / vec2(0.955,0.98);
   resize_coord = vec2(params.Xscale,params.Yscale) * 0.01;
   vTexCoord = TexCoord.xy;
   if (params.crtFrame == 4.0) screen_coord /= vec2(1.0,0.985);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 screen_coord;
layout(location = 2) in vec2 resize_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D maximus_refpass;

void main()
{

////CALCULATE REFLEXION COORDS 1st pass////

float curveFactor = 0.14;
float scaleFactor = 1.0;
float borderAdjustment = 0.99;

if (params.crtFrame == 3.0) scaleFactor = 0.835;
if (params.crtFrame == 4.0)
{
   curveFactor = 0.1;
   scaleFactor = 0.92;
   borderAdjustment = 1.035;
}

vec2 LightsCoordPre = screen_coord/cos(screen_coord.yx * vec2(3.0,4.0) * curveFactor) / scaleFactor;

vec2 LightsCoord = 0.5 + LightsCoordPre / borderAdjustment;
vec2 LightsCoordScale = 0.5 + LightsCoordPre / resize_coord;

vec2 LightsCoordInv = 1.0 - LightsCoord;
vec2 LightsCoordHalf = LightsCoord / 2.0;
vec2 LightsCoordHalfInv = LightsCoordHalf - 0.5;

vec2 rezCoords = 0.5 / resize_coord * borderAdjustment - 0.5;

////CALCULATE REFLEXION COORDS 2nd pass////

vec2 newLightsCoord = LightsCoord;

if (LightsCoordScale.x > -0.05 && LightsCoordScale.x < 1.05 && LightsCoordScale.y > -0.05 && LightsCoordScale.y < 1.05)
{
   if (LightsCoord.x < 0.0 && -LightsCoordHalf.x < LightsCoord.y && -LightsCoordHalf.x < LightsCoordInv.y)
   {
      newLightsCoord.x = -LightsCoord.x - rezCoords.x;
      newLightsCoord.y *= 1.0 - LightsCoord.x + rezCoords.y * 2.0;
      newLightsCoord.y += LightsCoord.x / 2.0 - rezCoords.y;
   }

   if (LightsCoord.x > 1.0 && LightsCoordHalfInv.x < LightsCoord.y && LightsCoordHalfInv.x < LightsCoordInv.y)
   {
      newLightsCoord.x = 2.0 - LightsCoord.x + rezCoords.x;
      newLightsCoord.y *= 1.0 - LightsCoordInv.x + rezCoords.y * 2.0;
      newLightsCoord.y += LightsCoordInv.x / 2.0 - rezCoords.y;
   }

   if (LightsCoord.y < 0.0 && -LightsCoordHalf.y < LightsCoord.x && -LightsCoordHalf.y < LightsCoordInv.x)
   {
      newLightsCoord.y = -LightsCoord.y - rezCoords.y;
      newLightsCoord.x *= 1.0 - LightsCoord.y + rezCoords.x * 2.0;
      newLightsCoord.x += LightsCoord.y / 2.0 - rezCoords.x;
   }

   if (LightsCoord.y > 1.0 && LightsCoordHalfInv.y < LightsCoord.x && LightsCoordHalfInv.y < LightsCoordInv.x)
   {
      newLightsCoord.y = 2.0 - LightsCoord.y + rezCoords.y;
      newLightsCoord.x *= 1.0 - LightsCoordInv.y + rezCoords.x * 2.0;
      newLightsCoord.x += LightsCoordInv.y / 2.0 - rezCoords.x;
   }
}

if (LightsCoord.x > 0.0  && LightsCoord.x < 1.0 && LightsCoord.y > 0.0  && LightsCoord.y < 1.0) { newLightsCoord = LightsCoordScale; }

vec4 lightSource = texture(maximus_refpass, newLightsCoord).rgba;

FragColor = lightSource; //final output
}
