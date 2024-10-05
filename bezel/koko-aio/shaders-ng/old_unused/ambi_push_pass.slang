#version 450

/* This pass apply an pre-gain to the leds on the rear of the virtual screen 
 * it is intended to produce a mipmap to be used by the next pass */


#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D first_pass;

#include "includes/functions.include.slang"

void main() {
  if (DO_AMBILIGHT != 1.0) return; 
   
   vec3 pixel_out = texture(first_pass, vTexCoord).rgb;
   pixel_out = apply_fuzzy_main_pass(pixel_out);
   pixel_out = pixel_push_luminance(pixel_out, AMBI_POWER-1);
   FragColor = vec4(pixel_out, 1.0);
}
