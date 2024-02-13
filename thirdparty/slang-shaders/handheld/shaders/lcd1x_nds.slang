#version 450

/*
   lcd1x_nds shader

   A slightly tweaked version of lcd3x:

   - Original lcd3x code written by Gigaherz and released into the public domain

   - Original 'nds_color' code written by hunterk, modified by Pokefan531 and
     released into the public domain

   Notes:

   > Omits LCD 'colour seperation' effect

   > Has 'properly' aligned scanlines

   > Includes NDS colour correction

   > Supports any NDS internal resolution setting

   Edited by jdgleaver

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.
*/

#pragma parameter BRIGHTEN_SCANLINES "Brighten Scanlines" 16.0 1.0 32.0 0.5
#pragma parameter BRIGHTEN_LCD "Brighten LCD" 4.0 1.0 12.0 0.1

layout(push_constant) uniform Push
{
   float BRIGHTEN_SCANLINES;
   float BRIGHTEN_LCD;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/*
   VERTEX_SHADER
*/
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord   = TexCoord;
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// Magic Numbers
#define PI 3.141592654

#define NDS_SCREEN_HEIGHT 192.0

#define TARGET_GAMMA 1.91
const float INV_DISPLAY_GAMMA = 1.0 / 1.91;
#define CC_LUM 0.89
#define CC_R 0.87
#define CC_G 0.645
#define CC_B 0.73
#define CC_RG 0.10
#define CC_RB 0.10
#define CC_GR 0.255
#define CC_GB 0.17
#define CC_BR -0.125
#define CC_BG 0.255

/*
   FRAGMENT SHADER
*/
void main()
{
   // Generate LCD grid effect
   // > Note the 0.25 pixel offset -> required to ensure that
   //   scanlines occur *between* pixels
   // > Divide pixel coordinate by current scale factor
   //   (input_video_height / nds_screen_height)
   vec2 pixelCoord = vTexCoord.xy * registers.SourceSize.xy;
   vec2 angle = 2.0 * PI * ((pixelCoord * NDS_SCREEN_HEIGHT * registers.SourceSize.w) - 0.25);

   float yfactor = (registers.BRIGHTEN_SCANLINES + sin(angle.y)) / (registers.BRIGHTEN_SCANLINES + 1.0);
   float xfactor = (registers.BRIGHTEN_LCD + sin(angle.x)) / (registers.BRIGHTEN_LCD + 1.0);

   // Get colour sample
   vec3 colour = texture(Source, registers.SourceSize.zw * pixelCoord.xy).rgb;

   // Apply colour correction
   colour.rgb = pow(colour.rgb, vec3(TARGET_GAMMA));
   colour.rgb = mat3(CC_R,  CC_RG, CC_RB,
                     CC_GR, CC_G,  CC_GB,
                     CC_BR, CC_BG, CC_B) * (colour.rgb * CC_LUM);
   colour.rgb = clamp(pow(colour.rgb, vec3(INV_DISPLAY_GAMMA)), 0.0, 1.0);

   // Apply LCD grid effect
   colour.rgb = yfactor * xfactor * colour.rgb;

   FragColor = vec4(colour.rgb, 1.0);
}
