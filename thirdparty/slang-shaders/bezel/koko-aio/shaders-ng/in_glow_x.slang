#version 450
#include "config.inc"

//Dumbify glow under this treshold.
#define GLOW_SHARP_MAX_ALTERNATE -2.0 

#include "includes/functions.include.slang" 

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float glow_h_and_w_outside_interval;
layout(location = 2) out float glow_w_inside_interval;
layout(location = 3) out float vSharp_sub;
layout(location = 4) out float vShowArtifactsMask;

void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord ;  
   
   //IN_GLOW_H is are over the upper limit and IN_GLOW_W is outside the allowed interval;
   glow_w_inside_interval = float(IN_GLOW_W < GLOW_SHARP_MAX  && IN_GLOW_W > GLOW_SHARP_MAX_ALTERNATE);
   float glow_h_inside_interval = float(IN_GLOW_H < GLOW_SHARP_MAX  && IN_GLOW_H > GLOW_SHARP_MAX_ALTERNATE);
   
   glow_h_and_w_outside_interval = float (glow_h_inside_interval != 1.0 && glow_w_inside_interval != 1.0) ;
   
    
   //if (IN_GLOW_W < 0.0) {
      //Compute subtractive sharpening via blur sharpness strength:
      //For sharpness 0.45, no unsharp mask
      //For sharpness 0.8, 0.65 unsharp mask
      //float sharp_sub = (1.857 * abs(IN_GLOW_W) - 0.836);
      float in_glow_w = abs(IN_GLOW_W);
      in_glow_w = max(in_glow_w, MIN_IN_GLOW_SHARP);
      vSharp_sub = 0.8125 * in_glow_w;
      vSharp_sub = clamp(vSharp_sub, 0.0,1.0);
      vSharp_sub = -2*vSharp_sub+1;
   //}
      
   vShowArtifactsMask = float(DO_NTSC_ARTIFACTS + NTSC_SHOW_ARTF_MASK > 1.5);
   
}


#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float glow_h_and_w_outside_interval;
layout(location = 2) in float glow_w_inside_interval;
layout(location = 3) in float vSharp_sub;
layout(location = 4) in float vShowArtifactsMask;
layout(location = 5) in float vDeltaRenderOk;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 5) uniform sampler2D colortools_and_ntsc_pass;
layout(set = 0, binding = 6) uniform sampler2D colortools_and_ntsc_passFeedback;
layout(set = 0, binding = 7) uniform sampler2D shift_and_bleed_pass;


vec3 blur_select_wrap(sampler2D texture_in, vec2 co, vec4 sourcesize, float sharpness, bool use_gauss) {
   if (use_gauss)
      return blur9_x(texture_in, co, sourcesize.xy, sharpness);
         else
      return blur9_x_box(texture_in, co, sourcesize.xy, sharpness, vSharp_sub);
}



void main() { 

   //Modulate glow power via ntsc artifact mask:
      float ntsc_artifacts = texture(shift_and_bleed_pass, vTexCoord).a; //compiler is smart enough to reuse this lookup, no need to branch.
      
      //To debug ntsc artifacts mask:
      if (vShowArtifactsMask == 1.0) {
         FragColor = vec4(ntsc_artifacts);
         return;
      }
   
   //Don't do anything if IN_GLOW_H is are over the upper limit
   //and IN_GLOW_W is outside the allowed interval;
      if ( glow_h_and_w_outside_interval == 1.0) {
         vec4 passthrough = texture(shift_and_bleed_pass, vTexCoord);
         //Delta render?
         FragColor = passthrough;
         return;
      }
      

   //Modulate glow power via ntsc artifact mask:
   //"add" them to the configured sharpness (-> less blur)
         //float in_glow_w_adapted = abs(IN_GLOW_W);
         //in_glow_w_adapted = in_glow_w_adapted +  DO_NTSC_ARTIFACTS * ((1-ntsc_artifacts) * NTSC_ARTF_NOBLUR);
         //Same as before, but faster (ASM PROOF)
         float in_glow_w_adapted = fma(
                                        DO_NTSC_ARTIFACTS,
                                        fma(-NTSC_ARTF_NOBLUR, ntsc_artifacts, NTSC_ARTF_NOBLUR),
                                        abs(IN_GLOW_W)
                                    );
      

   //don't allow lower values that look bad:
      //in_glow_w_adapted = max(in_glow_w_adapted, MIN_IN_GLOW_SHARP);

      
   //Scale it for box
      if (IN_GLOW_W < 0.0)
         in_glow_w_adapted = in_glow_w_adapted/1.2;
   

   //Make it resolution independent.
      in_glow_w_adapted = in_glow_w_adapted / NEW_SCALEMOD_X;
      

   //Finally blur horizontally 
   if (glow_w_inside_interval == 1.0)  {
         FragColor = vec4(blur_select_wrap(shift_and_bleed_pass, vTexCoord, global.flick_and_noise_passSize, in_glow_w_adapted, (IN_GLOW_W >= 0.0) ), 
                          0.0);
   } else {
      FragColor = vec4(texture(shift_and_bleed_pass, vTexCoord).rgb, 0.0);
   }

   
}

