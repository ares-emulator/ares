#version 450

/*  CRT shader
 *
 *  Copyright (C) 2010-2016 cgwg, Themaister and DOLLS
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 */
 
#include "geom-deluxe-params.inc"

#include "../../../include/subpixel_masks.h"

#define u_tex_size0 global.SourceSize.xy
//#define u_tex_size1 global.internal1Size.xy
#define u_quad_dims global.OutputSize.xy
#define u_tex_size1 vec2(global.OutputSize.xy * global.SourceSize.zw)

// Comment the next line to disable interpolation in linear gamma (and gain speed).
#define LINEAR_PROCESSING

// Enable 3x oversampling of the beam profile
#define OVERSAMPLE

// Use the older, purely gaussian beam profile
//#define USEGAUSSIAN

// Macros.
#define FIX(c) max(abs(c), 1e-5)
#define PI 3.141592653589

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec2 v_sinangle;
layout(location = 2) out vec2 v_cosangle;
layout(location = 3) out vec3 v_stretch;
layout(location = 4) out vec2 v_one;
layout(location = 5) out vec2 ilfac;
layout(location = 6) out vec2 TextureSize;

float intersect(vec2 xy , vec2 sinangle, vec2 cosangle)
{
  float A = dot(xy,xy)+d.x*d.x;
  float B = 2.0*(params.R.x*(dot(xy,sinangle)-d.x*cosangle.x*cosangle.y)-d.x*d.x);
  float C = d.x*d.x + 2.0*params.R.x*d.x*cosangle.x*cosangle.y;
  return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy, vec2 sinangle, vec2 cosangle)
{
  float c = intersect(xy, sinangle, cosangle);
  vec2 pt = vec2(c)*xy;
  pt -= vec2(-params.R.x)*sinangle;
  pt /= vec2(params.R.x);
  vec2 tang = sinangle/cosangle;
  vec2 poc = pt/cosangle;
  float A = dot(tang,tang)+1.0;
  float B = -2.0*dot(poc,tang);
  float C = dot(poc,poc)-1.0;
  float a = (-B+sqrt(B*B-4.0*A*C))/(2.0*A);
  vec2 uv = (pt-a*sinangle)/cosangle;
  float r = FIX(params.R.x*acos(a));
  return uv*r/sin(r/params.R.x);
}

vec2 fwtrans(vec2 uv, vec2 sinangle, vec2 cosangle)
{
  float r = FIX(sqrt(dot(uv,uv)));
  uv *= sin(r/params.R.x)/r;
  float x = 1.0-cos(r/params.R.x);
  float D = d.x/params.R.x + x*cosangle.x*cosangle.y+dot(uv,sinangle);
  return d.x*(uv*cosangle-x*sinangle)/D;
}

vec3 maxscale(vec2 sinangle, vec2 cosangle)
{
  vec2 c = bkwtrans(-params.R.x * sinangle / (1.0 + params.R.x/d.x*cosangle.x*cosangle.y), sinangle, cosangle);
  vec2 a = vec2(0.5,0.5)*aspect.xy;
  vec2 lo = vec2(fwtrans(vec2(-a.x,c.y), sinangle, cosangle).x,
		 fwtrans(vec2(c.x,-a.y), sinangle, cosangle).y)/aspect.xy;
  vec2 hi = vec2(fwtrans(vec2(+a.x,c.y), sinangle, cosangle).x,
		 fwtrans(vec2(c.x,+a.y), sinangle, cosangle).y)/aspect.xy;
  return vec3((hi+lo)*aspect.xy*0.5,max(hi.x-lo.x,hi.y-lo.y));
}

void main()
{
   gl_Position = global.MVP * Position;
   v_texCoord = TexCoord;

  // Precalculate a bunch of useful values we'll need in the fragment
  // shader.
  vec2 ang;
  // if (u_rotation_type.x < 0.5)
  //   ang = vec2(0.0,angle.x);
  // else if (u_rotation_type.x < 1.5)
  //   ang = vec2(angle.x,0.0);
  // else if (u_rotation_type.x < 2.5)
  //   ang = vec2(0.0,-angle.x);
  // else
  //   ang = vec2(-angle.x,0.0);
  ang = angle.xy;
  v_sinangle = sin(ang);
  v_cosangle = cos(ang);
  v_stretch = maxscale(v_sinangle, v_cosangle);
  
  TextureSize = global.SourceSize.xy;
  
  ilfac = vec2(1.0, clamp(floor(global.SourceSize.y/(interlace_detect == 1.0 ? 200.0 : 1000.0)),  1.0, 2.0));

  // The size of one texel, in texture-coordinates.
  v_one = ilfac / TextureSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec2 v_sinangle;
layout(location = 2) in vec2 v_cosangle;
layout(location = 3) in vec3 v_stretch;
layout(location = 4) in vec2 v_one;
layout(location = 5) in vec2 ilfac;
layout(location = 6) in vec2 TextureSize;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D blur_texture;
layout(set = 0, binding = 3) uniform sampler2D internal1;
// comment these out, as we're using generated masks instead of LUTs
layout(set = 0, binding = 4) uniform sampler2D aperture;
layout(set = 0, binding = 5) uniform sampler2D slot;
layout(set = 0, binding = 6) uniform sampler2D delta;
layout(set = 0, binding = 7) uniform sampler2D phosphor;

vec4 TEX2D(vec2 c)
{
	vec2 underscan = step(0.0,c) * step(0.0,vec2(1.0)-c);
	vec4 col = texture(internal1, c) * vec4(underscan.x*underscan.y);
#ifdef LINEAR_PROCESSING
       col = pow(col, vec4(CRTgamma.x));
#endif
	return col;
}

vec3 texblur(vec2 c)
{
  vec3 col = pow(texture(blur_texture,c).rgb, vec3(CRTgamma.x));
  // taper the blur texture outside its border with a gaussian
  float w = blurwidth.x / 320.0;
  c = min(c, vec2(1.0)-c) * aspect.xy * vec2(1.0/w);
  vec2 e2c = exp(-c*c);
  // approximation of erf gives smooth step
  // (convolution of gaussian with step)
  c = (step(0.0,c)-vec2(0.5)) * sqrt(vec2(1.0)-e2c) * (vec2(1.0) + vec2(0.1749)*e2c) + vec2(0.5);
  return col * vec3( c.x * c.y );
}

float intersect(vec2 xy , vec2 sinangle, vec2 cosangle)
{
  float A = dot(xy,xy)+d.x*d.x;
  float B = 2.0*(params.R.x*(dot(xy,sinangle)-d.x*cosangle.x*cosangle.y)-d.x*d.x);
  float C = d.x*d.x + 2.0*params.R.x*d.x*cosangle.x*cosangle.y;
  return (-B-sqrt(B*B-4.0*A*C))/(2.0*A);
}

vec2 bkwtrans(vec2 xy, vec2 sinangle, vec2 cosangle)
{
  float c = intersect(xy, sinangle, cosangle);
  vec2 pt = vec2(c)*xy;
  pt -= vec2(-params.R.x)*sinangle;
  pt /= vec2(params.R.x);
  vec2 tang = sinangle/cosangle;
  vec2 poc = pt/cosangle;
  float A = dot(tang,tang)+1.0;
  float B = -2.0*dot(poc,tang);
  float C = dot(poc,poc)-1.0;
  float a = (-B+sqrt(B*B-4.0*A*C))/(2.0*A);
  vec2 uv = (pt-a*sinangle)/cosangle;
  float r = FIX(params.R.x*acos(a));
  return uv*r/sin(r/params.R.x);
}

vec2 transform(vec2 coord, vec3 stretch, vec2 sinangle, vec2 cosangle)
{
  coord = (coord-vec2(0.5))*aspect.xy*stretch.z+stretch.xy;
  return (bkwtrans(coord, sinangle, cosangle)/overscan.xy/aspect.xy+vec2(0.5));
}

float corner(vec2 coord)
{
  coord = (coord - vec2(0.5)) * overscan.xy + vec2(0.5);
  coord = min(coord, vec2(1.0)-coord) * aspect.xy;
  vec2 cdist = vec2(cornersize.x);
  coord = (cdist - min(coord,cdist));
  float dist = sqrt(dot(coord,coord));
  return clamp((max(cdist.x,1e-3)-dist)*cornersmooth.x,0.0, 1.0);
}

// Calculate the influence of a scanline on the current pixel.
//
// 'distance' is the distance in texture coordinates from the current
// pixel to the scanline in question.
// 'color' is the colour of the scanline at the horizontal location of
// the current pixel.
vec4 scanlineWeights(float distance, vec4 color)
{
  // "wid" controls the width of the scanline beam, for each RGB channel
  // The "weights" lines basically specify the formula that gives
  // you the profile of the beam, i.e. the intensity as
  // a function of distance from the vertical center of the
  // scanline. In this case, it is gaussian if width=2, and
  // becomes nongaussian for larger widths. Ideally this should
  // be normalized so that the integral across the beam is
  // independent of its width. That is, for a narrower beam
  // "weights" should have a higher peak at the center of the
  // scanline than for a wider beam.
#ifdef USEGAUSSIAN
  vec4 wid = 0.3 + 0.1 * pow(color, vec4(3.0));
  vec4 weights = vec4(distance / wid);
  return (geom_lum + 0.4) * exp(-weights * weights) / wid;
#else
  vec4 wid = 2.0 + 2.0 * pow(color, vec4(4.0));
  vec4 weights = vec4(distance / scanline_weight);
  return (geom_lum + 1.4) * exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
#endif
}

void main()
{
  // Here's a helpful diagram to keep in mind while trying to
  // understand the code:
  //
  //  |      |      |      |      |
  // -------------------------------
  //  |      |      |      |      |
  //  |  01  |  11  |  21  |  31  | <-- current scanline
  //  |      | @    |      |      |
  // -------------------------------
  //  |      |      |      |      |
  //  |  02  |  12  |  22  |  32  | <-- next scanline
  //  |      |      |      |      |
  // -------------------------------
  //  |      |      |      |      |
  //
  // Each character-cell represents a pixel on the output
  // surface, "@" represents the current pixel (always somewhere
  // in the bottom half of the current scan-line, or the top-half
  // of the next scanline). The grid of lines represents the
  // edges of the texels of the underlying texture.

  // Texture coordinates of the texel containing the active pixel.
  vec2 xy;
  if (curvature.x > 0.5)
    xy = transform(v_texCoord, v_stretch, v_sinangle, v_cosangle);
  else
    xy = (v_texCoord-vec2(0.5))/overscan.xy+vec2(0.5);
  float cval = corner(xy);

  // extract average brightness from the mipmap texture
  float avgbright = dot(textureLod(blur_texture, vec2(1.,1.), 9.0).rgb,vec3(1.0))/3.0;
  float rbloom = 1.0 - rasterbloom.x * ( avgbright - 0.5 );
  // expand the screen when average brightness is higher
  xy = (xy - vec2(0.5)) * rbloom + vec2(0.5);
  vec2 xy0 = xy;

  // Of all the pixels that are mapped onto the texel we are
  // currently rendering, which pixel are we currently rendering?
    
  vec2 ilvec = vec2(0.0, ilfac.y * interlace_detect > 1.5 ? mod(float(global.FrameCount), 2.0) : 0.0);

  vec2 ratio_scale = (xy * TextureSize - vec2(0.5, 0.5) + ilvec) / ilfac;
  
#ifdef OVERSAMPLE
  float oversample_filter = fwidth(ratio_scale.y);
#endif
  vec2 uv_ratio = fract(ratio_scale);

  // Snap to the center of the underlying texel.
  xy = (floor(ratio_scale)*ilfac + vec2(0.5, 0.5) - ilvec) / TextureSize;

  // Calculate Lanczos scaling coefficients describing the effect
  // of various neighbour texels in a scanline on the current
  // pixel.
  vec4 coeffs = PI * vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);

  // Prevent division by zero.
  coeffs = FIX(coeffs);
  
  // Lanczos2 kernel.
  coeffs = 2.0 * sin(coeffs) * sin(coeffs / 2.0) / (coeffs * coeffs);

  // Normalize.
  coeffs /= dot(coeffs, vec4(1.0));

  // Calculate the effective colour of the current and next
  // scanlines at the horizontal location of the current pixel,
  // using the Lanczos coefficients above.
  vec4 col = clamp(TEX2D(xy + vec2(-v_one.x, 0.0))*coeffs.x +
                   TEX2D(xy)*coeffs.y +
		   TEX2D(xy +vec2(v_one.x, 0.0))*coeffs.z +
		   TEX2D(xy + vec2(2.0 * v_one.x, 0.0))*coeffs.w , 0.0, 1.0);
			 
  vec4 col2 = clamp(TEX2D(xy + vec2(-v_one.x, v_one.y))*coeffs.x +
		    TEX2D(xy + vec2(0.0, v_one.y))*coeffs.y +
		    TEX2D(xy + v_one)*coeffs.z +
		    TEX2D(xy + vec2(2.0 * v_one.x, v_one.y))*coeffs.w , 0.0, 1.0);


#ifndef LINEAR_PROCESSING
  col  = pow(col , vec4(CRTgamma.x));
  col2 = pow(col2, vec4(CRTgamma.x));
#endif

  // Calculate the influence of the current and next scanlines on
  // the current pixel.
  vec4 weights  = scanlineWeights(uv_ratio.y, col);
  vec4 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);
#ifdef OVERSAMPLE
  uv_ratio.y =uv_ratio.y+1.0/3.0*oversample_filter;
  weights = (weights+scanlineWeights(uv_ratio.y, col))/3.0;
  weights2=(weights2+scanlineWeights(abs(1.0-uv_ratio.y), col2))/3.0;
  uv_ratio.y =uv_ratio.y-2.0/3.0*oversample_filter;
  weights=weights+scanlineWeights(abs(uv_ratio.y), col)/3.0;
  weights2=weights2+scanlineWeights(abs(1.0-uv_ratio.y), col2)/3.0;
#endif
  vec3 mul_res  = (col * weights + col2 * weights2).rgb;

  // halation and corners
  vec3 blur = texblur(xy0);
  mul_res = mix(mul_res, blur, halation.x) * vec3(cval);
  // include factor of rbloom:

  // (probably imperceptible) brightness reduction when raster grows
  // Convert the image gamma for display on our output device.
  mul_res = mix(mul_res, blur, halation.x) * vec3(cval*rbloom);

  // Shadow mask
  // original code; just makes a giant phosphor here
   xy = fract(v_texCoord.xy * u_quad_dims.xy / u_tex_size1.xy);
  
  // gl_FragCoord; tied to physical pixel size
  //xy = fract(v_texCoord*global.internal1Size.xy);

  vec4 mask = vec4(1.0);
//  if (mask_picker == 1) mask = texture(aperture, xy);
//  else if (mask_picker == 2) mask = texture(slot, xy);
//  else if (mask_picker == 3) mask = texture(delta, xy);

  // use subpixel mask code instead of LUTs
  float alpha;
  mask = vec4(mask_weights_alpha(v_texCoord.xy * global.OutputSize.xy, 1., mask_picker, alpha), 1.0);
  mask.a = alpha;

  // count of total bright pixels is encoded in the mask's alpha channel
  float nbright = 255.0 - 255.0*mask.a;
  // fraction of bright pixels in the mask
  float fbright = nbright / ( u_tex_size1.x * u_tex_size1.y );
  // average darkening factor of the mask
  float aperture_average = mix(1.0-aperture_strength.x*(1.0-aperture_brightboost.x), 1.0, fbright);
  // colour of dark mask pixels
  vec3 clow = vec3(1.0-aperture_strength.x) * mul_res + vec3(aperture_strength.x*(aperture_brightboost.x)) * mul_res * mul_res;
  float ifbright = 1.0 / fbright;
  // colour of bright mask pixels
  vec3 chi = vec3(ifbright*aperture_average) * mul_res - vec3(ifbright - 1.0) * clow;
  vec3 cout = mix(clow,chi,mask.rgb); // mask texture selects dark vs bright

  // Convert the image gamma for display on our output device.
  cout = pow(cout, vec3(1.0 / monitorgamma.x));
  
  FragColor = vec4(cout, col.a);
}
