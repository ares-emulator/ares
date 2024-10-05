#version 450

#include "config.inc"


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;


void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
}


#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D in_glow_pass;
layout(set = 0, binding = 3) uniform sampler2D FXAA_pass;
layout(set = 0, binding = 4) uniform sampler2D doublesize_pass;


bool scanline_have_to_flicker(bool is_interlaced) {
    return ((scanline_flickering == 1.0) || ((scanline_flickering==2.0) && is_interlaced ));
}

void main()
{ 

   vec3 pixel_out = vec3(0.0);
   
   //Use to debug:
      //pixel_out=vec4(abs(sin(params.FrameCount/3.14/20))); //white fade
      //pixel_out=vec4(abs(sin(params.FrameCount/3.14/20)),0.0,0.0,0.0); //red fade
      //pixel_out=vec4(0.2);

   if (DO_SCANLINES == 1.0) {
      if (scanline_flickering != 0.0) {

         bool is_interlaced = (params.OriginalSize.y > MIN_LINES_INTERLACED) ? true : false ;

         if ( scanline_have_to_flicker(is_interlaced) ) {

            //Choose the right source
            if (DO_IN_GLOW == 1.0) {
               pixel_out = texture(in_glow_pass, vTexCoord).rgb;
            } else if  (DO_FXAA == 1.0) {
               pixel_out = texture(FXAA_pass, vTexCoord).rgb;
            } else {
               pixel_out = texture(doublesize_pass, vTexCoord).rgb;
            }
   
            int scanline_period;
            int scanline_period_half;

            /* Puae switches from lowres to sdres on interlaced screens
               since we blindly double the h-resolution, because we need
               to double y resolution on low resolution to simulate scanlines
               and providing sd-res to fxaa, we can check if a screen is
               interlaced by testing v-resolution of source image.
               which could be at least 200*2(puae)*2(ourselves)=800px.
               Rest assured that everything over 576,
               (maximum pal overscanned) is interlaced.
            */
            if (is_interlaced) {
               scanline_period=4;
               scanline_period_half=2;
            } else {
               scanline_period=2;
               scanline_period_half=1;
            }

            //Skip scanlines on interlaced content?
            if  (! ( is_interlaced && (scanline_disable_on_interlace == 1.0))  ) {

               float mymod = mod(vTexCoord.y * params.OutputSize.y , scanline_period);

               //Do flickering based on user prefs.
               //(blank odd lines on odd frames and even lines on even frames)
               if ((scanline_flickering == 1.0) ||
                  ((scanline_flickering==2.0) && is_interlaced )) {
                     if  (int(mod(float(params.FrameCount),2  )) <  1.0 ) {
                           if (mymod >= scanline_period_half) { pixel_out *= SCANLINE_DARK; }
                     } else {
                           if (mymod < scanline_period_half) { pixel_out *= SCANLINE_DARK; }
                     }
               }

            }
         }
      }
   }
   
   FragColor = vec4(pixel_out,1.0);

}

