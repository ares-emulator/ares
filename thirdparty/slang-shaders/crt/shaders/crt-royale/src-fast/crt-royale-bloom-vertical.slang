#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  crt-royale: A full-featured CRT shader, with cheese.
//  Copyright (C) 2014 TroggleMonkey <trogglemonkey@gmx.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along with
//  this program; if not, write to the Free Software Foundation, Inc., 59 Temple
//  Place, Suite 330, Boston, MA 02111-1307 USA

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	vec4 MASKED_SCANLINESSize;
	vec4 BLOOM_APPROXSize;
} params;

/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

#include "bind-shader-params.h"

///////////////////////////////  VERTEX INCLUDES  ///////////////////////////////

float bloom_approx_scale_x = params.OutputSize.x / params.SourceSize.y;
float max_viewport_size_x  = 1080.0*1024.0*(4.0/3.0);
float bloom_diff_thresh_   = 1.0/256.0;
float mask_min_allowed_tile_size  = ceil(mask_min_allowed_triad_size * mask_triads_per_tile);

struct st_gauss{
    vec4 w1_8;
    vec4 w1_8_ratio;
};

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tex_uv;
layout(location = 1) out vec2 bloom_dxdy;
layout(location = 2) out float bloom_sigma_runtime;
layout(location = 3) out vec4 w1_8;
layout(location = 4) out vec4 w1_8_ratio;
layout(location = 5) out float weight_sum_inv;

// copied from bloom-functions.h
float get_min_sigma_to_blur_triad(float triad_size,
    float thresh)
{
    //  Requires:   1.) triad_size is the final phosphor triad size in pixels
    //              2.) thresh is the max desired pixel difference in the
    //                  blurred triad (e.g. 1.0/256.0).
    //  Returns:    Return the minimum sigma that will fully blur a phosphor
    //              triad on the screen to an even color, within thresh.
    //              This closed-form function was found by curve-fitting data.
    //  Estimate: max error = ~0.086036, mean sq. error = ~0.0013387:
    return -0.05168 + 0.6113*triad_size - 1.122*triad_size*sqrt(0.000416 + thresh);
    //  Estimate: max error = ~0.16486, mean sq. error = ~0.0041041:
    //return 0.5985*triad_size - triad_size*sqrt(thresh)
}

float get_fast_gaussian_weight_sum_inv(float sigma)
{
    //  We can use the Gaussian integral to calculate the asymptotic weight for
    //  the center pixel.  Since the unnormalized center pixel weight is 1.0,
    //  the normalized weight is the same as the weight sum inverse.  Given a
    //  large enough blur (9+), the asymptotic weight sum is close and faster:
    //      center_weight = 0.5 *
    //          (erf(0.5/(sigma*sqrt(2.0))) - erf(-0.5/(sigma*sqrt(2.0))))
    //      erf(-x) == -erf(x), so we get 0.5 * (2.0 * erf(blah blah)):
    //  However, we can get even faster results with curve-fitting.  These are
    //  also closer than the asymptotic results, because they were constructed
    //  from 64 blurs sizes from [3, 131) and 255 equally-spaced sigmas from
    //  (0, blurN_std_dev), so the results for smaller sigmas are biased toward
    //  smaller blurs.  The max error is 0.0031793913.
    //  Relative FPS: 134.3 with erf, 135.8 with curve-fitting.
    //float temp = 0.5/sqrt(2.0);
    //return erf(temp/sigma);
    return min(exp(exp(0.348348412457428/(sigma - 0.0860587260734721))), 0.399334576340352/sigma);
}

st_gauss get_blur_fastest_w1_8(float sigma)
{
    float denom_inv = 0.5/(sigma*sigma);
    float w0 = 1.0;
    float w1 = exp(-1.0 * denom_inv);
    float w2 = exp(-4.0 * denom_inv);
    float w3 = exp(-9.0 * denom_inv);
    float w4 = exp(-16.0 * denom_inv);
    float w5 = exp(-25.0 * denom_inv);
    float w6 = exp(-36.0 * denom_inv);
    float w7 = exp(-49.0 * denom_inv);
    float w8 = exp(-64.0 * denom_inv);

    st_gauss blur_weights;

    blur_weights.w1_8.x = w1 + w2;
    blur_weights.w1_8.y = w3 + w4;
    blur_weights.w1_8.z = w5 + w6;
    blur_weights.w1_8.w = w7 + w8;
    blur_weights.w1_8_ratio.x = w2/(blur_weights.w1_8.x);
    blur_weights.w1_8_ratio.y = w4/(blur_weights.w1_8.y);
    blur_weights.w1_8_ratio.z = w6/(blur_weights.w1_8.z);
    blur_weights.w1_8_ratio.w = w8/(blur_weights.w1_8.w);

    return blur_weights;
}

vec2 get_resized_mask_tile_size(vec2 estimated_viewport_size, vec2 estimated_mask_resize_output_size, bool solemnly_swear_same_inputs_for_every_pass)
{
    //  Stated tile properties must be correct:
    float tile_aspect_ratio_inv = mask_resize_src_lut_size.y/mask_resize_src_lut_size.x;
    float tile_aspect_ratio     = 1.0/tile_aspect_ratio_inv;
    vec2  tile_aspect           = vec2(1.0, tile_aspect_ratio_inv);

    //  If mask_specify_num_triads is 1.0/true and estimated_viewport_size.x is
    //  wrong, the user preference will be misinterpreted:
    float desired_tile_size_x = mask_triads_per_tile * mix(global.mask_triad_size_desired, estimated_viewport_size.x / global.mask_num_triads_desired, global.mask_specify_num_triads);

    //  Make sure we're not upsizing:
    float temp_tile_size_x = min(desired_tile_size_x, mask_resize_src_lut_size.x);

    //  Enforce min_tile_size and max_tile_size in both dimensions:
    vec2 temp_tile_size    = temp_tile_size_x * tile_aspect;
    vec2 min_tile_size     = mask_min_allowed_tile_size * tile_aspect;
    vec2 max_tile_size     = estimated_mask_resize_output_size / mask_resize_num_tiles;
    vec2 clamped_tile_size = clamp(temp_tile_size, min_tile_size, max_tile_size);

    float x_tile_size_from_y = clamped_tile_size.y * tile_aspect_ratio;
    float y_tile_size_from_x = mix(clamped_tile_size.y, clamped_tile_size.x * tile_aspect_ratio_inv, float(solemnly_swear_same_inputs_for_every_pass));
    vec2 reclamped_tile_size = vec2(min(clamped_tile_size.x, x_tile_size_from_y), min(clamped_tile_size.y, y_tile_size_from_x));

    //  We need integer tile sizes in both directions for tiled sampling to
    //  work correctly.  Use floor (to make sure we don't round up), but be
    //  careful to avoid a rounding bug where floor decreases whole numbers:
    vec2 final_resized_tile_size = floor(reclamped_tile_size + vec2(FIX_ZERO(0.0)));

    return final_resized_tile_size;
}

void main()
{
    gl_Position = global.MVP * Position;
    tex_uv = TexCoord * 1.0001;
   
    //  Get the uv sample distance between output pixels.  Calculate dxdy like
    //  blurs/shaders/vertex-shader-blur-fast-vertical.h.
    vec2 dxdy_scale = params.SourceSize.xy/params.OutputSize.xy;
    vec2 dxdy       = dxdy_scale/params.SourceSize.xy;

    //  This blur is vertical-only, so zero out the vertical offset:
    bloom_dxdy = vec2(0.0, dxdy.y);

    //  Calculate a runtime bloom_sigma in case it's needed:
    float mask_tile_size_x = get_resized_mask_tile_size(params.OutputSize.xy, params.OutputSize.xy * mask_resize_viewport_scale, false).x;
    bloom_sigma_runtime    = get_min_sigma_to_blur_triad(mask_tile_size_x / mask_triads_per_tile, bloom_diff_thresh_);

    st_gauss blur_weights  = get_blur_fastest_w1_8(bloom_sigma_runtime);

    w1_8       = blur_weights.w1_8;
    w1_8_ratio = blur_weights.w1_8_ratio;

    weight_sum_inv = get_fast_gaussian_weight_sum_inv(bloom_sigma_runtime);
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in vec2 tex_uv;
layout(location = 1) in vec2 bloom_dxdy;
layout(location = 2) in float bloom_sigma_runtime;
layout(location = 3) in vec4 w1_8;
layout(location = 4) in vec4 w1_8_ratio;
layout(location = 5) in float weight_sum_inv;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
#define input_texture Source

//////////////////////////////  FRAGMENT INCLUDES  //////////////////////////////

vec3 tex2Dblur17fastest(sampler2D tex, vec2 tex_uv, vec2 dxdy, float weight_sum_inv, vec4 w1_8, vec4 w1_8_ratio)
{

    float w0 = 1.0;

    vec3 sum = vec3(0.0,0.0,0.0);

    sum += (w1_8.w * texture(tex, tex_uv - (7.0 + w1_8_ratio.w) * dxdy).rgb);
    sum += (w1_8.z * texture(tex, tex_uv - (5.0 + w1_8_ratio.z) * dxdy).rgb);
    sum += (w1_8.y * texture(tex, tex_uv - (3.0 + w1_8_ratio.y) * dxdy).rgb);
    sum += (w1_8.x * texture(tex, tex_uv - (1.0 + w1_8_ratio.x) * dxdy).rgb);
    sum += (w0 * texture(tex, tex_uv).rgb);
    sum += (w1_8.x * texture(tex, tex_uv + (1.0 + w1_8_ratio.x) * dxdy).rgb);
    sum += (w1_8.y * texture(tex, tex_uv + (3.0 + w1_8_ratio.y) * dxdy).rgb);
    sum += (w1_8.z * texture(tex, tex_uv + (5.0 + w1_8_ratio.z) * dxdy).rgb);
    sum += (w1_8.w * texture(tex, tex_uv + (7.0 + w1_8_ratio.w) * dxdy).rgb);

    return sum * weight_sum_inv;
}

void main()
{
    FragColor = vec4(tex2Dblur17fastest(Source, tex_uv, bloom_dxdy, weight_sum_inv, w1_8, w1_8_ratio), 1.0);
}
