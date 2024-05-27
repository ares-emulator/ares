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
layout(location = 12) out float vInflation;
layout(location = 13) out vec2 vInflation_offsets;

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

   //FIXME check for d3d compatibility.
   bool rotated = (abs((global.final_passFeedbackSize.x/global.final_passFeedbackSize.y) - (global.FinalViewportSize.x/global.FinalViewportSize.y)) > ar_tolerance);
   
   float Inflation_delta;
   if ( need_tate(rotated) == 1.0 ) {
      Inflation_delta = params.OriginalSize.z * 0.5;
      vInflation_offsets = vec2(+Inflation_delta, 0.0);
   } else {
      Inflation_delta = params.OriginalSize.w * 0.5;
      vInflation_offsets = vec2(0.0, +Inflation_delta); 
   }
   vInflation = (PIXELGRID_INFLATION * PIXELGRID_MAX_H * 0.25) * Inflation_delta;
   
}


#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float glow_h_inside_interval;
layout(location = 2) in float vDoInGlowBias;
layout(location = 3) in float vGlow_Gain;
layout(location = 4) in float vSharp_sub;
layout(location = 11) in float vDeltaRenderOk;
layout(location = 12) in float vInflation;
layout(location = 13) in vec2 vInflation_offsets;

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

/*   warpsharp experiment
   vec2 d = params.OriginalSize.zw*2.0;

   vec2 sharpness = d*SERVICE1;
   
   vec3 gx = texture(Original, vec2(vTexCoord.x + d.x, vTexCoord.y)).rgb - 
             texture(Original, vec2(vTexCoord.x - d.x, vTexCoord.y)).rgb;

   vec3 gy = texture(Original, vec2(vTexCoord.x, vTexCoord.y + d.y)).rgb - 
             texture(Original, vec2(vTexCoord.x, vTexCoord.y - d.y)).rgb;
      
  
   vec3 color = texture(Original, vTexCoord).rgb;
   
   float fgx = +(gx.x+gx.y+gx.z)/3.0;
   float fgy = +(gy.x+gy.y+gy.z)/3.0;
   

   fgx = clamp(fgx, -SERVICE1, SERVICE1);
   fgy = clamp(fgy, -SERVICE1, SERVICE1);
   
    vec2 warp = vec2(fgx, fgy) * sharpness;
    vec2 warpedTexCoord = vTexCoord + warp;
    
    vec4 warpedColor = texture(Original, warpedTexCoord);
    
    FragColor = vec4(fgx*fgy);
    
    //FragColor = warpedColor;
    
    return;
*/   

   //Delta render?
   float DR_changed = 1.0;
   if (DELTA_RENDER == 1.0) {
     if (vDeltaRenderOk == 1.0) {
         DR_changed = delta_render_changed(colortools_and_ntsc_passFeedback, colortools_and_ntsc_pass,
                                                      vTexCoord, DELTA_RENDER_CHECK_AREA);

     }
   }
   float max_inLum = max( 1.0, DO_CCORRECTION * apply_contrast_brightness(1.0, CONTRAST, BRIGHTNESS));
   //Scanlines (over)inflation.
   //The result will be stored in the alpha channel to be consumed by final pass.
  
      vec3 gy = texture(Source, vTexCoord + vInflation_offsets).rgb
              - texture(Source, vTexCoord - vInflation_offsets).rgb;
              
      gy /=max_inLum;
              
      float scanline_inflation = (gy.x+gy.y+gy.z) * (vInflation * global.FinalViewportSize.y) 
      + (eps * DELTA_RENDER); 
      //This eps is needed because we multiplex deltarender and inflection and we NEED to have dr = 0.0 (and not a positive number)
      //To reuse old pixels, only that way (using 0.0 to reuse pixels) multiplexing is possible.
      


   float inflation_or_DR = scanline_inflation;
   if (DR_changed == 0.0) {
      inflation_or_DR = DR_changed;
   }
   
   if (DO_IN_GLOW == 0.0) {
      FragColor = vec4(texture(shift_and_bleed_pass, vTexCoord).rgb * vGlow_Gain, inflation_or_DR);
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
   
   
   
   //It over the max sharp, then just passthrough, else do the blur.
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
   

   FragColor = vec4(pixel_glowed * vGlow_Gain, inflation_or_DR);
   
}

