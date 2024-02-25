#version 450

/*
   lcd1x_psp shader

   A slightly tweaked version of lcd3x:

   - Original lcd3x code written by Gigaherz and released into the public domain

   - Original 'psp_color' code written by hunterk, modified by Pokefan531 and
     released into the public domain

   Notes:

   > Omits LCD 'colour seperation' effect

   > Has 'properly' aligned scanlines

   > Includes PSP colour correction

   > Supports any PSP internal resolution setting

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

#define PSP_SCREEN_HEIGHT 272.0

#define TARGET_GAMMA 2.21
const float INV_DISPLAY_GAMMA = 1.0 / 2.2;
#define CC_R 0.98
#define CC_G 0.795
#define CC_B 0.98
#define CC_RG 0.04
#define CC_RB 0.01
#define CC_GR 0.20
#define CC_GB 0.01
#define CC_BR -0.18
#define CC_BG 0.165

/*
   FRAGMENT SHADER
*/
void main()
{
   // Generate LCD grid effect
   // > Note the 0.25 pixel offset -> required to ensure that
   //   scanlines occur *between* pixels
   // > Divide pixel coordinate by current scale factor
   //   (input_video_height / psp_screen_height)
   vec2 pixelCoord = vTexCoord.xy * registers.SourceSize.xy;
   vec2 angle = 2.0 * PI * ((pixelCoord * PSP_SCREEN_HEIGHT * registers.SourceSize.w) - 0.25);

   float yfactor = (registers.BRIGHTEN_SCANLINES + sin(angle.y)) / (registers.BRIGHTEN_SCANLINES + 1.0);
   float xfactor = (registers.BRIGHTEN_LCD + sin(angle.x)) / (registers.BRIGHTEN_LCD + 1.0);

   // Get colour sample
   vec3 colour = texture(Source, registers.SourceSize.zw * pixelCoord.xy).rgb;

   // Apply colour correction
   colour.rgb = pow(colour.rgb, vec3(TARGET_GAMMA));
   colour.rgb = mat3(CC_R,  CC_RG, CC_RB,
                     CC_GR, CC_G,  CC_GB,
                     CC_BR, CC_BG, CC_B) * colour.rgb;
   colour.rgb = clamp(pow(colour.rgb, vec3(INV_DISPLAY_GAMMA)), 0.0, 1.0);

   // Apply LCD grid effect
   colour.rgb = yfactor * xfactor * colour.rgb;

   FragColor = vec4(colour.rgb, 1.0);
}
