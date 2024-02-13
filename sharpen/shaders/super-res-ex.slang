#version 450

/*
   Super-res shader - Shiandow

   Ported from Shiandow's code by Hyllian, 2016.

   This file is a part of MPDN Extensions.
   https://github.com/zachsaw/MPDN_Extensions
  
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3.0 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library.
*/   

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 REFSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// -- Edge detection options -- 
#define strength 0.8
#define softness 0.3
#define acuity   100.0
#define radius   1.0
#define power    2.0

#define originalSize vec2(params.REFSize.xy)

#define dxdy (1.0 / vec2(params.SourceSize.xy))
#define ddxddy (1.0 / originalSize)

#define sqr(x) dot(x,x)


// -- Input processing --
//Current high res value
#define Get(x,y)    (texture(REF,ddxddy*(pos+ivec2(x,y)+0.5)).xyz)
#define GetY(x,y)    (texture(Source,ddxddy*(pos+ivec2(x,y)+0.5)).a)
//Downsampled result
#define Diff(x,y)     (texture(Source,ddxddy*(pos+ivec2(x,y)+0.5)).xyz)

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
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D REF;

const vec3 Y = vec3(.2126, .7152, .0722);


float RGBtoYUV(vec3 color)
{
  return dot(color, Y);
}

void main()
{
    vec2 tex = vTexCoord;

    vec4 c0 = texture(REF, tex);

    // Calculate position
    vec2 pos = tex * params.REFSize.xy - 0.5;
    vec2 offset = pos - round(pos);
    pos -= offset;

    // Calculate faithfulness force
    float W = 0.;
    vec3 diff = vec3(0.0);
    vec3 stab = vec3(0.0);
    float var = 0.;

    float c0y = RGBtoYUV(c0.rgb);

	// Loop unrolled for better compatibility.

	float dI2 = sqr(acuity*(c0y - GetY(-1,-1)));
        float dXY2 = sqr(vec2(-1,-1) - offset);
        float w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff(-1,-1);
        stab += w*(c0.rgb - Get(-1,-1));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY(-1, 0)));
        dXY2 = sqr(vec2(-1, 0) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff(-1, 0);
        stab += w*(c0.rgb - Get(-1, 0));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY(-1, 1)));
        dXY2 = sqr(vec2(-1, 1) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff(-1, 1);
        stab += w*(c0.rgb - Get(-1, 1));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 0,-1)));
        dXY2 = sqr(vec2( 0,-1) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 0,-1);
        stab += w*(c0.rgb - Get( 0,-1));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 0, 0)));
        dXY2 = sqr(vec2( 0, 0) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 0, 0);
        stab += w*(c0.rgb - Get( 0, 0));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 0, 1)));
        dXY2 = sqr(vec2( 0, 1) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 0, 1);
        stab += w*(c0.rgb - Get( 0, 1));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 1,-1)));
        dXY2 = sqr(vec2( 1,-1) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 1,-1);
        stab += w*(c0.rgb - Get( 1,-1));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 1, 0)));
        dXY2 = sqr(vec2( 1, 0) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 1, 0);
        stab += w*(c0.rgb - Get( 1, 0));
        var += w*dI2;
        W += w;

	dI2 = sqr(acuity*(c0y - GetY( 1, 1)));
        dXY2 = sqr(vec2( 1, 1) - offset);
        w = exp(-dXY2/(2.*radius*radius))*pow(1. + dI2/power, - power);
        diff += w*Diff( 1, 1);
        stab += w*(c0.rgb - Get( 1, 1));
        var += w*dI2;
        W += w;

    diff /= W;
    stab /= W;
    var = (var / W) - sqr(acuity*stab);

    // Calculate edge statistics
    float varD = softness * sqr(acuity*stab);
    float varS = (1. - softness) * var;

    // Apply force
    c0.xyz -= strength*mix(diff, stab, softness);

   FragColor = vec4(c0);
}