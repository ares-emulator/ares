#version 450

/*
   lcd1x shader

   A slightly tweaked version of lcd3x, original code written by Gigaherz
   and released into the public domain:

   > Omits LCD 'colour seperation' effect

   > Has 'properly' aligned scanlines

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

/*
   FRAGMENT SHADER
*/
void main()
{
   // Generate LCD grid effect
   // > Note the 0.25 pixel offset -> required to ensure that
   //   scanlines occur *between* pixels
   vec2 pixelCoord = vTexCoord.xy * registers.SourceSize.xy;
   vec2 angle = 2.0 * PI * (pixelCoord - 0.25);

   float yfactor = (registers.BRIGHTEN_SCANLINES + sin(angle.y)) / (registers.BRIGHTEN_SCANLINES + 1.0);
   float xfactor = (registers.BRIGHTEN_LCD + sin(angle.x)) / (registers.BRIGHTEN_LCD + 1.0);

   // Get colour sample
   vec3 colour = texture(Source, registers.SourceSize.zw * pixelCoord.xy).rgb;

   // Apply LCD grid effect
   colour.rgb = yfactor * xfactor * colour.rgb;

   FragColor = vec4(colour.rgb, 1.0);
}
