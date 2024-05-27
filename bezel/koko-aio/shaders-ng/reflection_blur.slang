#version 450
#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) out vec2 vCoords_zoomout;
layout(location = 3) out float vDo_Curvature;

#include "includes/functions.include.slang"
#include "includes/blooms.include.slang"

void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;

   //Get fuzzy mul and pow factor
      vFuzzy_main_pass_stage_1 = apply_fuzzy_main_pass_stage_1_no_gamma_in();   
      
   //Zoom out the image coordinates by the bezel border size to make room for reflections:
      vCoords_zoomout = zoom(vTexCoord, BEZEL_REFL_ZOOMOUT_ROOM);
      
   vDo_Curvature = curvature_needed();
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) in vec2 vCoords_zoomout;
layout(location = 3) in float vDo_Curvature;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D reflected_blurred_passFeedback;
layout(set = 0, binding = 3) uniform sampler2D colortools_and_ntsc_pass;

#include "includes/functions.include.slang"
#include "includes/blooms.include.slang"


float light_fadeout(vec2 co) {
   const float shade_start = 1-BEZEL_REFL_FADEOUT_START ;  
   vec2 gradients = smoothstep_fast( vec2(BEZEL_LIGHT_NO_FADEOUT), vec2(0.5-(shade_start)   ), abs(co-0.5));
   return  (gradients.x * gradients.y ); 
}


void main() {
   
   if (DO_BEZEL == 0.0) return;
   
   //Skip unuseful pixels
      float skip_border = BEZEL_REFL_SKIP_SIZE;
      if (vCoords_zoomout.x > skip_border && vCoords_zoomout.x < 1.0 - skip_border &&
         vCoords_zoomout.y > skip_border && vCoords_zoomout.y < 1.0 - skip_border ) {
         FragColor = vec4(0.0);
         return;
      }

   float shade_sharp_blur = texture(Source, vCoords_zoomout).a;
   
   //Create gradients in the corners to blur more more near corners:
      #define CORNER_BLURRINESS 0.6
      float fcorners_shade = corners_shade(vTexCoord, 0.9) * CORNER_BLURRINESS * 0.9;
   
      shade_sharp_blur = max(fcorners_shade, shade_sharp_blur);

      float blur_radius=   (shade_sharp_blur) * 3.5  ;
      blur_radius = max(blur_radius, 1.0);
   
      float start_offset = (pi*2.0/8.0/4.0);

      vec3 pixel_out = bloom(Source, vCoords_zoomout, global.FinalViewportSize, vec2(blur_radius), 1.0, start_offset, 0.0) ;
      pixel_out *= BEZEL_REFL_STRENGTH;
      
   //Next simulate a diffused light
      vec3 pixel_diffused = textureLod( colortools_and_ntsc_pass, vCoords_zoomout, 6.0).rgb;
      pixel_diffused = pow(pixel_diffused.rgb, vec3(IN_GLOW_GAMMA));

      pixel_out = mix( pixel_out, pixel_diffused, BEZEL_DIFFUSION_STR);

   //Apply gamma out to reflections here to so that we don't need to do it in final pass (heavier).
      //pixel_out = apply_fuzzy_main_pass_stage_2(pixel_out, vFuzzy_main_pass_stage_1);
      
   //Border Reflection strength modulation
      if (vDo_Curvature > 0.0) {
         vec2 curvedco = Warp_koko(vTexCoord, vec2(-GEOM_WARP_X, -GEOM_WARP_Y), 0.5);
         pixel_out *= vec3(light_fadeout(curvedco));
      } else {
         pixel_out *= vec3(light_fadeout(vTexCoord));
      }
      

   
   
  // vec3 pixel_out_fb =  texture(reflected_blurred_passFeedback, vTexCoord).rgb;
  // pixel_out = mix(pixel_out_fb.rgb, pixel_out, 0.5 );
   

   
   FragColor = vec4(pixel_out, 1.0); 
 
   //FragColor = vec4(shade_sharp_blur);
   
}
 
 
  
