#ifndef PHOSPHOR_MASK_RESIZING_H
#define PHOSPHOR_MASK_RESIZING_H

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


//////////////////////////////////  INCLUDES  //////////////////////////////////


/////////////////////////////  CODEPATH SELECTION  /////////////////////////////

        #define USE_SINGLE_STATIC_LOOP

//////////////////////////////////  CONSTANTS  /////////////////////////////////

//  The larger the resized tile, the fewer samples we'll need for downsizing.
//  See if we can get a static min tile size > mask_min_allowed_tile_size:
float mask_min_allowed_tile_size  = ceil(mask_min_allowed_triad_size * mask_triads_per_tile);
float mask_min_expected_tile_size = mask_min_allowed_tile_size;

//  Limit the number of sinc resize taps by the maximum minification factor:
float pi_over_lobes = pi/mask_sinc_lobes;
float max_sinc_resize_samples_float = 2.0 * mask_sinc_lobes * mask_resize_src_lut_size.x/mask_min_expected_tile_size;

//  Vectorized loops sample in multiples of 4.  Round up to be safe:
float max_sinc_resize_samples_m4 = ceil(max_sinc_resize_samples_float * 0.25) * 4.0;


/////////////////////////  RESAMPLING FUNCTION HELPERS  ////////////////////////

float get_dynamic_loop_size(float magnification_scale)
{

    float min_samples_float = 2.0 * mask_sinc_lobes / magnification_scale;
    float min_samples_m4 = ceil(min_samples_float * 0.25) * 4.0;

    //  Simulating loops with branches imposes a 128-sample limit.
    float max_samples_m4 = min(128.0, max_sinc_resize_samples_m4);

    return min(min_samples_m4, max_samples_m4);
}

vec2 get_first_texel_tile_uv_and_dist(vec2 tex_uv, vec2 tex_size, float dr, float input_tiles_per_texture_r, float samples, bool vertical)
{

    vec2 curr_texel  = tex_uv * tex_size;
    vec2 prev_texel  = floor(curr_texel - vec2(under_half)) + vec2(0.5);
    vec2 first_texel = prev_texel - vec2(samples/2.0 - 1.0);
    vec2 first_texel_uv_wrap_2D = first_texel * dr;
    vec2 first_texel_dist_2D    = curr_texel - first_texel;

    //  Convert from tex_uv to tile_uv coords so we can sub fracts for fmods.
    vec2 first_texel_tile_uv_wrap_2D = first_texel_uv_wrap_2D * input_tiles_per_texture_r;

    //  Project wrapped coordinates to the [0, 1] range.  We'll do this with all
    //  samples,but the first texel is special, since it might be negative.
    vec2 coord_negative = vec2((first_texel_tile_uv_wrap_2D.x < 0.),(first_texel_tile_uv_wrap_2D.y < 0.));
    vec2 first_texel_tile_uv_2D = fract(first_texel_tile_uv_wrap_2D) + coord_negative;

    //  Pack the first texel's tile_uv coord and texel distance in 1D:
    vec2 tile_u_and_dist = vec2(first_texel_tile_uv_2D.x, first_texel_dist_2D.x);
    vec2 tile_v_and_dist = vec2(first_texel_tile_uv_2D.y, first_texel_dist_2D.y);

    return vertical ? tile_v_and_dist : tile_u_and_dist;
}

vec4 tex2Dlod0try(sampler2D tex, vec2 tex_uv)
{
    //  Mipmapping and anisotropic filtering get confused by sinc-resampling.
    //  One [slow] workaround is to select the lowest mip level:
    return texture(tex, tex_uv);
}


//////////////////////////////  LOOP BODY MACROS  //////////////////////////////


    #define CALCULATE_R_COORD_FOR_4_SAMPLES                                    \
        vec4 true_i = vec4(i_base + i) + vec4(0.0, 1.0, 2.0, 3.0); \
        vec4 tile_uv_r = fract(                                         \
            first_texel_tile_uv_rrrr + true_i * tile_dr);                      \
        vec4 tex_uv_r = tile_uv_r * tile_size_uv_r;

    #ifdef PHOSPHOR_MASK_RESIZE_LANCZOS_WINDOW
        #define CALCULATE_SINC_RESAMPLE_WEIGHTS                                \
            vec4 pi_dist_over_lobes = pi_over_lobes * dist;            \
            vec4 weights = min(sin(pi_dist) * sin(pi_dist_over_lobes) /\
                (pi_dist*pi_dist_over_lobes), vec4(1.0));
    #else
        #define CALCULATE_SINC_RESAMPLE_WEIGHTS                                \
            vec4 weights = min(sin(pi_dist)/pi_dist, vec4(1.0));
    #endif

    #define UPDATE_COLOR_AND_WEIGHT_SUMS                                       \
        vec4 dist = magnification_scale *                              \
            abs(first_dist_unscaled - true_i);                                 \
        vec4 pi_dist = pi * dist;                                      \
        CALCULATE_SINC_RESAMPLE_WEIGHTS;                                       \
        pixel_color += new_sample0 * weights.xxx;                              \
        pixel_color += new_sample1 * weights.yyy;                              \
        pixel_color += new_sample2 * weights.zzz;                              \
        pixel_color += new_sample3 * weights.www;                              \
        weight_sum += weights;

    #define VERTICAL_SINC_RESAMPLE_LOOP_BODY                                   \
        CALCULATE_R_COORD_FOR_4_SAMPLES;                                       \
        vec3 new_sample0 = tex2Dlod0try(tex,                       \
            vec2(tex_uv.x, tex_uv_r.x)).rgb;                                 \
        vec3 new_sample1 = tex2Dlod0try(tex,                       \
            vec2(tex_uv.x, tex_uv_r.y)).rgb;                                 \
        vec3 new_sample2 = tex2Dlod0try(tex,                       \
            vec2(tex_uv.x, tex_uv_r.z)).rgb;                                 \
        vec3 new_sample3 = tex2Dlod0try(tex,                       \
            vec2(tex_uv.x, tex_uv_r.w)).rgb;                                 \
        UPDATE_COLOR_AND_WEIGHT_SUMS;

    #define HORIZONTAL_SINC_RESAMPLE_LOOP_BODY                                 \
        CALCULATE_R_COORD_FOR_4_SAMPLES;                                       \
        vec3 new_sample0 = tex2Dlod0try(tex,                       \
            vec2(tex_uv_r.x, tex_uv.y)).rgb;                                 \
        vec3 new_sample1 = tex2Dlod0try(tex,                       \
            vec2(tex_uv_r.y, tex_uv.y)).rgb;                                 \
        vec3 new_sample2 = tex2Dlod0try(tex,                       \
            vec2(tex_uv_r.z, tex_uv.y)).rgb;                                 \
        vec3 new_sample3 = tex2Dlod0try(tex,                       \
            vec2(tex_uv_r.w, tex_uv.y)).rgb;                                 \
        UPDATE_COLOR_AND_WEIGHT_SUMS;


////////////////////////////  RESAMPLING FUNCTIONS  ////////////////////////////

vec3 downsample_vertical_sinc_tiled(sampler2D tex, vec2 tex_uv, vec2 tex_size, float dr, float magnification_scale, float tile_size_uv_r)
{
    int samples = int(max_sinc_resize_samples_m4);

    //  Get the first sample location (scalar tile uv coord along the resized
    //  dimension) and distance from the output location (in texels):
    float input_tiles_per_texture_r = 1.0/tile_size_uv_r;

    //  true = vertical resize:
    vec2 first_texel_tile_r_and_dist = get_first_texel_tile_uv_and_dist(tex_uv, tex_size, dr, input_tiles_per_texture_r, samples, true);
    vec4 first_texel_tile_uv_rrrr    = first_texel_tile_r_and_dist.xxxx;
    vec4 first_dist_unscaled         = first_texel_tile_r_and_dist.yyyy;

    //  Get the tile sample offset:
    float tile_dr = dr * input_tiles_per_texture_r;

    //  Sum up each weight and weighted sample color, varying the looping
    //  strategy based on our expected dynamic loop capabilities.  See the
    //  loop body macros above.
    int i_base = 0;
    int i_step = 4;
    vec4 weight_sum  = vec4(0.0);
    vec3 pixel_color = vec3(0.0);
 
    for(int i = 0; i < samples; i += i_step)
    {
        VERTICAL_SINC_RESAMPLE_LOOP_BODY;
    }
 
    //  Normalize so the weight_sum == 1.0, and return:
    vec2 weight_sum_reduce = weight_sum.xy + weight_sum.zw;
    vec3 scalar_weight_sum = vec3(weight_sum_reduce.x + weight_sum_reduce.y);

    return (pixel_color/scalar_weight_sum);
}

vec3 downsample_horizontal_sinc_tiled(sampler2D tex, vec2 tex_uv, vec2 tex_size, float dr, float magnification_scale, float tile_size_uv_r)
{
    int samples = int(max_sinc_resize_samples_m4);

    //  Get the first sample location (scalar tile uv coord along resized
    //  dimension) and distance from the output location (in texels):
    float input_tiles_per_texture_r = 1.0/tile_size_uv_r;

    //  false = horizontal resize:
    vec2 first_texel_tile_r_and_dist = get_first_texel_tile_uv_and_dist(tex_uv, tex_size, dr, input_tiles_per_texture_r, samples, false);
    vec4 first_texel_tile_uv_rrrr    = first_texel_tile_r_and_dist.xxxx;
    vec4 first_dist_unscaled         = first_texel_tile_r_and_dist.yyyy;

    //  Get the tile sample offset:
    float tile_dr = dr * input_tiles_per_texture_r;

    //  Sum up each weight and weighted sample color, varying the looping
    //  strategy based on our expected dynamic loop capabilities.  See the
    //  loop body macros above.
    int i_base = 0;
    int i_step = 4;
    vec4 weight_sum  = vec4(0.0);
    vec3 pixel_color = vec3(0.0);

    for(int i = 0; i < samples; i += i_step)
    {
        HORIZONTAL_SINC_RESAMPLE_LOOP_BODY;
    }

    //  Normalize so the weight_sum == 1.0, and return:
    vec2 weight_sum_reduce = weight_sum.xy + weight_sum.zw;
    vec3 scalar_weight_sum = vec3(weight_sum_reduce.x + weight_sum_reduce.y);

    return (pixel_color/scalar_weight_sum);
}


////////////////////////////  TILE SIZE CALCULATION  ///////////////////////////

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


/////////////////////////  FINAL MASK SAMPLING HELPERS  ////////////////////////

vec4 get_mask_sampling_parameters(vec2 mask_resize_texture_size, vec2 mask_resize_video_size, vec2 true_viewport_size, out vec2 mask_tiles_per_screen)
{
    vec2 mask_resize_tile_size = get_resized_mask_tile_size(true_viewport_size, mask_resize_video_size, false);

    //  Sample MASK_RESIZE: The resized tile is a fracttion of the texture
    //  size and starts at a nonzero offset to allow for border texels:
    vec2 mask_tile_uv_size  = mask_resize_tile_size / mask_resize_texture_size;
    vec2 skipped_tiles      = mask_start_texels/mask_resize_tile_size;
    vec2 mask_tile_start_uv = skipped_tiles * mask_tile_uv_size;
    
    //  mask_tiles_per_screen must be based on the *true* viewport size:
    mask_tiles_per_screen = true_viewport_size / mask_resize_tile_size;
    
    return vec4(mask_tile_start_uv, mask_tile_uv_size);
}

vec2 convert_phosphor_tile_uv_wrap_to_tex_uv(vec2 tile_uv_wrap, vec4 mask_tile_start_uv_and_size)
{
    vec2 tile_uv = fract(tile_uv_wrap);
    vec2 mask_tex_uv = mask_tile_start_uv_and_size.xy + tile_uv * mask_tile_start_uv_and_size.zw;

    return mask_tex_uv;
}


#endif  //  PHOSPHOR_MASK_RESIZING_H

