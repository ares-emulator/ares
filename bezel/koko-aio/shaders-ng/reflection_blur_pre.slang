#version 450
#include "config.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vCoordsAdapted;
layout(location = 2) out vec2 vCoords_shade;
#include "includes/functions.include.slang"


void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   vCoordsAdapted = vTexCoord;
   //override aspect?
   if (DO_GAME_GEOM_OVERRIDE > 0.5)
      vCoordsAdapted = content_geom_override_zoom(vCoordsAdapted, GAME_GEOM_ASPECT, get_in_aspect(), GAME_GEOM_VSHIFT, GAME_GEOM_HSHIFT, GAME_GEOM_ZOOM);

   vCoordsAdapted = zoomout_coords( vCoordsAdapted, BEZEL_RFL_ZOOM*0.1);
   
   vCoords_shade = zoom(vTexCoord,1.05);
   
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vCoordsAdapted;
layout(location = 2) in vec2 vCoords_shade;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D shift_and_bleed_pass;
layout(set = 0, binding = 2) uniform sampler2D avglum_pass;

#include "includes/functions.include.slang"
#include "includes/blooms.include.slang"

float borders_gradient(vec2 co, float smoothness) {
   vec2 gradients = 1-smoothstep_fast( vec2(0.5-smoothness), vec2(0.5), abs(co-0.5));
   return  (gradients.x * gradients.y * 2.5 - 0.5 ); 
   
   
   /*vec2 hv = smoothstep_fast( vec2(0.0), vec2(smoothness), co) * 
             (1-smoothstep_fast( vec2(1-smoothness), vec2(1.0), co));
   return  min(hv.x, hv.y)*4.0-1;
   */
   
   //Draws a square with smooth borders:   
   /*vec4 rect = vec4(smoothness, smoothness, 1 - smoothness, 1 - smoothness);
   vec2 hv = smoothstep(rect.xy - smoothness, rect.xy, co) * smoothstep(co - smoothness, co, rect.zw);
   return  hv.x * hv.y;*/
}

void main() {
   
   if (DO_BEZEL == 0.0) return;
   
   #define skip_border BEZEL_REFL_SKIP_SIZE
   if (vTexCoord.x > skip_border && vTexCoord.x < 1.0 - skip_border &&
       vTexCoord.y > skip_border && vTexCoord.y < 1.0 - skip_border ) {
      return;
   }
   
   vec2 coords = vCoordsAdapted;

   if (DO_DYNZOOM > 0.0)
      coords = zoom(coords, get_dyn_zoom(avglum_pass));

   if (AUTOCROP_MAX > 0.0)
      coords = zoom(coords, get_autocrop_zoom(avglum_pass));
   
   //eventually return black pixel
   #define tol 0.001
   #define RECT01 vec4( vec2(0.0-tol), vec2(1.0+tol) )
   if (is_first_outside_rect(coords, RECT01)) {
   //if (coords.y < 0.0-tol || coords.y > 1.0+tol || coords.x < 0.0-tol || coords.x > 1.0+tol) {
      FragColor = vec4(0.0);
      return;
   }

   
  //Create a gradient near borders to modulate between blurred and sharp refection.
      #define smoothness 0.15
      
      float shade_sharp_blur = borders_gradient(vCoords_shade, smoothness);
      shade_sharp_blur =  shade_sharp_blur - BEZEL_RFL_BLR_SHD;
      
      shade_sharp_blur=clamp(shade_sharp_blur, 0.0,1.0);
      float blur_radius=(shade_sharp_blur) * 2.0  ;
      
      vec3 pixel_out = bloom(shift_and_bleed_pass, coords, global.FinalViewportSize, vec2(blur_radius), 1.0, 0.0, 1.0) ;

   FragColor = vec4(pixel_out,shade_sharp_blur );
//FragColor = vec4(shade_sharp_blur );
   
}
 
 
  
