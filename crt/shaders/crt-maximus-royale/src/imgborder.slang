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
   float bgMode;
   float bgR;
   float bgG;
   float bgB;
   float ledMode;
   float ledR;
   float ledG;
   float ledB;
   float ledSpeed;
   float ledBrightness;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   uint FrameCount;
} global;

#pragma parameter crtFrame "CRT Frame" 1.0 1.0 4.0 1.0
#pragma parameter bgMode "BG Mode" 0.0 0.0 3.0 1.0
#pragma parameter bgR "BG Red" 0.0 0.0 100.0 1.0
#pragma parameter bgG "BG Green" 25.0 0.0 100.0 1.0
#pragma parameter bgB "BG Blue" 50.0 0.0 100.0 1.0
#pragma parameter ledMode "Rear TV LED Mode" 0.0 0.0 3.0 1.0
#pragma parameter ledR "LED Red" 50.0 0.0 100.0 1.0
#pragma parameter ledG "LED Green" 50.0 0.0 100.0 1.0
#pragma parameter ledB "LED Blue" 50.0 0.0 100.0 1.0
#pragma parameter ledSpeed "LED Speed" 1.0 1.0 20.0 1.0
#pragma parameter ledBrightness "LED Brightness" 50.0 1.0 100.0 1.0

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 screen_coord;
layout(location = 2) out vec2 screen_coord_fix;
layout(location = 3) out vec2 frame_coord;
layout(location = 4) out float screen_ratio;

void main()
{
   gl_Position = global.MVP * Position;

   vec2 middle = vec2(0.5);
   vec2 center =  TexCoord.xy - middle;

   float s4_3_ratio = (global.OutputSize.y/3.0 * 4.0) / global.OutputSize.x;
   float s16_9_ratio = (global.OutputSize.y/9.0 * 16.0) / global.OutputSize.x;

   vec2 screen_coord_pre = center / vec2(s4_3_ratio , 1.0);

   screen_coord = middle + screen_coord_pre;
   screen_coord_fix = middle + screen_coord_pre / 0.975;

   frame_coord = middle + center / vec2(s16_9_ratio , 1.0);

   screen_ratio = global.OutputSize.x / global.OutputSize.y;
   vTexCoord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 screen_coord;
layout(location = 2) in vec2 screen_coord_fix;
layout(location = 3) in vec2 frame_coord;
layout(location = 4) in float screen_ratio;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D BGPASS2;
layout(set = 0, binding = 3) uniform sampler2D TVBORDER1;
layout(set = 0, binding = 4) uniform sampler2D TVBORDER2;
layout(set = 0, binding = 5) uniform sampler2D TVBORDER3;
layout(set = 0, binding = 6) uniform sampler2D TVBORDER2BG;
layout(set = 0, binding = 7) uniform sampler2D TVBORDER3BG;
layout(set = 0, binding = 8) uniform sampler2D TVBORDER2LED;
layout(set = 0, binding = 9) uniform sampler2D TVBORDER3LED;
layout(set = 0, binding = 10) uniform sampler2D MONITORBORDER;
layout(set = 0, binding = 11) uniform sampler2D SHAPE1;
layout(set = 0, binding = 12) uniform sampler2D SHAPE2;
layout(set = 0, binding = 13) uniform sampler2D LAST_PASS;

void main()
{

////DRAWING////

vec3 screen = texture(LAST_PASS, screen_coord_fix).rgb;
vec3 shape;
vec4 background;
vec4 backgroundBG = vec4(0.0).rgba;
vec3 backgroundLED = vec3(0.0).rgb;
vec3 BGcolor = vec3(0.0).rgb;
vec3 LEDcolor = vec3(0.0).rgb;
vec4 borders = vec4(0.0).rgba;

if (params.crtFrame == 1.0)
{
   background = texture(TVBORDER1, frame_coord);

   float Sratio = screen_ratio / (16.0/9.0);
   
   if (vTexCoord.x < 0.3)
   {
      borders = vec4(texture(TVBORDER1, vec2(vTexCoord.x * Sratio , frame_coord.y)).rgb , 0.0);
   }
   if (vTexCoord.x > 0.7)
   {
      borders = vec4(texture(TVBORDER1, vec2(1.0 + (vTexCoord.x - 1.0) * Sratio , frame_coord.y)).rgb , 0.0);
   }

   vec2 sidebar = vec2((vTexCoord.x * global.OutputSize.x - 25.0) / 5.0 , ((1.0 - vTexCoord.x) * global.OutputSize.x - 25.0) / 5.0);

   if (screen_ratio < (16.0/9.0) && screen_ratio > (4.205556/3.0))
   {
      if (sidebar.x < 0.0) { borders.a = 1.0; }
      else if (sidebar.x < 1.0) { borders.a += -sidebar.x; }

      if (sidebar.y < 0.0) { borders.a = 1.0; }
      else if (sidebar.y < 1.0) { borders.a += -sidebar.y; }
   }
}

if ((params.crtFrame == 2.0) || (params.crtFrame == 3.0))
{
   float BG_R = params.bgR / 100;
   float BG_G = params.bgG / 100;
   float BG_B = params.bgB / 100;
   float LED_R = params.ledR / 100;
   float LED_G = params.ledG / 100;
   float LED_B = params.ledB / 100;

   if ((params.bgMode > 0.0) && (params.bgMode < 3.0)) BGcolor = vec3(BG_R, BG_G, BG_B).rgb;
   if (params.bgMode > 1.0) BGcolor += texture(BGPASS2, vec2(0.0)).rgb;

   if (params.ledMode == 1.0) LEDcolor = vec3(LED_R, LED_G, LED_B).rgb;
   if (params.ledMode == 2.0)
   {
      float LED_Speed = global.FrameCount * params.ledSpeed / 1000;
      LEDcolor = vec3(sin(LED_Speed), sin(LED_Speed - 2.094395), sin(LED_Speed - 4.18879)).rgb + 1.0;
   }

   if (params.ledMode == 3.0) LEDcolor += texture(BGPASS2, vec2(0.0)).rgb * 5.0;
   if (params.ledMode >= 2.0)
   {
      float LED_Brightness = params.ledBrightness / 100;
      LEDcolor *= LED_Brightness;
   }
}

if (params.crtFrame == 2.0)
{
   background = texture(TVBORDER2, frame_coord);
   backgroundBG = texture(TVBORDER2BG, frame_coord);
   backgroundLED = texture(TVBORDER2LED, frame_coord).rgb;
}

if (params.crtFrame == 3.0)
{
   background = texture(TVBORDER3, frame_coord);
   backgroundBG = texture(TVBORDER3BG, frame_coord);
   backgroundLED = texture(TVBORDER3LED, frame_coord).rgb;
   shape = texture(SHAPE2, frame_coord).rgb;
}
else shape = texture(SHAPE1, frame_coord).rgb;

if (params.crtFrame == 4.0) background = texture(MONITORBORDER, frame_coord);

///NOISE///

vec3 grain;

float x = (vTexCoord.x + 1.0) * (vTexCoord.y + 1.0) * (mod(global.FrameCount, 1000.0) + 10.0) * 10;
grain = vec3(mod((mod(x, 10.0) + 1.0) * (mod(x, 100.0) + 1.0), 0.01) - 0.005);

////OUTPUT////

FragColor = vec4(mix(mix(mix(screen + shape + grain, background.rgb, background.a), backgroundBG.rgb * BGcolor.rgb + backgroundLED.rgb * LEDcolor.rgb, backgroundBG.a), borders.rgb, borders.a),1.0); //final output
}