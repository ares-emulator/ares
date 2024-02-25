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
   float ringAct;
   float ringDsp;
   float ringInt;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter ringAct "Ringing On" 0.0 0.0 1.0 1.0
#pragma parameter ringDsp "Ringing Displace" 3.0 0.0 3.0 0.1
#pragma parameter ringInt "Ringing Intensity" 5.0 0.0 10.0 0.5

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
vec3 screen = vec3(0.0);
float rDisp = params.ringDsp * 0.001;
float Disp;
bool rAct = bool(params.ringAct);

   if (rAct)
   {
      for (int i; i < 10 ; i++)
      {
         Disp = float(i) * rDisp;
         screen += (texture(Source, vec2(vTexCoord.x + Disp, vTexCoord.y)).xyz - texture(Source, vec2(vTexCoord.x + Disp + rDisp, vTexCoord.y)).xyz) * pow((10.0 - float(i))/10.0,5)/4.0;
      }

      for (int j; j < 10 ; j++)
      {
         Disp = float(j) * rDisp;
         screen -= (texture(Source, vec2(vTexCoord.x - Disp, vTexCoord.y)).xyz - texture(Source, vec2(vTexCoord.x - Disp - rDisp, vTexCoord.y)).xyz) * pow((10.0 - float(j))/10.0,5);
      }

       screen /= 10;
       screen *= params.ringInt;
   }

FragColor = texture(Source,vTexCoord) + vec4(screen,1.0);
}