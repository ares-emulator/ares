#ifndef BIND_SHADER_PARAMS_H
#define BIND_SHADER_PARAMS_H

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


/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	float crt_gamma;
	float lcd_gamma;
	float levels_contrast;
	float bloom_underestimate_levels;
	float bloom_excess;
	float beam_min_sigma;
	float beam_max_sigma;
	float beam_spot_power;
	float beam_min_shape;
	float beam_max_shape;
	float beam_shape_power;
	float beam_horiz_filter;
	float beam_horiz_sigma;
	float beam_horiz_linear_rgb_weight;
	float convergence_offset_x_r;
	float convergence_offset_x_g;
	float convergence_offset_x_b;
	float convergence_offset_y_r;
	float convergence_offset_y_g;
	float convergence_offset_y_b;
	float mask_type;
//	float mask_sample_mode_desired;
	float mask_num_triads_desired;
	float mask_triad_size_desired;
	float mask_specify_num_triads;
	float aa_subpixel_r_offset_x_runtime;
	float aa_subpixel_r_offset_y_runtime;
	float aa_cubic_c;
	float aa_gauss_sigma;
	float interlace_bff;
	float interlace_1080i;
	float interlace_detect_toggle;
} global;

#include "user-settings.h"
#include "derived-settings-and-constants.h"

//  Override some parameters for gamma-management.h and tex2Dantialias.h:
#define OVERRIDE_DEVICE_GAMMA
float gba_gamma = 3.5; //  Irrelevant but necessary to define.
#define ANTIALIAS_OVERRIDE_BASICS
#define ANTIALIAS_OVERRIDE_PARAMETERS

#pragma parameter crt_gamma "Simulated CRT Gamma" 2.4 1.0 5.0 0.025
#define crt_gamma global.crt_gamma
#pragma parameter lcd_gamma "Your Display Gamma" 2.4 1.0 5.0 0.025
#define lcd_gamma global.lcd_gamma
#pragma parameter levels_contrast "Contrast" 0.671875 0.0 4.0 0.015625
#define levels_contrast global.levels_contrast
#pragma parameter bloom_underestimate_levels "Bloom - Underestimate Levels" 1.0 0.0 5.0 0.01
#define bloom_underestimate_levels global.bloom_underestimate_levels
#pragma parameter bloom_excess "Bloom - Excess" 0.0 0.0 1.0 0.005
#pragma parameter beam_min_sigma "Beam - Min Sigma" 0.055 0.005 1.0 0.005
#define beam_min_sigma global.beam_min_sigma
#pragma parameter beam_max_sigma "Beam - Max Sigma" 0.2 0.005 1.0 0.005
#define beam_max_sigma global.beam_max_sigma
#pragma parameter beam_spot_power "Beam - Spot Power" 0.38 0.01 16.0 0.01
#define beam_spot_power global.beam_spot_power
#pragma parameter beam_min_shape "Beam - Min Shape" 2.0 2.0 32.0 0.1
#define beam_min_shape global.beam_min_shape
#pragma parameter beam_max_shape "Beam - Max Shape" 2.0 2.0 32.0 0.1
#define beam_max_shape global.beam_max_shape
#pragma parameter beam_shape_power "Beam - Shape Power" 0.25 0.01 16.0 0.01
#define beam_shape_power global.beam_shape_power
#pragma parameter beam_horiz_filter "Beam - Horiz Filter" 0.0 0.0 3.0 1.0
#define beam_horiz_filter global.beam_horiz_filter
#pragma parameter beam_horiz_sigma "Beam - Horiz Sigma" 0.35 0.0 0.67 0.005
#define beam_horiz_sigma global.beam_horiz_sigma
#pragma parameter beam_horiz_linear_rgb_weight "Beam - Horiz Linear RGB Weight" 1.0 0.0 1.0 0.01
#pragma parameter convergence_offset_x_r "Convergence - Offset X Red" 0.0 -4.0 4.0 0.05
#define convergence_offset_x_r global.convergence_offset_x_r
#pragma parameter convergence_offset_x_g "Convergence - Offset X Green" 0.0 -4.0 4.0 0.05
#define convergence_offset_x_g global.convergence_offset_x_g
#pragma parameter convergence_offset_x_b "Convergence - Offset X Blue" 0.0 -4.0 4.0 0.05
#define convergence_offset_x_b global.convergence_offset_x_b
#pragma parameter convergence_offset_y_r "Convergence - Offset Y Red" 0.05 -2.0 2.0 0.05
#define convergence_offset_y_r global.convergence_offset_y_r
#pragma parameter convergence_offset_y_g "Convergence - Offset Y Green" -0.05 -2.0 2.0 0.05
#define convergence_offset_y_g global.convergence_offset_y_g
#pragma parameter convergence_offset_y_b "Convergence - Offset Y Blue" 0.05 -2.0 2.0 0.05
#define convergence_offset_y_b global.convergence_offset_y_b
#pragma parameter mask_type "Mask - Type" 0.0 0.0 2.0 1.0
#define mask_type global.mask_type
//#pragma parameter mask_sample_mode_desired "Mask - Sample Mode" 0.0 0.0 2.0 1.0   //  Consider blocking mode 2.
//#define mask_sample_mode_desired global.mask_sample_mode_desired
#pragma parameter mask_specify_num_triads "Mask - Specify Number of Triads" 0.0 0.0 1.0 1.0
#pragma parameter mask_triad_size_desired "Mask - Triad Size Desired" 3.0 1.0 18.0 0.125
#pragma parameter mask_num_triads_desired "Mask - Number of Triads Desired" 480.0 342.0 1920.0 1.0
#pragma parameter aa_subpixel_r_offset_x_runtime "AA - Subpixel R Offset X" -0.333333333 -0.333333333 0.333333333 0.333333333
#define aa_subpixel_r_offset_x_runtime global.aa_subpixel_r_offset_x_runtime
#pragma parameter aa_subpixel_r_offset_y_runtime "AA - Subpixel R Offset Y" 0.0 -0.333333333 0.333333333 0.333333333
#define aa_subpixel_r_offset_y_runtime global.aa_subpixel_r_offset_y_runtime
#pragma parameter aa_cubic_c "AA - Cubic Sharpness" 0.5 0.0 4.0 0.015625
#define aa_cubic_c global.aa_cubic_c
#pragma parameter aa_gauss_sigma "AA - Gaussian Sigma" 0.5 0.0625 1.0 0.015625
#define aa_gauss_sigma global.aa_gauss_sigma
#pragma parameter interlace_detect_toggle "Interlacing - Toggle" 1.0 0.0 1.0 1.0
bool interlace_detect = bool(global.interlace_detect_toggle);
#pragma parameter interlace_bff "Interlacing - Bottom Field First" 0.0 0.0 1.0 1.0
//#define interlace_bff global.interlace_bff
#pragma parameter interlace_1080i "Interlace - Detect 1080i" 0.0 0.0 1.0 1.0
#define interlace_1080i global.interlace_1080i

//  Provide accessors for vector constants that pack scalar uniforms:
vec2 get_aspect_vector(const float geom_aspect_ratio)
{
    //  Get an aspect ratio vector.  Enforce geom_max_aspect_ratio, and prevent
    //  the absolute scale from affecting the uv-mapping for curvature:
    const float geom_clamped_aspect_ratio =
        min(geom_aspect_ratio, geom_max_aspect_ratio);
    const vec2 geom_aspect =
        normalize(vec2(geom_clamped_aspect_ratio, 1.0));
    return geom_aspect;
}


vec3 get_convergence_offsets_x_vector()
{
    return vec3(convergence_offset_x_r, convergence_offset_x_g,
        convergence_offset_x_b);
}

vec3 get_convergence_offsets_y_vector()
{
    return vec3(convergence_offset_y_r, convergence_offset_y_g,
        convergence_offset_y_b);
}

vec2 get_convergence_offsets_r_vector()
{
    return vec2(convergence_offset_x_r, convergence_offset_y_r);
}

vec2 get_convergence_offsets_g_vector()
{
    return vec2(convergence_offset_x_g, convergence_offset_y_g);
}

vec2 get_convergence_offsets_b_vector()
{
    return vec2(convergence_offset_x_b, convergence_offset_y_b);
}

vec2 get_aa_subpixel_r_offset()
{
    return aa_subpixel_r_offset_static;
}

//  Provide accessors settings which still need "cooking:"
float get_mask_amplify()
{
    float mask_grille_amplify = 1.0/mask_grille_avg_color;
    float mask_slot_amplify = 1.0/mask_slot_avg_color;
    float mask_shadow_amplify = 1.0/mask_shadow_avg_color;
    return mask_type < 0.5 ? mask_grille_amplify :
        mask_type < 1.5 ? mask_slot_amplify :
        mask_shadow_amplify;
}

//float get_mask_sample_mode()
//{
//    return mask_sample_mode_desired;
//}

#endif  //  BIND_SHADER_PARAMS_H
