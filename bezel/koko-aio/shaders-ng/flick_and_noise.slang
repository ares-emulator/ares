#version 450

#include "config.inc"
#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float vDynamicSeed;
layout(location = 2) out float vFlickering_power;
layout(location = 3) out float vDo_flickering;
layout(location = 4) out vec2  vFlickerCoords;

#include "includes/functions.include.slang"


void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   //Generate a seed that changes over time for temporal random noise
      vDynamicSeed = mod(params.FrameCount, 120.0001);
      
   vDo_flickering  = float ( scanline_have_to_flicker(is_interlaced()) ) ; 
   
   if (vDo_flickering == 1.0) {
      //Flicker one over 3 frames:
         float ModFlicker = params.FrameCount % 3;
      
      //Compute y flicker offset:
         float FlickerOffset = 0.0;
         float line_tick = is_interlaced() ? 1 : 2 ;
         if (ModFlicker == 1.0 )
            FlickerOffset = params.OriginalSize.w/line_tick;
         else if (ModFlicker == 2.0)
            FlickerOffset = -params.OriginalSize.w/line_tick;
      
      //Export flicker coords for Fragment shader here:
         vFlickerCoords =  vec2(TexCoord.x, TexCoord.y + FlickerOffset);
         
      vFlickering_power = PIXELGRID_INTR_FLICK_POWR;
   }

}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float vDynamicSeed;
layout(location = 2) in float vFlickering_power;
layout(location = 3) in float vDo_flickering;
layout(location = 4) in vec2  vFlickerCoords;

layout(location = 0) out vec4 FragColor;  

layout(set = 0, binding = 2) uniform sampler2D Source;

#include "includes/functions.include.slang"

vec4 pixel_flickering() {
/* Simulates the flickering effect of the interlaced screens.
 * As I remember, it was visible when a line and the next had high
 * luminosity differences.
 * So we need to sample the current line and the previous one
 * (eventually applying color corrections to both).
 *
 * Repeating the following:
 * On frame 0, return the "clean" pixel
 * On frame 1, mix the upper pixel with the current one
 * On frame 2, mix the lower pixel with the current one
 *
 * The effect of the mix is the flickering itself, and we modulate
 * the mix according to the luminance difference between the current
 * pixel and the mixed one.
 *
 * We choose to alternate on a period of 3,
 * (thus considering the upper pixel and the lower one)
 * or else the high pixel persistance of lcd displays wont allow
 * to see the effect (the lcd panel would just mix the pixels by itself (meh).
 */

   vec4 pixel_cur = texture_NEAREST(Source,vTexCoord, params.OriginalSize);   
   vec3 flickline = texture_NEAREST(Source,vFlickerCoords, params.OriginalSize).rgb;
   
   //float lumdiff = abs( flickline.r + flickline.g + flickline.b - pixel_cur.r - pixel_cur.g - pixel_cur.b);
   //float lumdiff = abs( (flickline.r + flickline.g) + (flickline.b - pixel_cur.r) - (pixel_cur.g + pixel_cur.b) ); //1687 1754 2946   ASM PROOF: FASTER
   float lumdiff = abs( dot(flickline.rgb, vec3(1.0)) - dot(pixel_cur.rgb, vec3(1.0)));// 1685 1750 2948 //ASM PROOF: Faster on SIMD8 and SIMD16, slower on SIMD32
      
   lumdiff = min(lumdiff * vFlickering_power, 1.0);
   return vec4( mix(pixel_cur.rgb,flickline.rgb,lumdiff),
                pixel_cur.a); //<-- wee need to return the alpha channel because it carries ntsc artifact mask
}


void main() {
   
   vec4 pixel_out;
   
   if (vDo_flickering==1.0) 
      pixel_out = pixel_flickering(); //<-alpha carries ntsc artifacts mask
   else 
      pixel_out = texture_NEAREST(Source, vTexCoord, params.OriginalSize); //<-alpha carries ntsc artifacts mask
   
   if (DO_RF_NOISE > 0.0) {
      float anoise = random_fast(vTexCoord * vDynamicSeed);
       pixel_out.rgb += scale_to_range(anoise, -RF_NOISE_STRENGTH, RF_NOISE_STRENGTH);
   }
   FragColor = pixel_out;
}


