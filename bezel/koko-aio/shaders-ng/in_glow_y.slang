#version 450
#include "config.inc"
#include "includes/functions.include.slang"  

//Dumbify glow under this treshold.
#define GLOW_SHARP_MAX_ALTERNATE -2.0 

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float glow_h_inside_interval;
layout(location = 2) out float vDoInGlowBias;
layout(location = 3) out float vGlow_Gain;
layout(location = 4) out float vSharp_sub;
layout(location = 11) out float vDeltaRenderOk;
layout(location = 12) out float vWarpedglow;
layout(location = 13) out vec2 vWarpedglow_offsets;

void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord ;  
   
   // If * at least one sharpness is lower than the max
   //    * we dont want just blur
   //    * we don't want to show ntsc artifact masks if we're doing ntsc artifacts
   // Then do in glow bias
   vDoInGlowBias = float (  IN_GLOW_BIAS < IN_GLOW_BIAS_MAX && 
                            (abs(IN_GLOW_W) < GLOW_SHARP_MAX || IN_GLOW_H < GLOW_SHARP_MAX) &&
                            (DO_NTSC_ARTIFACTS + NTSC_SHOW_ARTF_MASK < 1.5)   );
   
   glow_h_inside_interval = float(IN_GLOW_H < GLOW_SHARP_MAX  && IN_GLOW_H > GLOW_SHARP_MAX_ALTERNATE);
   
   vGlow_Gain = mix(1.0, IN_GLOW_POWER, DO_CCORRECTION);
   
  // if (IN_GLOW_H < 0.0) {
      //Compute subtractive sharpening via blur sharpness strength:
      //For sharpness 0.45, no unsharp mask
      //For sharpness 0.8, 0.65 unsharp mask
      //float sharp_sub = (1.857 * abs(IN_GLOW_W) - 0.836);
      float in_glow_h = abs(IN_GLOW_H);
      in_glow_h = max(in_glow_h, MIN_IN_GLOW_SHARP);
      float sharp_sub = 0.8125 * in_glow_h;
      vSharp_sub = clamp(sharp_sub, 0.0,1.0);
      vSharp_sub = -2*vSharp_sub+1;
   //}
      
   bool bIs_Interlaced = is_interlaced();
   vDeltaRenderOk = is_deltarender_allowed(bIs_Interlaced); //carries DR mandatory conditions, including forced refreshed frames

   float warpedglow_scale = 1.0;
   if (DO_PIXELGRID > 0.0 && DO_PIXELGRID_H > 0.0)
      warpedglow_scale = PIXELGRID_MAX_H;
   
   float IN_GLOW_WARPSHARP_TATED = IN_GLOW_WARPSHARP_Y;
   if (need_tate(is_rotated()) == 1.0)
      IN_GLOW_WARPSHARP_TATED = IN_GLOW_WARPSHARP_X;

   vWarpedglow = (IN_GLOW_WARPSHARP_TATED * warpedglow_scale) * 0.6 / 1400.0 * 3.0;   //*3.0 needed because we scale to 0->1 to use pow in the fragment.

   float warpedglow_delta = params.OriginalSize.w * 0.5;
   vWarpedglow_offsets = vec2(0.0, +warpedglow_delta); 
      
}


#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float glow_h_inside_interval;
layout(location = 2) in float vDoInGlowBias;
layout(location = 3) in float vGlow_Gain;
layout(location = 4) in float vSharp_sub;
layout(location = 11) in float vDeltaRenderOk;
layout(location = 12) in float vWarpedglow;
layout(location = 13) in vec2 vWarpedglow_offsets;

layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 5) uniform sampler2D shift_and_bleed_pass;
layout(set = 0, binding = 6) uniform sampler2D Source;
layout(set = 0, binding = 7) uniform sampler2D in_glow_passFeedback;
layout(set = 0, binding = 8) uniform sampler2D colortools_and_ntsc_passFeedback;
layout(set = 0, binding = 9) uniform sampler2D colortools_and_ntsc_pass;

vec3 glow_blur_bias(sampler2D source_tex, vec2 co, vec3 pixel_glowed, float bias){
   vec3 pixel_source = texture(source_tex, co).rgb;
   //good with sx,sy=1.3; light gain 2.7, light sharpness 1.1
   vec3 glow_light = pixel_glowed - pixel_source;
   glow_light = clamp(glow_light * IN_GLOW_SPREAD, 0.0, 1.0);
   glow_light = pow(glow_light, vec3(IN_GLOW_SPREAD_SHARPNESS));
   vec3 proper_glow = glow_light  + pixel_source;  
   return mix(proper_glow, pixel_glowed, max(0.0, bias));            
}

vec3 blur_select_wrap(sampler2D texture_in, vec2 co, vec4 sourcesize, float sharpness, bool use_gauss) {
   if (use_gauss)
      return blur9_y(texture_in, co, sourcesize.xy, sharpness);
         else
      return blur9_y_box(texture_in, co, sourcesize.xy, sharpness, vSharp_sub);      
}



void main() { 


   //Delta render?
   float DR_changed = 1.0;
   if (DELTA_RENDER == 1.0) {
     if (vDeltaRenderOk == 1.0) {
         DR_changed = delta_render_changed(colortools_and_ntsc_passFeedback, colortools_and_ntsc_pass,
                                                      vTexCoord, DELTA_RENDER_CHECK_AREA);

     }
   }
   
   //Warped Glow Y
   //The result will be stored in the alpha channel to be consumed by final pass.
   float warpedglow_y = eps; //see down note: (**)
   if (SKIP_WARPEDGLOW_Y == 0.0) {
      if (DO_IN_GLOW == 1.0) {
         float max_inLum = max( 1.0, DO_CCORRECTION * apply_contrast_brightness(1.0, CONTRAST, BRIGHTNESS));
         vec3 gy = texture(Source, vTexCoord + vWarpedglow_offsets).rgb
               - texture(Source, vTexCoord - vWarpedglow_offsets).rgb;
               
         gy /=max_inLum;
               
         warpedglow_y = (gy.x+gy.y+gy.z) / 3.0;
                                 
         warpedglow_y = pow( abs(warpedglow_y), IN_GLOW_WARPSHARP_GAMMA) * sign(warpedglow_y);
         
         warpedglow_y = warpedglow_y *  vWarpedglow + (eps * DELTA_RENDER); 
                                 
         // (**) This eps is needed because we multiplex deltarender and warpedglow and we NEED to have dr = 0.0 (and not a positive number)
         //To reuse old pixels, only that way (using 0.0 to reuse pixels) multiplexing is possible.
      }
   }


   float warpedglow_or_DR = warpedglow_y;
   if (DR_changed == 0.0) {
      warpedglow_or_DR = DR_changed;
   }
   
   if (DO_IN_GLOW == 0.0) {
      FragColor = vec4(texture(shift_and_bleed_pass, vTexCoord).rgb * vGlow_Gain, warpedglow_or_DR);
      return;
   }

   vec3 pixel_glowed;
   
   float IN_GLOW_H_ADAPTED = abs(IN_GLOW_H);
   
   if (DO_NTSC_ARTIFACTS == 1.0) {
      float ntsc_artifacts = texture(shift_and_bleed_pass, vTexCoord).a;
      //finally "add" them to the configured sharpness (-> less blur)
      IN_GLOW_H_ADAPTED = IN_GLOW_H_ADAPTED +  ((1-ntsc_artifacts) * NTSC_ARTF_NOBLUR);
   }

   //don't allow lower values that look bad:
   //   IN_GLOW_H_ADAPTED = max(IN_GLOW_H_ADAPTED, MIN_IN_GLOW_SHARP);

   //Make it resolution independent.
      IN_GLOW_H_ADAPTED = IN_GLOW_H_ADAPTED / NEW_SCALEMOD_Y;

   //Also, a little hacky adaption for when flick_and_noise_pass does not scale to 2X
      if (params.OutputSize.xy == params.OriginalSize.xy)
         IN_GLOW_H_ADAPTED*=1.5;
      
   //If over the max sharp, then just passthrough, else do the blur.
      if (glow_h_inside_interval == 1.0) {
         pixel_glowed = blur_select_wrap(Source, vTexCoord, global.flick_and_noise_passSize, IN_GLOW_H_ADAPTED, (IN_GLOW_H >= 0.0) );
         //pixel_glowed = blur9_y(Source, vTexCoord, params.SourceSize.xy, IN_GLOW_H_ADAPTED);
      } else {
         pixel_glowed = texture(Source, vTexCoord).rgb;
      }
   
   

   
   //Series of conditions to do in glow bias done in vertex shader:
      if (vDoInGlowBias == 1.0) {
            pixel_glowed = glow_blur_bias(shift_and_bleed_pass, vTexCoord, pixel_glowed, IN_GLOW_BIAS);
            //pixel_glowed = antialias_blur_bias(shift_and_bleed_pass, vTexCoord, pixel_glowed, IN_GLOW_BIAS);
      }
   

   FragColor = vec4(pixel_glowed * vGlow_Gain, warpedglow_or_DR);
   
}

