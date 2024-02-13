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
   float bgMode;
   float ledMode;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter bgMode "BG Mode" 0.0 0.0 3.0 1.0
#pragma parameter ledMode "Rear TV LED Mode" 0.0 0.0 3.0 1.0

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
layout(set = 0, binding = 2) uniform sampler2D maximus_refpass;

void main()
{
   vec3 BColor = vec3(0.0).rgb;
   if ((params.bgMode > 1.0) || (params.ledMode == 3.0)) for (float i = 0.02; i <= 1.0; i += 0.02) BColor += texture(maximus_refpass, vec2(i,vTexCoord.y)).rgb;
   FragColor = vec4(BColor/50, 1.0).rgba;
}
