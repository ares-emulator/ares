#version 450

/*
    Hyllian's CRT Shader

    Copyright (C) 2011-2024 Hyllian - sergiogdb@gmail.com

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float CRT_OutputGamma;
    float PHOSPHOR_LAYOUT;
    float MASK_INTENSITY;
    float MONITOR_SUBPIXELS;
    float BRIGHTBOOST;
    float SCANLINES_SHAPE;
    float SCANLINES_STRENGTH;
    float BEAM_MIN_WIDTH;
    float BEAM_MAX_WIDTH;
    float POST_BRIGHTNESS;
    float CURVATURE;
    float WARP_X;
    float WARP_Y;
    float CORNER_SIZE;
    float CORNER_SMOOTHNESS;
} params;

#pragma parameter CRT_OutputGamma "OUTPUT GAMMA" 2.2 1.0 5.0 0.05
#pragma parameter PHOSPHOR_LAYOUT "PHOSPHOR LAYOUT [1-6 Aperture, 7-10 Shadow, 11-14 Slot]" 1.0 0.0 15.0 1.0
#pragma parameter MASK_INTENSITY "MASK INTENSITY" 0.65 0.0 1.0 0.01
#pragma parameter MONITOR_SUBPIXELS "MONITOR SUBPIXELS LAYOUT [0=RGB, 1=BGR]" 0.0 0.0 1.0 1.0
#pragma parameter BRIGHTBOOST "BRIGHTNESS BOOST" 1.40 1.0 3.0 0.05
#pragma parameter BEAM_MIN_WIDTH "MIN BEAM WIDTH" 0.86 0.0 1.0 0.02
#pragma parameter BEAM_MAX_WIDTH "MAX BEAM WIDTH" 1.0 0.0 1.0 0.02
#pragma parameter SCANLINES_STRENGTH "SCANLINES STRENGTH" 0.72 0.0 1.0 0.01
#pragma parameter SCANLINES_SHAPE "SCANLINES SHAPE (SINC | GAUSSIAN)" 0.0 0.0 1.0 1.0
#pragma parameter POST_BRIGHTNESS "POST-BRIGHTNESS" 1.00 1.0 3.0 0.05
#pragma parameter CURVATURE "ENABLE CURVATURE" 0.0 0.0 1.0 1.0
#pragma parameter WARP_X "  CURVATURE-X" 0.015 0.0 0.125 0.005
#pragma parameter WARP_Y "  CURVATURE-Y" 0.015 0.0 0.125 0.005
#pragma parameter CORNER_SIZE "  CORNER SIZE" 0.02 0.001 1.0 0.005
#pragma parameter CORNER_SMOOTHNESS "  CORNER SMOOTHNESS" 1.10 1.0 2.2 0.02


#define CRT_OutputGamma params.CRT_OutputGamma
#define BRIGHTBOOST (params.BRIGHTBOOST+1.1)
#define SCANLINES_SHAPE params.SCANLINES_SHAPE
#define SCANLINES_STRENGTH (-0.16*SCANLINES_SHAPE+params.SCANLINES_STRENGTH)
#define BEAM_MIN_WIDTH params.BEAM_MIN_WIDTH
#define BEAM_MAX_WIDTH params.BEAM_MAX_WIDTH
#define POST_BRIGHTNESS params.POST_BRIGHTNESS
#define CURVATURE params.CURVATURE
#define WARP_X params.WARP_X
#define WARP_Y params.WARP_Y
#define CORNER_SIZE params.CORNER_SIZE
#define CORNER_SMOOTHNESS (80.0*pow(params.CORNER_SMOOTHNESS,10.0))


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define GAMMA_OUT(color)   pow(color, vec3(1.0 / CRT_OutputGamma, 1.0 / CRT_OutputGamma, 1.0 / CRT_OutputGamma))

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord*1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


const vec2 corner_aspect   = vec2(1.0,  0.75);

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * corner_aspect;
    vec2 cdist = vec2(CORNER_SIZE);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
    return clamp((cdist.x - dist)*CORNER_SMOOTHNESS, 0.0, 1.0);
}

/* Curvature code. Credits to torridgristle! */
vec2 CRT_Distortion = vec2(0.0, WARP_Y) * 15.;

#define SQRT_OF_2  1.4142135623730950488016887242097

// Radius of Convergence = 1.0 - SQRT_OF_2 / 2

#define CONVERGENCE_RADIUS 0.29289321881345247559915563789515

vec2 Warp(vec2 texCoord)
{
   vec2 cCoords = texCoord * 2.0 - 1.0;
   float cCoordsDist = sqrt(cCoords.x * cCoords.x + cCoords.y * cCoords.y);
   cCoords = cCoords / cCoordsDist;
   cCoords = cCoords * (1.0 - pow(vec2(1.0 - (cCoordsDist/SQRT_OF_2)),(1.0/(1.0+CRT_Distortion*0.2))));
   cCoords = cCoords / (1.0-pow(vec2(CONVERGENCE_RADIUS),(1.0/(vec2(1.0)+CRT_Distortion*0.2))));
   cCoords = cCoords * 0.5 + 0.5;

   return cCoords;
}

/* Mask code pasted from subpixel_masks.h. Masks 3 and 4 added. */
vec3 mask_weights(vec2 coord, float mask_intensity, int phosphor_layout, float monitor_subpixels){
   vec3 weights = vec3(1.,1.,1.);
   float on = 1.;
   float off = 1.-mask_intensity;
   vec3 red     = monitor_subpixels==1.0 ? vec3(on,  off, off) : vec3(off, off, on );
   vec3 green   = vec3(off, on,  off);
   vec3 blue    = monitor_subpixels==1.0 ? vec3(off, off, on ) : vec3(on,  off, off);
   vec3 magenta = vec3(on,  off, on );
   vec3 yellow  = monitor_subpixels==1.0 ? vec3(on,  on,  off) : vec3(off, on,  on );
   vec3 cyan    = monitor_subpixels==1.0 ? vec3(off, on,  on ) : vec3(on,  on,  off);
   vec3 black   = vec3(off, off, off);
   vec3 white   = vec3(on,  on,  on );
   int w, z = 0;
   
   // This pattern is used by a few layouts, so we'll define it here
   vec3 aperture_weights = mix(magenta, green, floor(mod(coord.x, 2.0)));
   
   if(phosphor_layout == 0) return weights;

   else if(phosphor_layout == 1){
      // classic aperture for RGB panels; good for 1080p, too small for 4K+
      // aka aperture_1_2_bgr
      weights  = aperture_weights;
      return weights;
   }

   else if(phosphor_layout == 2){
      // Classic RGB layout; good for 1080p and lower
      vec3 bw3[3] = vec3[](red, green, blue);
//      vec3 bw3[3] = vec3[](black, yellow, blue);
      
      z = int(floor(mod(coord.x, 3.0)));
      
      weights = bw3[z];
      return weights;
   }

   else if(phosphor_layout == 3){
      // black and white aperture; good for weird subpixel layouts and low brightness; good for 1080p and lower
      vec3 bw3[3] = vec3[](black, white, black);
      
      z = int(floor(mod(coord.x, 3.0)));
      
      weights = bw3[z];
      return weights;
   }

   else if(phosphor_layout == 4){
      // reduced TVL aperture for RGB panels. Good for 4k.
      // aperture_2_4_rgb
      
      vec3 big_ap_rgb[4] = vec3[](red, yellow, cyan, blue);
      
      w = int(floor(mod(coord.x, 4.0)));
      
      weights = big_ap_rgb[w];
      return weights;
   }
   
   else if(phosphor_layout == 5){
      // black and white aperture; good for weird subpixel layouts and low brightness; good for 4k 
      vec3 bw4[4] = vec3[](black, black, white, white);
      
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = bw4[z];
      return weights;
   }

   else if(phosphor_layout == 6){
      // aperture_1_4_rgb; good for simulating lower 
      vec3 ap4[4] = vec3[](red, green, blue, black);
      
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = ap4[z];
      return weights;
   }

   else if(phosphor_layout == 7){
      // 2x2 shadow mask for RGB panels; good for 1080p, too small for 4K+
      // aka delta_1_2x1_bgr
      vec3 inverse_aperture = mix(green, magenta, floor(mod(coord.x, 2.0)));
      weights               = mix(aperture_weights, inverse_aperture, floor(mod(coord.y, 2.0)));
      return weights;
   }

   else if(phosphor_layout == 8){
      // delta_2_4x1_rgb
      vec3 delta[2][4] = {
         {red, yellow, cyan, blue},
         {cyan, blue, red, yellow}
      };
      
      w = int(floor(mod(coord.y, 2.0)));
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = delta[w][z];
      return weights;
   }

   else if(phosphor_layout == 9){
      // delta_1_4x1_rgb; dunno why this is called 4x1 when it's obviously 4x2 /shrug
      vec3 delta1[2][4] = {
         {red,  green, blue, black},
         {blue, black, red,  green}
      };
      
      w = int(floor(mod(coord.y, 2.0)));
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = delta1[w][z];
      return weights;
   }
   
   else if(phosphor_layout == 10){
      // delta_2_4x2_rgb
      vec3 delta[4][4] = {
         {red,  yellow, cyan, blue},
         {red,  yellow, cyan, blue},
         {cyan, blue,   red,  yellow},
         {cyan, blue,   red,  yellow}
      };
      
      w = int(floor(mod(coord.y, 4.0)));
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = delta[w][z];
      return weights;
   }

   else if(phosphor_layout == 11){
      // slot mask for RGB panels; looks okay at 1080p, looks better at 4K
      vec3 slotmask[4][6] = {
         {red, green, blue,    red, green, blue,},
         {red, green, blue,  black, black, black},
         {red, green, blue,    red, green, blue,},
         {black, black, black, red, green, blue,}
      };
      
      w = int(floor(mod(coord.y, 4.0)));
      z = int(floor(mod(coord.x, 6.0)));

      // use the indexes to find which color to apply to the current pixel
      weights = slotmask[w][z];
      return weights;
   }

   else if(phosphor_layout == 12){
      // slot mask for RGB panels; looks okay at 1080p, looks better at 4K
      vec3 slotmask[4][6] = {
         {black,  white, black,   black,  white, black,},
         {black,  white, black,  black, black, black},
         {black,  white, black,  black,  white, black,},
         {black, black, black,  black,  white, black,}
      };
      
      w = int(floor(mod(coord.y, 4.0)));
      z = int(floor(mod(coord.x, 6.0)));

      // use the indexes to find which color to apply to the current pixel
      weights = slotmask[w][z];
      return weights;
   }

   else if(phosphor_layout == 13){
      // based on MajorPainInTheCactus' HDR slot mask
      vec3 slot[4][8] = {
         {red,   green, blue,  black, red,   green, blue,  black},
         {red,   green, blue,  black, black, black, black, black},
         {red,   green, blue,  black, red,   green, blue,  black},
         {black, black, black, black, red,   green, blue,  black}
      };
      
      w = int(floor(mod(coord.y, 4.0)));
      z = int(floor(mod(coord.x, 8.0)));
      
      weights = slot[w][z];
      return weights;
   }

   else if(phosphor_layout == 14){
      // same as above but for RGB panels
      vec3 slot2[4][10] = {
         {red,   yellow, green, blue,  blue,  red,   yellow, green, blue,  blue },
         {black, green,  green, blue,  blue,  red,   red,    black, black, black},
         {red,   yellow, green, blue,  blue,  red,   yellow, green, blue,  blue },
         {red,   red,    black, black, black, black, green,  green, blue,  blue }
      };
   
      w = int(floor(mod(coord.y, 4.0)));
      z = int(floor(mod(coord.x, 10.0)));
      
      weights = slot2[w][z];
      return weights;
   }
   
   else if(phosphor_layout == 15){
      // slot_3_7x6_rgb
      vec3 slot[6][14] = {
         {red,   red,   yellow, green, cyan,  blue,  blue,  red,   red,   yellow, green,  cyan,  blue,  blue},
         {red,   red,   yellow, green, cyan,  blue,  blue,  red,   red,   yellow, green,  cyan,  blue,  blue},
         {red,   red,   yellow, green, cyan,  blue,  blue,  black, black, black,  black,  black, black, black},
         {red,   red,   yellow, green, cyan,  blue,  blue,  red,   red,   yellow, green,  cyan,  blue,  blue},
         {red,   red,   yellow, green, cyan,  blue,  blue,  red,   red,   yellow, green,  cyan,  blue,  blue},
         {black, black, black,  black, black, black, black, black, red,   red,    yellow, green, cyan,  blue}
      };
      
      w = int(floor(mod(coord.y, 6.0)));
      z = int(floor(mod(coord.x, 14.0)));
      
      weights = slot[w][z];
      return weights;
   }


   
   else return weights;
}




#define pi    3.1415926535897932384626433832795
#define wa    (0.5*pi)
#define wb    (pi)

vec3 resampler3(vec3 x)
{
    vec3 res;

    res.x = (x.x<=0.001) ?  1.0  :  sin(x.x*wa)*sin(x.x*wb)/(wa*wb*x.x*x.x);
    res.y = (x.y<=0.001) ?  1.0  :  sin(x.y*wa)*sin(x.y*wb)/(wa*wb*x.y*x.y);
    res.z = (x.z<=0.001) ?  1.0  :  sin(x.z*wa)*sin(x.z*wb)/(wa*wb*x.z*x.z);

    return res;
}

vec3 get_scanlines(vec3 d0, vec3 d1, vec3 color0, vec3 color1)
{
    if (SCANLINES_SHAPE > 0.5) {
        d0 = exp(-16.0*d0*d0);
        d1 = exp(-16.0*d1*d1);
    }
    else {
        d0 = clamp(2.0*d0, 0.0, 1.0);
        d1 = clamp(2.0*d1, 0.0, 1.0);
        d0 = resampler3(d0);
        d1 = resampler3(d1);
    }

    return (color0*d0+color1*d1);
}

void main()
{
    vec2 texture_size = params.SourceSize.xy;

    vec3 color;
    vec2 dy = vec2(0.0, 1.0/texture_size.y);

    vec2 WarpedTexCoord = vTexCoord.xy;

    WarpedTexCoord = (CURVATURE > 0.5) ? Warp(WarpedTexCoord) : WarpedTexCoord;
 
    vec2 pix_coord = WarpedTexCoord.xy*texture_size + vec2(0.0, -0.5);

    vec2 tc = (floor(pix_coord)+vec2(0.5,0.5))/texture_size;

    vec2 fp = fract(pix_coord);

    vec3 color0 = texture(Source, tc     ).xyz;
    vec3 color1 = texture(Source, tc + dy).xyz;

    float pos0 = fp.y;
    float pos1 = 1 - fp.y;

    vec3 lum0 = mix(vec3(BEAM_MIN_WIDTH), vec3(BEAM_MAX_WIDTH), color0);
    vec3 lum1 = mix(vec3(BEAM_MIN_WIDTH), vec3(BEAM_MAX_WIDTH), color1);

    vec3 d0 = SCANLINES_STRENGTH*pos0/(lum0*lum0+0.0000001);
    vec3 d1 = SCANLINES_STRENGTH*pos1/(lum1*lum1+0.0000001);

    color  = BRIGHTBOOST*get_scanlines(d0, d1, color0, color1);

    color  = GAMMA_OUT(color);

    vec2 mask_coords =vTexCoord.xy * params.OutputSize.xy;

    color.rgb*=GAMMA_OUT(mask_weights(mask_coords, params.MASK_INTENSITY, int(params.PHOSPHOR_LAYOUT), params.MONITOR_SUBPIXELS));

    FragColor = vec4(POST_BRIGHTNESS*color, 1.0);

    FragColor *= (CURVATURE > 0.5) ? corner(WarpedTexCoord) : 1.0;
}
