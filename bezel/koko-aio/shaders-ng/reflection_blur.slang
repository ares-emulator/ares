#version 450
#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) out vec2 vCoords_zoomout;
layout(location = 3) out float vDo_Curvature;
layout(location = 4) out vec2 vTexCoord_rotated_maybe;
layout(location = 5) out vec3 vShadesUDS;

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
   
   
   //...to Change bezel sides luminance
      #define _shade_up   -0.11
      #define _shade_side -0.04
      #define _shade_down +0.02
     
      vTexCoord_rotated_maybe = vTexCoord;
      vShadesUDS.x = _shade_up;
      vShadesUDS.y = _shade_down;
      vShadesUDS.z = _shade_side;
     
      // multiply by 4 because the lut driving it has been divided by 4.0
      // This would eventually allow to modify the lut to alter the shades.
      vShadesUDS *= 4.0;
     
      vTexCoord_rotated_maybe = vTexCoord;
     
      if (params.Rotation == 1.0) { //90
         vTexCoord_rotated_maybe.xy = vTexCoord.yx; //rotate helper lut
         vShadesUDS.xy = vShadesUDS.yx; //swap shade up and shade down.
      } else
      if (params.Rotation == 3.0) { // 270
         vTexCoord_rotated_maybe.xy = vTexCoord.yx; //rotate helper lut
      }

      //slight adaption to match bezel aspect ratio:
         vTexCoord_rotated_maybe.x = zoom1D(vTexCoord_rotated_maybe.x, 0.965);
         
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vFuzzy_main_pass_stage_1;
layout(location = 2) in vec2 vCoords_zoomout;
layout(location = 3) in float vDo_Curvature;
layout(location = 4) in vec2 vTexCoord_rotated_maybe;
layout(location = 5) in vec3 vShadesUDS;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D colortools_and_ntsc_pass;
layout(set = 0, binding = 3) uniform sampler2D sideshade;

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
         FragColor.rgb = vec3(0.0);
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

      pixel_out += pixel_diffused * BEZEL_DIFFUSION_STR;

   //Apply gamma out to reflections here to so that we don't need to do it in final pass (heavier).
      pixel_out = apply_fuzzy_main_pass_stage_2(pixel_out, vFuzzy_main_pass_stage_1);
     
   //Fadeout reflected lighe
      if (vDo_Curvature > 0.0) {
         vec2 curvedco = Warp_koko(vTexCoord, vec2(-GEOM_WARP_X, -GEOM_WARP_Y), 0.5);
         pixel_out *= vec3(light_fadeout(curvedco));
      } else {
         pixel_out *= vec3(light_fadeout(vTexCoord));
      }


   // Load a lut containing 3 colored triangles red,blue,green.
   // that identifies screen sides, we use them to shade the reflection area
      vec3 side_shade_lut_DRUL = textureLod(sideshade, vTexCoord_rotated_maybe.xy, 0.0).rgb;
      float side_shade = dot(side_shade_lut_DRUL, vShadesUDS.yzx );
         
   //Corners: less reflections:
      float fcorners_dark = 1 - corners_shade(vTexCoord, 1.125) * BEZEL_CORNER_DARK ;

   
   FragColor.rgb = (pixel_out * fcorners_dark)+side_shade;

}