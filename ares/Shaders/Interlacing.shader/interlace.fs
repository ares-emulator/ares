#version 150

/*
   Interlacing
   Author: hunterk
   License: Public domain
   
   Note: This shader is designed to work with the typical interlaced output from an emulator, which displays both even and odd fields twice.
   This shader will un-weave the image, resulting in a standard, alternating-field interlacing.
*/

uniform sampler2D source[];
uniform vec2 targetSize;
uniform vec4 sourceSize[];
uniform int phase;

in Vertex {
  vec2 texCoord;
};

out vec4 fragColor;

// This controls how bright the intermediate lines are; 0.0 = totally black
#define percent 0.0

void main()
{
vec4 res = texture(source[0], texCoord);
   float y = 0.0;

   // assume anything with a vertical resolution greater than 400 lines is interlaced
   if (sourceSize[0].y > 400.0) y = sourceSize[0].y * texCoord.y + phase;
   else
      y = 2.00001 * sourceSize[0].y * texCoord.y;

   if (mod(y, 2.0) > 0.99999) fragColor = res;
   else
      fragColor = vec4(percent) * res;
}
