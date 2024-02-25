#version 450
/* 
 * gizmo98 slotmask crt shader
 * Copyright (C) 2023 gizmo98
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 * version 0.1, 10.05.2023
 * ---------------------------------------------------------------------------------------
 * - initial commit
 * 
 * https://github.com/gizmo98/gizmo-crt-shader
 *
 * This shader tries to mimic a CRT without extensive use of scanlines and masks and rgb pattern emulation.
 * It uses horizontal subpixel scaling and adds brightness dependent scanline patterns and allows 
 * fractional scaling. 
 *
 * HORIZONTAL_BLUR simulates a bad composite signal which is neede for consoles like megadrive 
 * VERTICAL_BLUR vertical blur simulates N64 vertical blur 
 * BGR_LCD_PATTERN most LCDs have a RGB pixel pattern. Enable BGR pattern with this switch
 * BRIGHTNESS makes scanlines more or less visible
 * SHRINK scale screen in X direction
 * SNR noise intensity 
 * COLOUR_BLEEDING colour bleeding intensity
 * GRID slotmask intensity
 * SLOTMASK switch between slotmask (1) and aperture grille (0)
 *
 * uses parts curvator of CRT-PI shader from davej https://github.com/libretro/glsl-shaders/blob/master/crt/shaders/crt-pi.glsl
 * uses parts of texture anti-aliasing shader from Ikaros https://www.shadertoy.com/view/ldsSRX
 * uses gold noise shader from dcerisano https://www.shadertoy.com/view/ltB3zD
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float CURVATURE_X;
	float CURVATURE_Y;
	float BRIGHTNESS;
	float HORIZONTAL_BLUR;
	float VERTICAL_BLUR;
	float BLUR_OFFSET;
	float BGR_LCD_PATTERN;
	float SHRINK;
	float SNR;
	float COLOUR_BLEEDING;
	float GRID;
	float SLOTMASK;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter CURVATURE_X "Screen curvature - horizontal"  0.10 0.0 1.0  0.01
#pragma parameter CURVATURE_Y "Screen curvature - vertical"    0.15 0.0 1.0  0.01
#pragma parameter BRIGHTNESS "Scanline Intensity"              0.5 0.05 1.0 0.05
#pragma parameter HORIZONTAL_BLUR "Horizontal Blur"            0.0 0.0 1.0 1.0
#pragma parameter VERTICAL_BLUR "Vertical Blur"                0.0 0.0 1.0 1.0
#pragma parameter BLUR_OFFSET "Blur Intensity"                 0.5 0.0 1.0 0.05
#pragma parameter BGR_LCD_PATTERN "BGR output pattern"         0.0 0.0 1.0 1.0
#pragma parameter SHRINK "Horizontal scale"                    0.0 0.0 1.0 0.01
#pragma parameter SNR "Noise intensity"                        1.0 0.0 2.0 0.1
#pragma parameter COLOUR_BLEEDING "Colour bleeding intensity"  0.0 0.0 3.0 0.1
#pragma parameter GRID "Grid intensity"                        0.0 0.0 3.0 0.05
#pragma parameter SLOTMASK "Use slot mask"                     0.0 0.0 1.0 1.0

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

float PHI = 1.61803398874989484820459;  // Φ = Golden Ratio

float gold_noise(in vec2 xy, in float seed){
    return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}

vec2 saturateA(in vec2 x)
{
    return clamp(x, 0.0, 1.0);
}

vec2 magnify(in vec2 uv, in vec2 res)
{
    uv *= res; 
    return (saturateA(fract(uv) / saturateA(fwidth(uv))) + floor(uv) - 0.5) / res.xy;
}

vec4 textureAABlur(in vec2 uv){
    vec2 texSize = vec2(textureSize(Source, 0));
    uv = magnify(uv,texSize.xy);
    uv = uv*texSize.xy + 0.5;

    vec2 iuv = floor(uv);
    vec2 fuv = uv - iuv;    
    if (params.HORIZONTAL_BLUR == 1.0){
        vec2 uv1 = vec2(uv + vec2(-0.5,-0.5)) / texSize.xy;
        vec2 uv2 = vec2(uv + vec2(-0.5 - params.BLUR_OFFSET,-0.5)) / texSize.xy;
        vec4 col1 = texture( Source, uv1 );
        vec4 col2 = texture( Source, uv2 );
        vec4 col = (col1 + col2) / vec4(2.0);
        if (params.VERTICAL_BLUR == 1.0){
            vec2 uv3 = vec2(uv + vec2(-0.5,-0.5 - params.BLUR_OFFSET)) / texSize.xy;
            vec2 uv4 = vec2(uv + vec2(-0.5 - params.BLUR_OFFSET,-0.5 - params.BLUR_OFFSET)) / texSize.xy;
            vec4 col3 = texture( Source, uv3 );
            vec4 col4 = texture( Source, uv4 );
            col = (((col3 + col4) / vec4(2.0)) + col) / vec4(2.0);
        }
        return col;
    }
    else{
        uv = vec2(uv + vec2(-0.5,-0.5)) / texSize.xy;
        return texture( Source, uv );
    }
}

vec4 textureSubpixelScaling(in vec2 uvr, in vec2 uvg, in vec2 uvb ){
    return vec4(textureAABlur(uvr).r, textureAABlur(uvg).g, textureAABlur(uvb).b, 1.0);
}

float GetFuv(in vec2 uv){
    vec2 texSize = vec2(textureSize(Source, 0));
    uv = uv*texSize.xy + 0.5;
    vec2 iuv = floor(uv);
    vec2 fuv = uv - iuv;
    return abs((fuv*fuv*fuv*(fuv*(fuv*6.0-15.0)+10.0)).y - 0.5);
}

vec4 AddNoise(in vec4 col, in vec2 coord){
    /* Add some subpixel noise which simulates small CRT color variations */
    float iGlobalTime = float(params.FrameCount)*0.025;
    float snr = params.SNR * 0.03125;
    return clamp(col + gold_noise(coord,sin(iGlobalTime)) * snr - snr/2.0,0.0,1.0);
}

vec4 AddScanlines(in vec4 col, in vec2 uvr, in vec2 uvg, in vec2 uvb){
    /* Add scanlines which are wider for dark colors.
       You cannot see scanlines if color is bright. */
    vec2 texSize = vec2(textureSize(Source, 0));
    float brightness = 1.0 / params.BRIGHTNESS * 0.05; 
    float scale = (params.OutputSize.y / texSize.y) * 0.5;
    float dim = brightness * scale;
    col.rgb -= dim * (abs(1.5* (1.0 - col.rgb) * (0.5 - vec3(GetFuv(uvr), GetFuv(uvg), GetFuv(uvb)))));
    return col;
}

vec4 AddGrid(in vec4 col, in vec2 coord){
    vec2 texSize = vec2(textureSize(Source, 0));
    float scale = (params.OutputSize.x / texSize.y) * 0.25;
    if (mod(floor((coord.x) * 1.0) ,3.0) == 0.0){
        col = mix(col,vec4(0.0,0.0,0.0,0.5 * scale),params.GRID);
    }
    else{
        if (params.SLOTMASK == 1.0){
            float field = fract (coord.x / 6.0);
            if(((field >= 0.166 && field <= 0.5) && (mod(floor(coord.y * 1.0 + 1.0),3.0) == 0.0)) ||
               ((field >= 0.666 && field <= 1.0) && (mod(floor(coord.y * 1.0),3.0) == 0.0)))
               col = mix(col,vec4(0.0,0.0,0.0,0.5 * scale),params.GRID*0.3);
        }
    }
    return col;
}

vec3 XCoords(in float coord, in float factor){
    float iGlobalTime = float(params.FrameCount)*0.025;
    float spread = 1.0 / 3.0 + params.COLOUR_BLEEDING;;
    vec3 coords = vec3(coord);
    if(params.BGR_LCD_PATTERN == 1.0)
        coords.r += spread * 2.0;
    else
        coords.b += spread * 2.0;
    coords.g += spread;
    coords *= factor;
    return coords;
}

float YCoord(in float coord, in float factor){
    return coord * factor;
}

vec2 CURVATURE_DISTORTION = vec2(params.CURVATURE_X, params.CURVATURE_Y);
// Barrel distortion shrinks the display area a bit, this will allow us to counteract that.
vec2 barrelScale = 1.0 - (0.23 * CURVATURE_DISTORTION);

vec2 Distort(vec2 coord)
{
//  coord *= screenScale; // not necessary in slang
    coord -= vec2(0.5);
    float rsq = coord.x * coord.x + coord.y * coord.y;
    coord += coord * (CURVATURE_DISTORTION * rsq);
    coord *= barrelScale;
    if (abs(coord.x) >= 0.5 || abs(coord.y) >= 0.5)
        coord = vec2(-1.0);     // If out of bounds, return an invalid value.
    else
    {
        coord += vec2(0.5);
//      coord /= screenScale; // not necessary in slang
    }

    return coord;
}

vec2 Jitter(vec2 coord)
{
    float iGlobalTime = float(params.FrameCount)*0.025;
    coord.y += sin(iGlobalTime * 220.1) * 0.10;
    return coord;
}

void main()
{
    vec2 texSize = vec2(textureSize(Source, 0));
    vec2 texcoord = vTexCoord.xy;

    if (params.SHRINK > 0.0)
    {
        texcoord.x -= 0.5;
        texcoord.x *= 1.0 + params.SHRINK;
        texcoord.x += 0.5; 
    }

    texcoord = Distort(texcoord);
    if (texcoord.x < 0.0) 
    {
        FragColor = vec4(0.0);
        return;
    }

    vec2 fragCoord = texcoord.xy * params.OutputSize.xy;
    vec2 factor = texSize.xy / params.OutputSize.xy ;
    
    fragCoord = Jitter(fragCoord);
    
    highp float yCoord = YCoord(fragCoord.y, factor.y) ;
    highp vec3 xCoords = XCoords(fragCoord.x, factor.x) ;

    vec2 coord_r = vec2(xCoords.r/ texSize.x, texcoord.y) ;
    vec2 coord_g = vec2(xCoords.g, yCoord) / texSize.xy;
    vec2 coord_b = vec2(xCoords.b, yCoord) / texSize.xy;

    FragColor = textureSubpixelScaling(coord_r,coord_g,coord_b);
    FragColor = AddNoise(FragColor, fragCoord);
    FragColor = AddScanlines(FragColor, coord_r, coord_g, coord_b);
    FragColor = AddGrid(FragColor, gl_FragCoord.xy);  
    FragColor.a = 1.0;
}
