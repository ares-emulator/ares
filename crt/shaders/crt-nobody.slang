#version 450

/*
   Hyllian's crt-nobody Shader
   
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
    float CN_VSCANLINES;
    float CN_BEAM_MIN_WIDTH;
    float CN_BEAM_MAX_WIDTH;
    float CN_SCAN_SIZE;
    float CN_BRIGHTBOOST;
    float CN_InputGamma;
    float CN_OutputGamma;
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

#pragma parameter CN_NONONO          "CRT-NOBODY:"                  0.0  0.0 1.0 1.0
#pragma parameter CN_VSCANLINES      "    Vertical Scanlines"       0.0  0.0 1.0 1.0
#pragma parameter CN_BEAM_MIN_WIDTH  "    Min Beam Width"           0.80 0.0 1.0 0.01
#pragma parameter CN_BEAM_MAX_WIDTH  "    Max Beam Width"           1.0  0.0 1.0 0.01
#pragma parameter CN_SCAN_SIZE       "    Scanlines Thickness"      0.86 0.0 1.0 0.01
#pragma parameter CN_BRIGHTBOOST     "    Brightness Boost"         1.0  0.5 1.5 0.01
#pragma parameter CN_InputGamma      "    Input Gamma"              2.4  0.0 4.0 0.1
#pragma parameter CN_OutputGamma     "    Output Gamma"             2.2  0.0 3.0 0.1

#pragma parameter geom_space         "                     "                0.0  0.0 1.0 1.0
#pragma parameter geom_nonono        "GEOM'S CURVATURE:"                0.0  0.0 1.0 1.0
#pragma parameter geom_curvature     "    Curvature Toggle"             0.0 0.0 1.0 1.0
#pragma parameter geom_R             "    Curvature Radius"             2.0 0.3 10.0 0.1
#pragma parameter geom_d             "    Distance"                     1.5 0.1 3.0 0.1
#pragma parameter geom_invert_aspect "    Curvature Aspect Inversion"   0.0 0.0 1.0 1.0
#pragma parameter geom_cornersize    "    Corner Size"                  0.006 0.001 1.0 0.005
#pragma parameter geom_cornersmooth  "    Corner Smoothness"          400.0 80.0 2000.0 100.0
#pragma parameter geom_x_tilt        "    Horizontal Tilt"              0.0 -0.5 0.5 0.01
#pragma parameter geom_y_tilt        "    Vertical Tilt"                0.0 -0.5 0.5 0.01
#pragma parameter geom_center_x      "    Center X"                     0.0 -1.0 1.0 0.001
#pragma parameter geom_center_y      "    Center Y"                    0.0 -1.0 1.0 0.001
#pragma parameter geom_overscan_x    "    Horiz. Overscan %"         100.0 -125.0 125.0 0.5
#pragma parameter geom_overscan_y    "    Vert. Overscan %"          100.0 -125.0 125.0 0.5

#define CN_VSCANLINES     params.CN_VSCANLINES
#define CN_BEAM_MIN_WIDTH params.CN_BEAM_MIN_WIDTH
#define CN_BEAM_MAX_WIDTH params.CN_BEAM_MAX_WIDTH
#define CN_SCAN_SIZE      params.CN_SCAN_SIZE
#define CN_BRIGHTBOOST    params.CN_BRIGHTBOOST
#define CN_InputGamma     params.CN_InputGamma
#define CN_OutputGamma    params.CN_OutputGamma

#define GAMMA_IN(color)     CN_BRIGHTBOOST*pow(color, vec3(CN_InputGamma))
#define GAMMA_OUT(color)    pow(color, vec3(1.0 / CN_OutputGamma))

#define PIX_SIZE    1.111111

float pix_sizex  = mix(PIX_SIZE, CN_SCAN_SIZE, CN_VSCANLINES);
float scan_sizey = mix(CN_SCAN_SIZE, PIX_SIZE, CN_VSCANLINES);

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

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
    float B = 2.0 * (params.geom_R * (dot(xy,sinangle) - params.geom_d * cosangle.x * cosangle.y) - params.geom_d * params.geom_d);
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
    vec2 c  = bkwtrans( -params.geom_R * sinangle / (1.0 + params.geom_R / params.geom_d * cosangle.x * cosangle.y) );
    vec2 a  = vec2(0.5,0.5) * aspect;
    
    vec2 lo = vec2(fwtrans(vec2(-a.x,  c.y)).x,
                   fwtrans(vec2( c.x, -a.y)).y)/aspect;

    vec2 hi = vec2(fwtrans(vec2(+a.x,  c.y)).x,
                   fwtrans(vec2( c.x, +a.y)).y)/aspect;
    
    return vec3( (hi + lo) * aspect * 0.5, max(hi.x - lo.x, hi.y - lo.y) );
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

}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 sinangle;
layout(location = 2) in vec2 cosangle;
layout(location = 3) in vec3 stretch;
layout(location = 4) in float R_d_cx_cy;
layout(location = 5) in float d2;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

vec2 transform(vec2 coord)
{
    coord = (coord - vec2(0.5, 0.5)) * aspect * stretch.z + stretch.xy;

    float A = dot(coord, coord) + d2;
    float B = 2.0 * ( params.geom_R * dot(coord, sinangle) - R_d_cx_cy - d2);
    float C = d2 + 2.0 * R_d_cx_cy;
    
    float c     = (-B - sqrt(B*B - 4.0*A*C)) / (2.0*A);

    vec2 point  = (vec2(c, c) * coord - vec2(-params.geom_R, -params.geom_R) * sinangle) / vec2(params.geom_R, params.geom_R);
    vec2 poc    = point / cosangle;
    
    vec2 tang   = sinangle / cosangle;
    A     = dot(tang, tang) + 1.0;
    B     = -2.0 * dot(poc, tang);
    C     = dot(poc, poc) - 1.0;
    
    float a     = (-B + sqrt(B*B - 4.0*A*C))/(2.0*A);
    vec2 uv     = (point - a*sinangle)/cosangle;
    float r     = FIX(params.geom_R*acos(a));
    
    vec2 bkw = uv*r/sin(r/params.geom_R);

    return (bkw / vec2(params.geom_overscan_x / 100.0, params.geom_overscan_y / 100.0)/aspect + vec2(0.5, 0.5));
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

const vec2 CN_OFFSET = vec2(0.5);

vec2 wgt(vec2 size)
{
   size = clamp(size, -1.0, 1.0);

   size = 1.0 - size * size;

   return size * size * size;
}

void main()
{
    // Texture coordinates of the texel containing the active pixel.
    vec2  uv   = vTexCoord;
    float cval = 1.0;

    if (params.geom_curvature > 0.5)
    {
        uv   = transform(vTexCoord);
        cval = corner(uv);
    }

    vec2 pix_coord = uv * params.SourceSize.xy;
    vec2 tc  = (floor(pix_coord) + CN_OFFSET) * params.SourceSize.zw; // tc  = texel coord
    vec2 pos =  fract(pix_coord) - CN_OFFSET; // pos = pixel position
    vec2 dir =  sign(pos); // dir = pixel direction
    pos      =   abs(pos);

    vec2 g1 = dir * vec2(params.SourceSize.z,  0);
    vec2 g2 = dir * vec2( 0, params.SourceSize.w);

    mat2x3 AB = mat2x3(clamp(GAMMA_IN(texture(Source, tc    ).xyz), 0.0, 1.0), clamp(GAMMA_IN(texture(Source, tc +g1   ).xyz), 0.0, 1.0));
    mat2x3 CD = mat2x3(clamp(GAMMA_IN(texture(Source, tc +g2).xyz), 0.0, 1.0), clamp(GAMMA_IN(texture(Source, tc +g1+g2).xyz), 0.0, 1.0));

    vec2 wx = wgt(vec2(pos.x, 1.0-pos.x) / pix_sizex);

    mat2x3 c = mat2x3(AB * wx, CD * wx);

    float c0max = max(c[0].r, max(c[0].g, c[0].b));
    float c1max = max(c[1].r, max(c[1].g, c[1].b));

    float lum0  = mix(CN_BEAM_MIN_WIDTH, CN_BEAM_MAX_WIDTH, c0max);
    float lum1  = mix(CN_BEAM_MIN_WIDTH, CN_BEAM_MAX_WIDTH, c1max);

    vec2  ssy   = mix(vec2(scan_sizey * lum0, scan_sizey * lum1), vec2(scan_sizey), CN_VSCANLINES);

    vec3  color = (c * wgt(vec2(pos.y, 1.0-pos.y) / ssy));

    FragColor = vec4(GAMMA_OUT(color) * vec3(cval), 1.0);
}
