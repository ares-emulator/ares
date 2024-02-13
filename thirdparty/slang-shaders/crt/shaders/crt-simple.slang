#version 450

/*
 *  CRT-simple shader
 * 
 *  Copyright (C) 2011 DOLLS. Based on cgwg's CRT shader.
 *  ported and improved by DariusG @2023
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float DISTORTION,SCANLINE,INPUTGAMMA,OUTPUTGAMMA,MASK,SIZE,DOWNSCALE;
} params;

// Parameter lines go here:
#pragma parameter DISTORTION "Distortion" 0.12 0.0 0.30 0.01
#pragma parameter SCANLINE "Scanline Weight" 0.3 0.2 0.6 0.05
#pragma parameter DOWNSCALE "Scanlines Downscale" 1.0 1.0 2.0 1.0
#pragma parameter INPUTGAMMA "Input Gamma" 2.4 0.0 4.0 0.05
#pragma parameter OUTPUTGAMMA "Output Gamma" 2.2 0.0 4.0 0.05
#pragma parameter MASK "Mask Brightness" 0.7 0.0 1.0 0.05
#pragma parameter SIZE "Mask Size" 1.0 1.0 2.0 1.0

#define DISTORTION params.DISTORTION
#define SCANLINE params.SCANLINE
#define INPUTGAMMA params.INPUTGAMMA
#define OUTPUTGAMMA params.OUTPUTGAMMA
#define MASK params.MASK
#define SIZE params.SIZE
#define DOWNSCALE params.DOWNSCALE

#define SourceSize params.SourceSize
#define OriginalSize params.OriginalSize
#define OutputSize params.OutputSize
#define CURVATURE        
#define PI 3.141592653589
#define outgamma 1.0 / OUTPUTGAMMA
#define scale SourceSize.xy/OriginalSize.xy

#define pi 3.141592654
	
layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;


///////////////////////////////////////////////////////////////////////////////////////////////



        // Calculate the influence of a scanline on the current pixel.
        //
        // 'distance' is the distance in texture coordinates from the current
        // pixel to the scanline in question.
        // 'color' is the colour of the scanline at the horizontal location of
        // the current pixel.
        vec4 scanlineWeights(float distance, vec4 color)
        {
                // The "width" of the scanline beam is set as 2*(1 + x^4) for
                // each RGB channel.
                vec4 wid = 2.0 + 2.0 * pow(color, vec4(4.0));

                // The "weights" lines basically specify the formula that gives
                // you the profile of the beam, i.e. the intensity as
                // a function of distance from the vertical center of the
                // scanline. In this case, it is gaussian if width=2, and
                // becomes nongaussian for larger widths. Ideally this should
                // be normalized so that the integral across the beam is
                // independent of its width. That is, for a narrower beam
                // "weights" should have a higher peak at the center of the
                // scanline than for a wider beam.
                vec4 weights = vec4(distance / SCANLINE);
                return 1.4 * exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
        }


vec2 Distort(vec2 coord)
{
        vec2 CURVATURE_DISTORTION = vec2(DISTORTION, DISTORTION*1.5);
        // Barrel distortion shrinks the display area a bit, this will allow us to counteract that.
        vec2 barrelScale = 1.0 - (0.23 * CURVATURE_DISTORTION);
        coord *= SourceSize.xy/OriginalSize.xy;
        coord -= vec2(0.5);
        float rsq = coord.x * coord.x + coord.y * coord.y;
        coord += coord * (CURVATURE_DISTORTION * rsq);
        coord *= barrelScale;
        if (abs(coord.x) >= 0.5 || abs(coord.y) >= 0.5)
                coord = vec2(-1.0);             // If out of bounds, return an invalid value.
        else
        {
                coord += vec2(0.5);
                coord /= SourceSize.xy/OriginalSize.xy;
        }

        return coord;
}
void main()
{
        // Texture coordinates of the texel containing the active pixel.
              vec2 abspos = vTexCoord.xy*SourceSize.xy*scale;
        vec2 xy;

        #ifdef CURVATURE
                xy = Distort(vTexCoord.xy); float xblur = xy.x;
        #else
                xy = vTexCoord.xy;
        #endif
                // Of all the pixels that are mapped onto the texel we are
                // currently rendering, which pixel are we currently rendering?
                vec2 ratio_scale = xy * SourceSize.xy - 0.5;
                vec2 uv_ratio = fract(ratio_scale/DOWNSCALE);

                // Snap to the center of the underlying texel.
                xy = (floor(ratio_scale) + 0.5) / SourceSize.xy;
                xy.x = xblur;
                // Calculate the effective colour of the current and next
                // scanlines at the horizontal location of the current pixel.
                vec4 col  = texture(Source,xy); 
                col=pow(col,vec4(INPUTGAMMA));
                vec4 col2 = texture(Source,xy + vec2(0.0, SourceSize.w)); 
                col2=pow(col2,vec4(INPUTGAMMA));

                // Calculate the influence of the current and next scanlines on
                // the current pixel.
                vec4 weights  = scanlineWeights(uv_ratio.y, col);
                vec4 weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);

                vec3 mul_res  = (col * weights + col2 * weights2).rgb;
                vec2 fragCoord = vTexCoord*OutputSize.xy;
                // dot-mask emulation:
                vec3 dotMaskWeights = mix(vec3(MASK), vec3(1.0),fract(fragCoord.x*0.5/SIZE));

                mul_res *= dotMaskWeights;

                FragColor = vec4(vec3(pow(mul_res, vec3(outgamma))), 1.0);
        }
