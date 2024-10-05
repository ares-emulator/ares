#version 450
#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord ;  
}


#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 3) uniform sampler2D FXAA_pass;
layout(set = 0, binding = 4) uniform sampler2D first_pass;
layout(set = 0, binding = 5) uniform sampler2D shift_and_bleed_pass;


// Blur sizes must not depend on input resolution
#define scalemod_x (params.OutputSize.x/360.0)
#define scalemod_y (params.OutputSize.y/270.0)

#include "includes/pixel_glows.include.slang" 



vec3 glow_blur_bias_smother_bloomer(sampler2D source_tex, vec2 co, vec3 pixel_glowed, float bias) {
   //Modulates the mix of the blurred version over the sharp one
   //by the luminosity difference of the blurred version and the original
   vec3 pixel_original = texture(source_tex,vTexCoord).rgb;
   vec3 pixel_sharp  = pow(pixel_original,vec3(IN_GLOW_GAMMA)) * IN_GLOW_POWER;
   
   vec3 pixel_glowed_c = clamp(pixel_glowed, 0.0, 1.0);
   vec3 pixel_sharp_c = clamp(pixel_sharp, 0.0, 1.0);
   
   vec3 vmix = vec3( pixel_glowed_c.r - pixel_original.r,
                     pixel_glowed_c.g - pixel_original.g,
                     pixel_glowed_c.b - pixel_original.b);

   vmix += IN_GLOW_BIAS;
   vmix = clamp(vmix, 0.0, 1.0);
   vmix *= vmix;
   return mix(pixel_sharp, pixel_glowed, vmix );
   //return mix(pixel_sharp, pixel_glowed, clamp(   (vmix+0.01 * vInGlowBias) , 0.0,1.0)   );
   //return pixel_sharp + pixel_glowed*clamp(   (vmix+0.01 * vInGlowBias) , 0.0,1.0);
}

vec3 glow_blur_bias_sharper_AA(sampler2D source_tex, vec2 co, vec3 pixel_glowed, float bias) {
   //from latitude. with pow.
   vec3 pixel_source = texture(source_tex, co).rgb;
   pixel_source  = pow(pixel_source,vec3(IN_GLOW_GAMMA)) * IN_GLOW_POWER;
   
   float glowlum = max(max(pixel_glowed.r, pixel_glowed.g), pixel_glowed.b);
   float orilum =  max(max(pixel_source.r, pixel_source.g), pixel_source.b);
   glowlum = clamp(glowlum, 0.0, 1.0);
   orilum =  clamp(orilum, 0.0, 1.0);
   float g_mix = (glowlum - orilum);
   g_mix += IN_GLOW_BIAS;
   g_mix = clamp(g_mix, 0.0, 1.0);
   g_mix *= g_mix ;
   return mix(pixel_source, pixel_glowed, g_mix);
}

vec3 glow_blur_bias(sampler2D source_tex, vec2 co, vec3 pixel_glowed, float bias){
   vec3 pixel_source = texture(source_tex, co).rgb;
   pixel_source  = pow(pixel_source,vec3(IN_GLOW_GAMMA)) * IN_GLOW_POWER;
   vec3 glow_light = clamp((pixel_glowed - pixel_source) * IN_GLOW_SPREAD, 0.0, 1.0);

   /* powering glow_light is fine when not dealing with scanline minimum thickness,
    * but then the scanline shape gets smaller, it kinda produces a visive
    * pow(x,2) by its own; in that case powering glow_light make things worse.
    * It makes sense, because we are emulating the light spread twice.
    * FIXME: How to deal with this?
    */

   vec3 proper_glow = glow_light * glow_light + pixel_source;

   return mix(proper_glow, pixel_glowed, max(0.0, bias));   
   
   
   /*if (IN_GLOW_W < 0.99)
      return  glow_blur_bias_sharper_AA( source_tex,  co,  pixel_glowed,  bias);
         else
      return  glow_blur_bias_smother_bloomer( source_tex,  co,  pixel_glowed,  bias);
   */
}




void main() { 
 
   
   
   if (DO_IN_GLOW == 0.0) return;
   
   vec3 pixel_glowed;
   if ( DO_SHIFT_RGB + DO_SAT_BLEED > 0.0) {
      pixel_glowed = pixel_glow(shift_and_bleed_pass, IN_GLOW_W, IN_GLOW_H, IN_GLOW_POWER, IN_GLOW_GAMMA, vTexCoord, params.OutputSize, params.OutputSize).rgb;
       if (IN_GLOW_BIAS < IN_GLOW_BIAS_MAX && (IN_GLOW_W < GLOW_SHARP_MAX || IN_GLOW_H < GLOW_SHARP_MAX) )
          pixel_glowed = glow_blur_bias(shift_and_bleed_pass, vTexCoord, pixel_glowed, IN_GLOW_BIAS);
      
      }
   else if (DO_FXAA > 0.0) {
      pixel_glowed = pixel_glow(FXAA_pass, IN_GLOW_W, IN_GLOW_H, IN_GLOW_POWER, IN_GLOW_GAMMA, vTexCoord, params.OutputSize, params.OutputSize).rgb;
       if (IN_GLOW_BIAS < IN_GLOW_BIAS_MAX &&  (IN_GLOW_W < GLOW_SHARP_MAX || IN_GLOW_H < GLOW_SHARP_MAX) )
          pixel_glowed = glow_blur_bias(FXAA_pass, vTexCoord, pixel_glowed, IN_GLOW_BIAS);
      
   }
   else {
      pixel_glowed = pixel_glow(first_pass, IN_GLOW_W, IN_GLOW_H, IN_GLOW_POWER, IN_GLOW_GAMMA, vTexCoord, params.OutputSize, params.OutputSize).rgb;
      if (IN_GLOW_BIAS < IN_GLOW_BIAS_MAX &&  (IN_GLOW_W < GLOW_SHARP_MAX || IN_GLOW_H < GLOW_SHARP_MAX) )
          pixel_glowed = glow_blur_bias(first_pass, vTexCoord, pixel_glowed, IN_GLOW_BIAS);
      
   }



   FragColor = vec4((pixel_glowed),1.0);
}

