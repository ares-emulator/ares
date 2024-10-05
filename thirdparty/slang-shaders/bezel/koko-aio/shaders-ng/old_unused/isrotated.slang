#version 450
#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float mark_color_rotated;

#include "includes/functions.include.slang"
void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   if (is_rotated())
      mark_color_rotated = 1.0;
         else
      mark_color_rotated = 0.0;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float mark_color_rotated;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform sampler2D Source;


void main() {
   FragColor.r = mark_color_rotated;
}
 
  
