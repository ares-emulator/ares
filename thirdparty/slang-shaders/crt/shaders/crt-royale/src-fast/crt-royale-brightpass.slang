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
} params;

#define MASKED_SCANLINEStexture MASKED_SCANLINES
#define MASKED_SCANLINEStexture_size params.MASKED_SCANLINESSize.xy
#define MASKED_SCANLINESvideo_size params.MASKED_SCANLINESSize.xy

/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

#include "bind-shader-params.h"

///////////////////////////////  VERTEX INCLUDES  ///////////////////////////////

float bloom_approx_scale_x = params.OutputSize.x / params.SourceSize.y;
float max_viewport_size_x = 1080.0*1024.0*(4.0/3.0);
float bloom_diff_thresh_ = 1.0/256.0;
float mask_min_allowed_tile_size  = ceil(mask_min_allowed_triad_size * mask_triads_per_tile);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 tex_uv;
layout(location = 1) out float center_weight;
layout(location = 2) out float undim_mask_contrast_factors;

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

    tex_uv = TexCoord;

    //  Calculate a runtime bloom_sigma in case it's needed:
    float mask_tile_size_x    = get_resized_mask_tile_size(params.OutputSize.xy, params.OutputSize.xy * mask_resize_viewport_scale, false).x;
    float bloom_sigma_runtime = get_min_sigma_to_blur_triad(mask_tile_size_x / mask_triads_per_tile, bloom_diff_thresh_);

    center_weight = get_fast_gaussian_weight_sum_inv(bloom_sigma_runtime);

    float undim_factor = 1.0/levels_autodim_temp;

    undim_mask_contrast_factors = undim_factor * get_mask_amplify() * levels_contrast;
}

#pragma stage fragment
layout(location = 0) in vec2 tex_uv;
layout(location = 1) in float center_weight;
layout(location = 2) in float undim_mask_contrast_factors;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D MASKED_SCANLINES;
layout(set = 0, binding = 3) uniform sampler2D ORIG_LINEARIZED;

//////////////////////////////  FRAGMENT INCLUDES  //////////////////////////////

void main()
{
    //  Sample the masked scanlines:
    vec3 intensity_dim = texture(MASKED_SCANLINEStexture, tex_uv).rgb;

    //  Get the full intensity, including auto-undimming, and mask compensation:
    vec3  intensity = intensity_dim * undim_mask_contrast_factors;

    //  Sample BLOOM_APPROX to estimate what a straight blur of masked scanlines
    //  would look like, so we can estimate how much energy we'll receive from
    //  blooming neighbors:
    vec3 phosphor_blur_approx = levels_contrast * texture(ORIG_LINEARIZED, tex_uv).rgb;

    //  Compute the blur weight for the center texel and the maximum energy we
    //  expect to receive from neighbors:
    vec3  max_area_contribution_approx = max(vec3(0.0, 0.0, 0.0), phosphor_blur_approx - center_weight * intensity);

    //  Assume neighbors will blur 100% of their intensity (blur_ratio = 1.0),
    //  because it actually gets better results (on top of being very simple),
    //  but adjust all intensities for the user's desired underestimate factor:
    vec3 area_contrib_underestimate = bloom_underestimate_levels * max_area_contribution_approx;
    vec3 intensity_underestimate    = bloom_underestimate_levels * intensity;

    //  Calculate the blur_ratio, the ratio of intensity we want to blur:
    vec3 blur_ratio_temp = ((vec3(1.0, 1.0, 1.0) - area_contrib_underestimate) / intensity_underestimate - vec3(1.0, 1.0, 1.0)) / (center_weight - 1.0);
    vec3 blur_ratio      = clamp(blur_ratio_temp, 0.0, 1.0);

    //  Calculate the brightpass based on the auto-dimmed, unamplified, masked
    //  scanlines, encode if necessary, and return!
    vec3 brightpass = intensity_dim * mix(blur_ratio, vec3(1.0, 1.0, 1.0), global.bloom_excess);

    FragColor = vec4(brightpass, 1.0);
}
