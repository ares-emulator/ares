#version 450

layout(push_constant) uniform Push
{
    float GLOW_ENABLE;
    float GLOW_RADIUS;
    float GLOW_STRENGTH;
    float PRESET_OPTION;
    float DISPLAY_RES;
    float PHOSPHOR_LAYOUT;
    float MASK_STRENGTH;
    float BRIGHTBOOST;
    float MONITOR_SUBPIXELS;
    float VSCANLINES;
    float H_OUTPUT_GAMMA;
    float geom_d;
    float geom_R;
    float geom_cornersize;
    float geom_cornersmooth;
    float geom_x_tilt;
    float geom_y_tilt;
    float geom_overscan_x;
    float geom_overscan_y;
    float geom_center_x;
    float geom_center_y;
    float geom_curvature;
    float geom_invert_aspect;
} params;


layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
} global;

#pragma parameter H_OUTPUT_GAMMA "OUTPUT GAMMA" 2.2 1.0 3.0 0.05
#pragma parameter GLOW_RADIUS "Glow Radius" 4.0 2.0 4.0 0.1
#pragma parameter GLOW_STRENGTH "Glow Strength" 0.15 0.0 1.0 0.01
#pragma parameter GLOW_ENABLE "Enable Glow" 1.0 0.0 1.0 1.0

#pragma parameter DISPLAY_RES "(IF PRESET > 0) TARGET RESOLUTION [ 1080p | 4k ]" 0.0 0.0 1.0 1.0
#pragma parameter PRESET_OPTION "PRESET [0=Custom, Aperture1, Aperture2, Slot1, Slot2, Shadow]" 0.0 0.0 5.0 1.0
#pragma parameter PHOSPHOR_LAYOUT "      MASK [1..6 Aperture, 7..10 Shadow, 11..14 Slot]" 1.0 0.0 15.0 1.0
#pragma parameter MASK_STRENGTH "      MASK STRENGTH" 0.0 0.0 1.0 0.02
#pragma parameter BRIGHTBOOST "BRIGHTBOOST" 1.0 0.5 2.0 0.01
#pragma parameter MONITOR_SUBPIXELS "MONITOR SUBPIXELS LAYOUT [ RGB | BGR ]" 0.0 0.0 1.0 1.0
#pragma parameter VSCANLINES "VERTICAL SCANLINES [ Off | On ]" 0.0 0.0 1.0 1.0

#pragma parameter geom_curvature "Geom Curvature Toggle" 0.0 0.0 1.0 1.0
#pragma parameter geom_R "Geom Curvature Radius" 2.0 0.3 10.0 0.1
#pragma parameter geom_d "Geom Distance" 1.5 0.1 3.0 0.1
#pragma parameter geom_invert_aspect "Geom Curvature Aspect Inversion" 0.0 0.0 1.0 1.0
#pragma parameter geom_cornersize "Geom Corner Size" 0.006 0.001 1.0 0.005
#pragma parameter geom_cornersmooth "Geom Corner Smoothness" 400.0 80.0 2000.0 100.0
#pragma parameter geom_x_tilt "Geom Horizontal Tilt" 0.0 -0.5 0.5 0.01
#pragma parameter geom_y_tilt "Geom Vertical Tilt" 0.0 -0.5 0.5 0.01
#pragma parameter geom_center_x "Geom Center X" 0.0 -1.0 1.0 0.001
#pragma parameter geom_center_y "Geom Center Y" 0.0 -1.0 1.0 0.001
#pragma parameter geom_overscan_x "Geom Horiz. Overscan %" 100.0 -125.0 125.0 0.5
#pragma parameter geom_overscan_y "Geom Vert. Overscan %" 100.0 -125.0 125.0 0.5

#define BRIGHTBOOST params.BRIGHTBOOST
#define DISPLAY_RES params.DISPLAY_RES
#define GLOW_ENABLE params.GLOW_ENABLE
#define GLOW_RADIUS params.GLOW_RADIUS
#define MASK_STRENGTH params.MASK_STRENGTH
#define MONITOR_SUBPIXELS params.MONITOR_SUBPIXELS
#define VSCANLINES params.VSCANLINES
#define GLOW_STRENGTH params.GLOW_STRENGTH

#define INV_OUTPUT_GAMMA (1.0 / params.H_OUTPUT_GAMMA)
#define saturate(c) clamp(c, 0.0, 1.0)

#define GAMMA_OUT(color)    pow(color, vec3(INV_OUTPUT_GAMMA))

// Macros.
#define FIX(c) max(abs(c), 1e-5);
#define PI 3.141592653589


// aspect ratio
vec2 aspect     = vec2(params.geom_invert_aspect > 0.5 ? (0.75, 1.0) : (1.0, 0.75));
vec2 overscan   = vec2(1.01, 1.01);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 sinangle;
layout(location = 2) out vec2 cosangle;
layout(location = 3) out vec3 stretch;
layout(location = 4) out float R_d_cx_cy;
layout(location = 5) out float d2;
layout(location = 6) out vec2 mask_profile;

vec2 get_beam_profile()
{
    vec2 bp = vec2(params.PHOSPHOR_LAYOUT, MASK_STRENGTH);

    if (DISPLAY_RES < 0.5)
    {
	if (params.PRESET_OPTION == 1.0)  bp = vec2( 1.0, 0.00);
	if (params.PRESET_OPTION == 2.0)  bp = vec2( 2.0, 0.00);
	if (params.PRESET_OPTION == 3.0)  bp = vec2(11.0, 0.00);
	if (params.PRESET_OPTION == 4.0)  bp = vec2(11.0, 0.20);
	if (params.PRESET_OPTION == 5.0)  bp = vec2( 7.0, 0.00);
    } else {
	if (params.PRESET_OPTION == 1.0)  bp = vec2( 2.0, 0.00);
	if (params.PRESET_OPTION == 2.0)  bp = vec2( 4.0, 0.00);
	if (params.PRESET_OPTION == 3.0)  bp = vec2(14.0, 0.00);
	if (params.PRESET_OPTION == 4.0)  bp = vec2(14.0, 0.20);
	if (params.PRESET_OPTION == 5.0)  bp = vec2( 9.0, 0.00);
     }

	return bp;
}


/*
    Geom code - a modified Geom code without CRT features made to provide
    curvature/warping/oversampling features.

    Adapted by Hyllian (2024).
*/

/*
    Copyright (C) 2010-2012 cgwg, Themaister and DOLLS

    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    (cgwg gave their consent to have the original version of this shader
    distributed under the GPL in this message:

    http://board.byuu.org/viewtopic.php?p=26075#p26075

    "Feel free to distribute my shaders under the GPL. After all, the
    barrel distortion code was taken from the Curvature shader, which is
    under the GPL."
    )
    This shader variant is pre-configured with screen curvature
*/


float intersect(vec2 xy)
{
    float A = dot(xy,xy) + params.geom_d*params.geom_d;
    float B = 2.0*(params.geom_R*(dot(xy,sinangle)-params.geom_d*cosangle.x*cosangle.y)-params.geom_d*params.geom_d);
    float C = params.geom_d*params.geom_d + 2.0*params.geom_R*params.geom_d*cosangle.x*cosangle.y;
    
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy)
{
    float c     = intersect(xy);
    vec2 point  = (vec2(c, c)*xy - vec2(-params.geom_R, -params.geom_R)*sinangle) / vec2(params.geom_R, params.geom_R);
    vec2 poc    = point/cosangle;
    
    vec2 tang   = sinangle/cosangle;
    float A     = dot(tang, tang) + 1.0;
    float B     = -2.0*dot(poc, tang);
    float C     = dot(poc, poc) - 1.0;
    
    float a     = (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
    vec2 uv     = (point - a*sinangle)/cosangle;
    float r     = FIX(params.geom_R*acos(a));
    
    return uv*r/sin(r/params.geom_R);
}

vec2 fwtrans(vec2 uv)
{
    float r = FIX(sqrt(dot(uv,uv)));
    uv *= sin(r/params.geom_R)/r;
    float x = 1.0-cos(r/params.geom_R);
    float D = params.geom_d/params.geom_R + x*cosangle.x*cosangle.y+dot(uv,sinangle);
    
    return params.geom_d*(uv*cosangle-x*sinangle)/D;
}

vec3 maxscale()
{
    vec2 c  = bkwtrans(-params.geom_R * sinangle / (1.0 + params.geom_R/params.geom_d*cosangle.x*cosangle.y));
    vec2 a  = vec2(0.5,0.5)*aspect;
    
    vec2 lo = vec2(fwtrans(vec2(-a.x,  c.y)).x,
                   fwtrans(vec2( c.x, -a.y)).y)/aspect;

    vec2 hi = vec2(fwtrans(vec2(+a.x,  c.y)).x,
                   fwtrans(vec2( c.x, +a.y)).y)/aspect;
    
    return vec3((hi+lo)*aspect*0.5,max(hi.x-lo.x,hi.y-lo.y));
}

void main()
{
    gl_Position = global.MVP * Position;

    vTexCoord = TexCoord * vec2(1.0001) - vec2(params.geom_center_x, params.geom_center_y);

    // Precalculate a bunch of useful values we'll need in the fragment
    // shader.
    sinangle    = sin(vec2(params.geom_x_tilt, params.geom_y_tilt));
    cosangle    = cos(vec2(params.geom_x_tilt, params.geom_y_tilt));
    stretch     = maxscale();

    d2        = params.geom_d * params.geom_d;
    R_d_cx_cy = params.geom_R * params.geom_d * cosangle.x * cosangle.y;

    mask_profile = get_beam_profile();
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 sinangle;
layout(location = 2) in vec2 cosangle;
layout(location = 3) in vec3 stretch;
layout(location = 4) in float R_d_cx_cy;
layout(location = 5) in float d2;
layout(location = 6) in vec2 mask_profile;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D CRTPass;

float intersect(vec2 xy, float d2, float R_d_cx_cy)
{
    float A = dot(xy,xy) + d2;
    float B = 2.0 * ( params.geom_R * dot(xy, sinangle) - R_d_cx_cy - d2);
    float C = d2 + 2.0 * R_d_cx_cy;
    
    return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy, float d2, float R_d_cx_cy)
{
    float c     = intersect(xy, d2, R_d_cx_cy);
    vec2 point  = (vec2(c, c)*xy - vec2(-params.geom_R, -params.geom_R)*sinangle) / vec2(params.geom_R, params.geom_R);
    vec2 poc    = point/cosangle;
    
    vec2 tang   = sinangle/cosangle;
    float A     = dot(tang, tang) + 1.0;
    float B     = -2.0*dot(poc, tang);
    float C     = dot(poc, poc) - 1.0;
    
    float a     = (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
    vec2 uv     = (point - a*sinangle)/cosangle;
    float r     = FIX(params.geom_R*acos(a));
    
    return uv*r/sin(r/params.geom_R);
}

vec2 transform(vec2 coord, float d2, float R_d_cx_cy)
{
    coord = (coord - vec2(0.5, 0.5))*aspect*stretch.z + stretch.xy;
    
    return (bkwtrans(coord, d2, R_d_cx_cy) /
        vec2(params.geom_overscan_x / 100.0, params.geom_overscan_y / 100.0)/aspect + vec2(0.5, 0.5));
}

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) * vec2(params.geom_overscan_x / 100.0, params.geom_overscan_y / 100.0) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * aspect;
    vec2 cdist = vec2(params.geom_cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
      return clamp((cdist.x - dist)*params.geom_cornersmooth, 0.0, 1.0);
}



/* Mask code pasted from subpixel_masks.h. Masks 3 and 4 added. */
vec3 mask_weights(vec2 coord, float phosphor_layout){
   vec3 weights = vec3(1.,1.,1.);

   const float on  = 1.;
   const float off = 0.;

   const vec3 red     = vec3(off, off, on );
   const vec3 green   = vec3(off, on,  off);
   const vec3 blue    = vec3(on,  off, off);
   const vec3 magenta = vec3(on,  off, on );
   const vec3 yellow  = vec3(off, on,  on );
   const vec3 cyan    = vec3(on,  on,  off);
   const vec3 black   = vec3(off, off, off);
   const vec3 white   = vec3(on,  on,  on );

   int w, z = 0;
   
   // This pattern is used by a few layouts, so we'll define it here
   vec3 aperture_classic = mix(magenta, green, floor(mod(coord.x, 2.0)));
   
   if(phosphor_layout == 0.) return weights;

   else if(phosphor_layout == 1.){
      // classic aperture for RGB panels; good for 1080p, too small for 4K+
      // aka aperture_1_2_bgr
      weights  = aperture_classic;
      return weights;
   }

   else if(phosphor_layout == 2.){
      // Classic RGB layout; good for 1080p and lower
      const vec3 aperture1[3] = vec3[](red, green, blue);
//      vec3 bw3[3] = vec3[](black, yellow, blue);
      
      z = int(floor(mod(coord.x, 3.0)));
      
      weights = aperture1[z];
      return weights;
   }

   else if(phosphor_layout == 3.){
      // black and white aperture; good for weird subpixel layouts and low brightness; good for 1080p and lower
      const vec3 aperture2[3] = vec3[](black, white, white);
      
      z = int(floor(mod(coord.x, 3.0)));
      
      weights = aperture2[z];
      return weights;
   }

   else if(phosphor_layout == 4.){
      // reduced TVL aperture for RGB panels. Good for 4k.
      // aperture_2_4_rgb
      
      const vec3 aperture3[4] = vec3[](red, yellow, cyan, blue);
      
      w = int(floor(mod(coord.x, 4.0)));
      
      weights = aperture3[w];
      return weights;
   }
   

   else if(phosphor_layout == 5.){
      // black and white aperture; good for weird subpixel layouts and low brightness; good for 4k 
      const vec3 aperture4[4] = vec3[](black, black, white, white);
      
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = aperture4[z];
      return weights;
   }


   else if(phosphor_layout == 6.){
      // aperture_1_4_rgb; good for simulating lower 
      const vec3 aperture5[4] = vec3[](red, green, blue, black);
      
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = aperture5[z];
      return weights;
   }

   else if(phosphor_layout == 7.){
      // 2x2 shadow mask for RGB panels; good for 1080p, too small for 4K+
      // aka delta_1_2x1_bgr
      vec3 inverse_aperture = mix(green, magenta, floor(mod(coord.x, 2.0)));
      weights               = mix(aperture_classic, inverse_aperture, floor(mod(coord.y, 2.0)));
      return weights;
   }

   else if(phosphor_layout == 8.){
      // delta_2_4x1_rgb
      const vec3 delta1[2][4] = {
         {red, yellow, cyan, blue},
         {cyan, blue, red, yellow}
      };
      
      w = int(floor(mod(coord.y, 2.0)));
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = delta1[w][z];
      return weights;
   }

   else if(phosphor_layout == 9.){
      // delta_1_4x1_rgb; dunno why this is called 4x1 when it's obviously 4x2 /shrug
      const vec3 delta1[2][4] = {
         {red,  green, blue, black},
         {blue, black, red,  green}
      };
      
      w = int(floor(mod(coord.y, 2.0)));
      z = int(floor(mod(coord.x, 4.0)));
      
      weights = delta1[w][z];
      return weights;
   }
   
   else if(phosphor_layout == 10.){
      // delta_2_4x2_rgb
      const vec3 delta[4][4] = {
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

   else if(phosphor_layout == 11.){
      // slot mask for RGB panels; looks okay at 1080p, looks better at 4K
      const vec3 slotmask[4][6] = {
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

   else if(phosphor_layout == 12.){
      // slot mask for RGB panels; looks okay at 1080p, looks better at 4K
      const vec3 slotmask[4][6] = {
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

   else if(phosphor_layout == 13.){
      // based on MajorPainInTheCactus' HDR slot mask
      const vec3 slot[4][8] = {
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

   else if(phosphor_layout == 14.){
      // same as above but for RGB panels
      const vec3 slot2[4][10] = {
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
   
   else if(phosphor_layout == 15.){
      // slot_3_7x6_rgb
      const vec3 slot[6][14] = {
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


vec3 get_bloom(sampler2D src, vec2 tex, vec2 dxy) {
    vec3  bloom = vec3(0.0);

    const float k[9] = {
        0.001234403, 0.014304688, 0.082317798,
        0.235235546, 0.33381513,  0.235235546,
        0.082317798, 0.014304688, 0.001234403
    };

    bloom += k[0] * texture(src, tex - 4.0 * dxy).rgb;
    bloom += k[1] * texture(src, tex - 3.0 * dxy).rgb;
    bloom += k[2] * texture(src, tex - 2.0 * dxy).rgb;
    bloom += k[3] * texture(src, tex -       dxy).rgb;
    bloom += k[4] * texture(src, tex           ).rgb;
    bloom += k[5] * texture(src, tex +       dxy).rgb;
    bloom += k[6] * texture(src, tex + 2.0 * dxy).rgb;
    bloom += k[7] * texture(src, tex + 3.0 * dxy).rgb;
    bloom += k[8] * texture(src, tex + 4.0 * dxy).rgb;

    return bloom;
}

void main()
{
    // Texture coordinates of the texel containing the active pixel.
    vec2  uv   = vTexCoord;
    float cval = 1.0;

    if (params.geom_curvature > 0.5)
    {
        uv   = transform(vTexCoord, d2, R_d_cx_cy);
        cval = corner(uv);
    }

    // Bloom: Vertical blur
    vec2 dy    = vec2(0.0, GLOW_RADIUS*global.SourceSize.w);

    vec3 bloom = (GLOW_ENABLE > 0.5) ? get_bloom(Source, uv, dy) : vec3(0.0);

    // Mask
    vec2 mask_coords = vTexCoord.xy * global.OutputSize.xy;

    mask_coords = mix(mask_coords.xy, mask_coords.yx, VSCANLINES);

    vec3 mask_wgts = mask_weights(mask_coords, mask_profile.x);

    mask_wgts = mask_wgts + vec3(1.0-mask_profile.y);

    mask_wgts = (MONITOR_SUBPIXELS > 0.5) ? mask_wgts.bgr : mask_wgts;

    // CRT + Bloom + Mask + Curvature
    vec3 crt = BRIGHTBOOST * texture(CRTPass, uv).rgb;

    crt += GLOW_STRENGTH * bloom;

    crt = GAMMA_OUT(crt) * GAMMA_OUT(mask_wgts) * vec3(cval);

    crt *= step(0.0, fract(uv.y)); // Discard off limit pixels

    FragColor = vec4(crt, 1.0);
}
