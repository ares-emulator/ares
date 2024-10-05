#version 450

/*
    Hyllian's CRT Shader

    Copyright (C) 2011-2024 Hyllian

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
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    float PRESET_OPTION;
    float BEAM_MIN_WIDTH;
    float BEAM_MAX_WIDTH;
    float SCANLINES_STRENGTH;
    float SCANLINES_SHAPE;
    float SHARPNESS_HACK;
    float SCANLINES_CUTOFF;
    float SCANLINES_HIRES;
    float IR_SCALE;
    float CRT_ANTI_RINGING;
    float VSCANLINES;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;


#pragma parameter H_OUTPUT_GAMMA "OUTPUT GAMMA" 2.2 1.0 3.0 0.05
#pragma parameter DISPLAY_RES "(IF PRESET > 0) TARGET RESOLUTION [ 1080p | 4k ]" 0.0 0.0 1.0 1.0
#pragma parameter PRESET_OPTION "PRESET [0=Custom, Aperture1, Aperture2, Slot1, Slot2, Shadow]" 0.0 0.0 5.0 1.0
#pragma parameter CUSTOM_OPTIONS "[Custom Settings]" 0.0 0.0 1.0 1.0
#pragma parameter BEAM_MIN_WIDTH "      MIN BEAM WIDTH" 0.86 0.0 1.0 0.01
#pragma parameter BEAM_MAX_WIDTH "      MAX BEAM WIDTH" 1.0 0.0 1.0 0.01
#pragma parameter SCANLINES_STRENGTH "      SCANLINES STRENGTH" 0.72 0.0 1.0 0.01
#pragma parameter SCANLINES_SHAPE "      SCANLINES SHAPE [ Sharp | Soft ]" 1.0 0.0 1.0 1.0
#pragma parameter PHOSPHOR_LAYOUT "      MASK [1..6 Aperture, 7..10 Shadow, 11..14 Slot]" 1.0 0.0 15.0 1.0
#pragma parameter MASK_STRENGTH "      MASK STRENGTH" 0.0 0.0 1.0 0.02
#pragma parameter BRIGHTBOOST "BRIGHTBOOST" 1.0 0.5 2.0 0.01
#pragma parameter MONITOR_SUBPIXELS "MONITOR SUBPIXELS LAYOUT [ RGB | BGR ]" 0.0 0.0 1.0 1.0
#pragma parameter SCANLINES_CUTOFF "SCANLINES CUTOFF" 400.0 0.0 1000.0 2.0
#pragma parameter SCANLINES_HIRES "HIGH RESOLUTION SCANLINES" 0.0 0.0 1.0 1.0
#pragma parameter IR_SCALE "INTERNAL RESOLUTION SCALE (Downsampling)" 1.0 1.0 10.0 1.0
#pragma parameter VSCANLINES "VERTICAL SCANLINES [ Off | On ]" 0.0 0.0 1.0 1.0
#pragma parameter SHARPNESS_HACK "SHARPNESS HACK" 1.0 1.0 4.0 1.0
#pragma parameter CRT_ANTI_RINGING "ANTI RINGING" 1.0 0.0 1.0 1.0

#define CRT_ANTI_RINGING params.CRT_ANTI_RINGING
#define SHARPNESS_HACK params.SHARPNESS_HACK
#define SCANLINES_SHAPE params.SCANLINES_SHAPE
#define SCANLINES_STRENGTH (-0.16*SCANLINES_SHAPE+params.SCANLINES_STRENGTH)
#define SCANLINES_CUTOFF params.SCANLINES_CUTOFF
#define BEAM_MIN_WIDTH params.BEAM_MIN_WIDTH
#define BEAM_MAX_WIDTH params.BEAM_MAX_WIDTH
#define IR_SCALE params.IR_SCALE
#define SCANLINES_HIRES params.SCANLINES_HIRES
#define VSCANLINES params.VSCANLINES


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 TextureSize;
layout(location = 2) out vec2 dx;
layout(location = 3) out vec2 dy;
layout(location = 4) out vec4 profile;
layout(location = 5) out float draw_scanlines;


vec4 get_beam_profile()
{
	vec4 bp = vec4(SCANLINES_STRENGTH, BEAM_MIN_WIDTH, BEAM_MAX_WIDTH, SCANLINES_SHAPE);

	if (params.PRESET_OPTION == 1.0)  bp = vec4(0.58, 0.86, 1.00, 1.0);
	if (params.PRESET_OPTION == 2.0)  bp = vec4(0.58, 0.86, 1.00, 1.0);
	if (params.PRESET_OPTION == 3.0)  bp = vec4(0.58, 0.86, 1.00, 0.0);
	if (params.PRESET_OPTION == 4.0)  bp = vec4(0.42, 0.86, 1.00, 1.0);
	if (params.PRESET_OPTION == 5.0)  bp = vec4(0.58, 0.86, 1.00, 1.0);

	return bp;
}

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord * 1.0001;

    vec2 pre_offset = vec2(SHARPNESS_HACK, 1.0/IR_SCALE);
    pre_offset      = mix(pre_offset, pre_offset.yx, VSCANLINES);

    TextureSize     = vec4(pre_offset, 1.0/pre_offset) * params.SourceSize;

    dx = mix(vec2(TextureSize.z, 0.0), vec2(0.0, TextureSize.w), VSCANLINES);
    dy = mix(vec2(0.0, TextureSize.w), vec2(TextureSize.z, 0.0), VSCANLINES);

    profile = get_beam_profile();

    draw_scanlines = (mix(TextureSize.y, TextureSize.x, VSCANLINES) <= SCANLINES_CUTOFF) ? 1.0 : 0.0;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 TextureSize;
layout(location = 2) in vec2 dx;
layout(location = 3) in vec2 dy;
layout(location = 4) in vec4 profile;
layout(location = 5) in float draw_scanlines;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;




// Horizontal cubic filter.
// Some known filters use these values:

//    B = 0.5, C = 0.0        =>  A sharp almost gaussian filter.
//    B = 0.0, C = 0.0        =>  Hermite cubic filter.
//    B = 1.0, C = 0.0        =>  Cubic B-Spline filter.
//    B = 0.0, C = 0.5        =>  Catmull-Rom Spline filter.
//    B = C = 1.0/3.0         =>  Mitchell-Netravali cubic filter.
//    B = 0.3782, C = 0.3109  =>  Robidoux filter.
//    B = 0.2620, C = 0.3690  =>  Robidoux Sharp filter.

// For more info, see: http://www.imagemagick.org/Usage/img_diagrams/cubic_survey.gif

const vec2 offset = vec2(0.5);

// Change these params to configure the horizontal filter.
const float  B =  0.0;
const float  C =  0.5;
 
const mat4 invX = mat4( (          -B - 6.0*C)/6.0,          (3.0*B + 12.0*C)/6.0, (-3.0*B - 6.0*C)/6.0,             B/6.0,
                        (12.0 - 9.0*B - 6.0*C)/6.0, (-18.0 + 12.0*B +  6.0*C)/6.0,                  0.0, (6.0 - 2.0*B)/6.0,
                       -(12.0 - 9.0*B - 6.0*C)/6.0, ( 18.0 - 15.0*B - 12.0*C)/6.0, ( 3.0*B + 6.0*C)/6.0,             B/6.0,
                        (           B + 6.0*C)/6.0,                            -C,                  0.0,               0.0);


#define scanlines_strength profile.x
#define beam_min_width     profile.y
#define beam_max_width     profile.z
#define scanlines_shape        profile.w

vec3 get_scanlines(vec3 d0, vec3 d1, vec3 color0, vec3 color1, float scan_shape)
{
    if (scan_shape > 0.5) {
        d0 = exp(-16.0*d0*d0);
        d1 = exp(-16.0*d1*d1);
    }
    else {
        d0 = vec3(1.0)-smoothstep(0.0, 0.5, d0);
        d1 = vec3(1.0)-smoothstep(0.0, 0.5, d1);
    }

    return (color0*d0+color1*d1);
}

void main()
{
    vec2 pix_coord = vTexCoord.xy*TextureSize.xy - offset;
    vec2 lr_pxc    = floor(pix_coord);
    vec2 hr_pxc    = mix(vec2(lr_pxc.x, pix_coord.y), vec2(pix_coord.x, lr_pxc.y), VSCANLINES);

    vec2 tc = ( (SCANLINES_HIRES > 0.5 ? hr_pxc : lr_pxc) + offset ) * TextureSize.zw;

    vec2 fp = mix(fract(pix_coord), fract(pix_coord.yx), VSCANLINES);

    vec3 c00 = texture(Source, tc     - dx     ).xyz;
    vec3 c01 = texture(Source, tc              ).xyz;
    vec3 c02 = texture(Source, tc     + dx     ).xyz;
    vec3 c03 = texture(Source, tc + 2.0*dx     ).xyz;

    vec3 c10, c11, c12, c13;

    if (SCANLINES_HIRES < 0.5)
    {
        c10 = texture(Source, tc     - dx + dy).xyz;
        c11 = texture(Source, tc          + dy).xyz;
        c12 = texture(Source, tc     + dx + dy).xyz;
        c13 = texture(Source, tc + 2.0*dx + dy).xyz;
    }
    else { c10 = c00; c11 = c01; c12 = c02; c13 = c03;}

    mat4x3 color_matrix0 = mat4x3(c00, c01, c02, c03);
    mat4x3 color_matrix1 = mat4x3(c10, c11, c12, c13);
    
    vec4 invX_Px    = vec4(fp.x*fp.x*fp.x, fp.x*fp.x, fp.x, 1.0) * invX;
    vec3 color0     = color_matrix0 * invX_Px;
    vec3 color1     = color_matrix1 * invX_Px;

    // Get min/max samples
    vec3 min_sample0 = min(c01,c02);
    vec3 max_sample0 = max(c01,c02);
    vec3 min_sample1 = min(c11,c12);
    vec3 max_sample1 = max(c11,c12);
    
    // Anti-ringing
    vec3 aux = color0;
    color0 = clamp(color0, min_sample0, max_sample0);
    color0 = mix(aux, color0, CRT_ANTI_RINGING);
    aux = color1;
    color1 = clamp(color1, min_sample1, max_sample1);
    color1 = mix(aux, color1, CRT_ANTI_RINGING);

    float pos0 = fp.y;
    float pos1 = 1 - fp.y;

    vec3 lum0 = mix(vec3(beam_min_width), vec3(beam_max_width), color0);
    vec3 lum1 = mix(vec3(beam_min_width), vec3(beam_max_width), color1);

    vec3 d0 = scanlines_strength*pos0/(lum0*lum0+0.0000001);
    vec3 d1 = scanlines_strength*pos1/(lum1*lum1+0.0000001);

    vec3 color  = (draw_scanlines > 0.5) ? get_scanlines(d0, d1, color0, color1, scanlines_shape) : texture(Source, vTexCoord).xyz;
    
    FragColor = vec4(color, 1.0);
}
