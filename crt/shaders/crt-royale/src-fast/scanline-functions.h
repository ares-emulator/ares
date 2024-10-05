#ifndef SCANLINE_FUNCTIONS_H
#define SCANLINE_FUNCTIONS_H

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

#include "special-functions.h"

/////////////////////////////  SCANLINE FUNCTIONS  /////////////////////////////

vec3 get_gaussian_sigma(vec3 color, float sigma_range)
{
    if(beam_spot_shape_function < 0.5)
    {
        //  Use a power function:
        return vec3(beam_min_sigma) + sigma_range * pow(color, vec3(beam_spot_power));
    }
    else
    {
        //  Use a spherical function:
        vec3 color_minus_1 = color - vec3(1.0);

	return vec3(beam_min_sigma) + sigma_range * sqrt(vec3(1.0) - color_minus_1*color_minus_1);
    }
}

vec3 get_generalized_gaussian_beta(vec3 color, float shape_range)
{
    return beam_min_shape + shape_range * pow(color, vec3(beam_shape_power));
}

vec3 scanline_gaussian_integral_contrib(vec3 dist, vec3 color, float pixel_height, float sigma_range)
{
    vec3 sigma         = get_gaussian_sigma(color, sigma_range);
    vec3 ph_offset     = vec3(pixel_height * 0.5);
    vec3 denom_inv     = 1.0/(sigma*sqrt(2.0));
    vec3 integral_high = erf((dist + ph_offset)*denom_inv);
    vec3 integral_low  = erf((dist - ph_offset)*denom_inv);

    return color * 0.5*(integral_high - integral_low)/pixel_height;
}

vec3 scanline_generalized_gaussian_integral_contrib(vec3 dist, vec3 color, float pixel_height, float sigma_range, float shape_range)
{
    vec3 alpha     = sqrt(2.0) * get_gaussian_sigma(color, sigma_range);
    vec3 beta      = get_generalized_gaussian_beta(color, shape_range);
    vec3 alpha_inv = vec3(1.0)/alpha;
    vec3 s         = vec3(1.0)/beta;
    vec3 ph_offset = vec3(pixel_height * 0.5);

    vec3 gamma_s_inv   = vec3(1.0)/gamma_impl(s, beta);
    vec3 dist1         = dist + ph_offset;
    vec3 dist0         = dist - ph_offset;
    vec3 integral_high = sign(dist1) * normalized_ligamma_impl(s, pow(abs(dist1)*alpha_inv, beta), beta, gamma_s_inv);
    vec3 integral_low  = sign(dist0) * normalized_ligamma_impl(s, pow(abs(dist0)*alpha_inv, beta), beta, gamma_s_inv);

    return color * 0.5*(integral_high - integral_low)/pixel_height;
}

vec3 scanline_gaussian_sampled_contrib(vec3 dist, vec3 color, float pixel_height, float sigma_range)
{
    vec3 sigma     = get_gaussian_sigma(color, sigma_range);
    vec3 sigma_inv = vec3(1.0)/sigma;

    vec3 inner_denom_inv = 0.5 * sigma_inv * sigma_inv;
    vec3 outer_denom_inv = sigma_inv/sqrt(2.0*pi);

    if(beam_antialias_level > 0.5)
    {
        //  Sample 1/3 pixel away in each direction as well:
        vec3 sample_offset = vec3(pixel_height/3.0);
        vec3 dist2         = dist + sample_offset;
        vec3 dist3         = abs(dist - sample_offset);

	//  Average three pure Gaussian samples:
        vec3 scale   = color/3.0  * outer_denom_inv;
        vec3 weight1 = exp(-( dist* dist)*inner_denom_inv);
        vec3 weight2 = exp(-(dist2*dist2)*inner_denom_inv);
        vec3 weight3 = exp(-(dist3*dist3)*inner_denom_inv);

	return scale * (weight1 + weight2 + weight3);
    }
    else
    {
        return color*exp(-(dist*dist)*inner_denom_inv)*outer_denom_inv;
    }
}

vec3 scanline_generalized_gaussian_sampled_contrib(vec3 dist, vec3 color, float pixel_height, float sigma_range, float shape_range)
{
    vec3 alpha = sqrt(2.0) * get_gaussian_sigma(color, sigma_range);
    vec3 beta  = get_generalized_gaussian_beta(color, shape_range);

    //  Avoid repeated divides:
    vec3 alpha_inv = vec3(1.0)/alpha;
    vec3 beta_inv  = vec3(1.0)/beta;
    vec3 scale     = color * beta * 0.5 * alpha_inv / gamma_impl(beta_inv, beta);

    if(beam_antialias_level > 0.5)
    {
        //  Sample 1/3 pixel closer to and farther from the scanline too.
        vec3 sample_offset = vec3(pixel_height/3.0);
        vec3 dist2         = dist + sample_offset;
        vec3 dist3         = abs(dist - sample_offset);

	//  Average three generalized Gaussian samples:
        vec3 weight1 = exp(-pow(abs( dist*alpha_inv), beta));
        vec3 weight2 = exp(-pow(abs(dist2*alpha_inv), beta));
        vec3 weight3 = exp(-pow(abs(dist3*alpha_inv), beta));

	return scale/3.0 * (weight1 + weight2 + weight3);
    }
    else
    {
        return scale * exp(-pow(abs(dist*alpha_inv), beta));
    }
}

vec3 scanline_contrib(vec3 dist, vec3 color, float pixel_height, float sigma_range, float shape_range)
{
    if(beam_generalized_gaussian)
    {
        if(beam_antialias_level > 1.5)
        {
            return scanline_generalized_gaussian_integral_contrib(dist, color, pixel_height, sigma_range, shape_range);
        }
        else
        {
            return scanline_generalized_gaussian_sampled_contrib(dist, color, pixel_height, sigma_range, shape_range);
        }
    }
    else
    {
        if(beam_antialias_level > 1.5)
        {
            return scanline_gaussian_integral_contrib(dist, color, pixel_height, sigma_range);
        }
        else
        {
            return scanline_gaussian_sampled_contrib(dist, color, pixel_height, sigma_range);
        }
    }
}


// 2 - Apply mask only.
vec3 get_raw_interpolated_color(vec3 color0, vec3 color1, vec3 color2, vec3 color3, vec4 weights)
{
    //  Use max to avoid bizarre artifacts from negative colors:
    return max((mat4x3(color0, color1, color2, color3) * weights), 0.0);
}

vec3 get_interpolated_linear_color(vec3 color0, vec3 color1, vec3 color2, vec3 color3, vec4 weights)
{
    float intermediate_gamma = lcd_gamma;

    //  Inputs: color0-3 are colors in linear RGB.
            vec3 linear_mixed_color = get_raw_interpolated_color(color0, color1, color2, color3, weights);

	    vec3 gamma_mixed_color = get_raw_interpolated_color(
                    pow(color0, vec3(1.0/intermediate_gamma)),
                    pow(color1, vec3(1.0/intermediate_gamma)),
                    pow(color2, vec3(1.0/intermediate_gamma)),
                    pow(color3, vec3(1.0/intermediate_gamma)),
                    weights);
			// wtf fixme
//			float beam_horiz_linear_rgb_weight1 = 1.0;
            return mix(gamma_mixed_color, linear_mixed_color, global.beam_horiz_linear_rgb_weight);
}

vec3 get_scanline_color(sampler2D tex, vec2 scanline_uv, vec2 uv_step_x, vec4 weights)
{
    vec3 color1 = texture(tex, scanline_uv).rgb;
    vec3 color2 = texture(tex, scanline_uv + uv_step_x).rgb;
    vec3 color0 = vec3(0.0);
    vec3 color3 = vec3(0.0);

    if(beam_horiz_filter > 0.5)
    {
        color0 = texture(tex, scanline_uv - uv_step_x).rgb;
        color3 = texture(tex, scanline_uv + 2.0 * uv_step_x).rgb;
    }

    return get_interpolated_linear_color(color0, color1, color2, color3, weights);
}

vec3 sample_single_scanline_horizontal(sampler2D tex, vec2 tex_uv, vec2 tex_size, vec2 texture_size_inv)
{
    vec2 curr_texel = tex_uv * tex_size;

    //  Use under_half to fix a rounding bug right around exact texel locations.
    vec2 prev_texel        = floor(curr_texel - vec2(under_half)) + vec2(0.5);
    vec2 prev_texel_hor    = vec2(prev_texel.x, curr_texel.y);
    vec2 prev_texel_hor_uv = prev_texel_hor * texture_size_inv;

    float prev_dist   = curr_texel.x - prev_texel_hor.x;
    vec4 sample_dists = vec4(1.0 + prev_dist, prev_dist, 1.0 - prev_dist, 2.0 - prev_dist);

    //  Get Quilez, Lanczos2, or Gaussian resize weights for 2/4 nearby texels:
    vec4 weights;

    if(beam_horiz_filter < 0.5)
    {
        //  Quilez:
        float x  = sample_dists.y;
        float w2 = x*x*x*(x*(x*6.0 - 15.0) + 10.0);
        weights  = vec4(0.0, 1.0 - w2, w2, 0.0);
    }
    else if(beam_horiz_filter < 1.5)
    {
        //  Gaussian:
        float inner_denom_inv = 1.0/(2.0*beam_horiz_sigma*beam_horiz_sigma);
        weights               = exp(-(sample_dists*sample_dists)*inner_denom_inv);
    }
    else if(beam_horiz_filter < 2.5)
    {
        //  Lanczos2:
        vec4 pi_dists = FIX_ZERO(sample_dists * pi);
        weights       = 2.0 * sin(pi_dists) * sin(pi_dists * 0.5)/(pi_dists * pi_dists);
    }
    else
    {
        //  Linear:
        float x  = sample_dists.y;
        weights  = vec4(0.0, 1.0 - x, x, 0.0);
    }

    //  Ensure the weight sum == 1.0:
    vec4 final_weights = weights/dot(weights, vec4(1.0));

    //  Get the interpolated horizontal scanline color:
    vec2 uv_step_x = vec2(texture_size_inv.x, 0.0);

    return get_scanline_color(tex, prev_texel_hor_uv, uv_step_x, final_weights);
}

vec3 sample_rgb_scanline_horizontal(sampler2D tex, vec2 tex_uv, vec2 tex_size, vec2 texture_size_inv)
{
    //  TODO: Add function requirements.
    //  Rely on a helper to make convergence easier.
    if(beam_misconvergence)
    {
        vec3 convergence_offsets_rgb = get_convergence_offsets_x_vector();
        vec3 offset_u_rgb = convergence_offsets_rgb * texture_size_inv.xxx;

	vec2 scanline_uv_r = tex_uv - vec2(offset_u_rgb.r, 0.0);
        vec2 scanline_uv_g = tex_uv - vec2(offset_u_rgb.g, 0.0);
        vec2 scanline_uv_b = tex_uv - vec2(offset_u_rgb.b, 0.0);

	vec3 sample_r = sample_single_scanline_horizontal(tex, scanline_uv_r, tex_size, texture_size_inv);
        vec3 sample_g = sample_single_scanline_horizontal(tex, scanline_uv_g, tex_size, texture_size_inv);
        vec3 sample_b = sample_single_scanline_horizontal(tex, scanline_uv_b, tex_size, texture_size_inv);

	return vec3(sample_r.r, sample_g.g, sample_b.b);
    }
    else
    {
        return sample_single_scanline_horizontal(tex, tex_uv, tex_size, texture_size_inv);
    }
}

// Monolythic
vec2 get_last_scanline_uv( vec2 tex_uv,
		           vec2 tex_size, vec2 texture_size_inv, 
		           vec2 il_step_multiple,
                           float frame_count, out float dist)
{
    float field_offset = floor(il_step_multiple.y * 0.75)*mod(frame_count + float(global.interlace_bff), 2.0);
    vec2  curr_texel   = tex_uv * tex_size;

    //  Use under_half to fix a rounding bug right around exact texel locations.
    vec2  prev_texel_num     = floor(curr_texel - vec2(under_half));
    float wrong_field        = mod(prev_texel_num.y + field_offset, il_step_multiple.y);
    vec2  scanline_texel_num = prev_texel_num - vec2(0.0, wrong_field);

    //  Snap to the center of the previous scanline in the current field:
    vec2  scanline_texel = scanline_texel_num + vec2(0.5);
    vec2  scanline_uv    = scanline_texel * texture_size_inv;

    //  Save the sample's distance from the scanline, in units of scanlines:
    dist = (curr_texel.y - scanline_texel.y)/il_step_multiple.y;

    return scanline_uv;
}

bool is_interlaced(float num_lines)
{
    //  Detect interlacing based on the number of lines in the source.
    if(interlace_detect)
    {
        //  NTSC: 525 lines, 262.5/field; 486 active (2 half-lines), 243/field
        //  NTSC Emulators: Typically 224 or 240 lines
        //  PAL: 625 lines, 312.5/field; 576 active (typical), 288/field
        //  PAL Emulators: ?
        //  ATSC: 720p, 1080i, 1080p
        //  Where do we place our cutoffs?  Assumptions:
        //  1.) We only need to care about active lines.
        //  2.) Anything > 288 and <= 576 lines is probably interlaced.
        //  3.) Anything > 576 lines is probably not interlaced...
        //  4.) ...except 1080 lines, which is a crapshoot (user decision).
        //  5.) Just in case the main program uses calculated video sizes,
        //      we should nudge the float thresholds a bit.
        bool sd_interlace = ((num_lines > 288.5) && (num_lines < 576.5));
        bool hd_interlace = bool(interlace_1080i) ? ((num_lines > 1079.5) && (num_lines < 1080.5)) : false;

	return (sd_interlace || hd_interlace);
    }
    else
    {
        return false;
    }
}


#endif  //  SCANLINE_FUNCTIONS_H

