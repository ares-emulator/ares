#version 450

/* 
A shader that tries to emulate a sony PVM type aperture grille screen but with full brightness.

The novel thing about this shader is that it relies on the HDR shaders to brighten up the image so that when 
we apply this shader which emulates the apperture grille the resulting screen isn't left too dark.  

I think you need at least a DisplayHDR 600 monitor but to get close to CRT levels of brightness I think DisplayHDR 1000.

Please Enable HDR in RetroArch 1.10+

NOTE: when this shader is envoked the Contrast, Peak Luminance and Paper White Luminance in the HDR menu do nothing instead set those values through the shader parameters 

For this shader set Paper White Luminance to above 700 and Peak Luminance to the peak luminance of your monitor.  

Also try to use a integer scaling - its just better - overscaling is fine.

This shader doesn't do any geometry warping or bouncing of light around inside the screen etc - I think these effects just add unwanted noise, I know people disagree. Please feel free to make you own and add them

Dont use this shader directly - use the hdr\crt-make-model-hdr.slangp where make and model are the make and model of the CRT you want.

THIS SHADER DOES NOT SUPPORT WRGB OLED (Due to the sub pixel layout of WRGB - RGB QD-OLED or LCD (and variants thereof screens are fine)
*/

#pragma format R16G16B16A16_SFLOAT

layout(push_constant) uniform Push
{
   float hcrt_hdr;
   float hcrt_colour_space;
   float hcrt_max_nits;
   float hcrt_paper_white_nits;
   float hcrt_lcd_resolution;
   float hcrt_lcd_subpixel;
   float hcrt_colour_system;
   float hcrt_expand_gamut;
   float hcrt_white_temperature;
   float hcrt_brightness;
   float hcrt_contrast;
   float hcrt_saturation;
   float hcrt_gamma_in;
   float hcrt_colour_accurate;
} params;


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} global;


// #include "../base/common/globals-and-screen-scale-params.inc"
// #include "../base/common/helper-functions.inc"

#include "include/parameters.h"

#define HCRT_HDR                            params.hcrt_hdr
#define HCRT_OUTPUT_COLOUR_SPACE            params.hcrt_colour_space
#define HCRT_CRT_COLOUR_SYSTEM              params.hcrt_colour_system
#define HCRT_WHITE_TEMPERATURE              params.hcrt_white_temperature
#define HCRT_BRIGHTNESS                     params.hcrt_brightness
#define HCRT_CONTRAST                       params.hcrt_contrast
#define HCRT_SATURATION                     params.hcrt_saturation
#define HCRT_GAMMA_IN                       params.hcrt_gamma_in
#define HCRT_COLOUR_ACCURATE                params.hcrt_colour_accurate

#define COMPAT_TEXTURE(c, d) texture(c, d)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * vec2(1.00001);  // To resolve rounding issues when sampling
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#include "include/colour_grade.h"

void main()
{
   vec3 source = COMPAT_TEXTURE(Source, vTexCoord).rgb;

   const vec3 colour   = ColourGrade(source);

   vec3 transformed_colour;

   if((HCRT_HDR < 1.0f) && (HCRT_COLOUR_ACCURATE < 1.0f))
   {
      if(HCRT_OUTPUT_COLOUR_SPACE == 2.0f)
      {
         transformed_colour = (colour * k709_to_XYZ) * kXYZ_to_DCIP3; 
      }
      else
      {
         transformed_colour = colour;
      }
   }
   else
   {
      transformed_colour = colour;
   }

   FragColor = vec4(transformed_colour, 1.0);
}
