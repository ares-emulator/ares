/*
    Mega Bezel - Creates a graphic treatment for the game play area to give a retro feel
    Copyright (C) 2019-2022 HyperspaceMadness - HyperspaceMadness@outlook.com

    Incorporates much great feedback from the libretro forum, and thanks 
    to Hunterk who helped me get started

    See more at the libretro forum
    https://forums.libretro.com/t/hsm-mega-bezel-reflection-shader-feedback-and-updates

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

// Float format so we can avoid banding
#pragma format R16G16B16A16_SFLOAT

/////////////// IMPORTS ///////////////
#include "common/common-functions-bezel.inc"

vec2 VIEWPORT_COORD = vec2(0.5);

/////////////// Helper Functions ///////////////

layout(push_constant) uniform Push
{
	vec4 BR_MirrorBlurredPassSize;
	vec4 BR_MirrorReflectionDiffusedPassSize;
	vec4 BR_MirrorFullscreenGlowPassSize;
	
} params;

//TODO remove this and replace with simpler calls
float GetFade(float current_position, float corner_position, float fade_distance)
{
	return smoothstep(corner_position + fade_distance / 2, corner_position - fade_distance / 2, current_position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 6) out vec2 vTexCoord;
layout(location = 7) out vec2 UNFLIPPED_VIEWPORT_COORD;

//////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;

	UNFLIPPED_VIEWPORT_COORD = vTexCoord * 1.0001;
	vTexCoord * 1.0001;

}

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma stage fragment

layout(location = 6) in vec2 vTexCoord;
layout(location = 7) in vec2 UNFLIPPED_VIEWPORT_COORD;

layout(location = 0) out vec4 FragColor;

// Pass Framebuffer Textures
layout(set = 0, binding = 1) uniform sampler2D InfoCachePass;

layout(set = 0, binding = 2) uniform sampler2D BackgroundImage;
layout(set = 0, binding = 3) uniform sampler2D ReflectionMaskImage;

layout(set = 0, binding = 4) uniform sampler2D BR_MirrorLowResPass;
layout(set = 0, binding = 5) uniform sampler2D PostCRTPass;
layout(set = 0, binding = 6) uniform sampler2D BR_MirrorBlurredPass;
layout(set = 0, binding = 7) uniform sampler2D BR_MirrorReflectionDiffusedPass;
layout(set = 0, binding = 8) uniform sampler2D BR_MirrorFullscreenGlowPass;

layout(set = 0, binding = 9) uniform sampler2D ReflectionPassFeedback;
#define PassFeedback ReflectionPassFeedback

//////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
	
	if (HSM_AB_COMPARE_FREEZE_CRT_TUBE == 1 && HSM_GetIsInABCompareArea(vTexCoord))
	{
		FragColor = texture(PassFeedback, vTexCoord);
		return;
	}

	VIEWPORT_UNSCALED_COORD = HSM_GetViewportCoordWithFlip(vTexCoord);
	VIEWPORT_COORD = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);

	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);


	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
						TUBE_DIFFUSE_SCALE, 
						TUBE_SCALE, 
						TUBE_DIFFUSE_ASPECT, 
						true,
						BEZEL_OUTSIDE_SCALE,
						BEZEL_OUTSIDE_COORD, 
						BEZEL_OUTSIDE_CURVED_COORD, 
						FRAME_OUTSIDE_CURVED_COORD);

	if (HHLP_IsOutsideCoordSpace(BEZEL_OUTSIDE_COORD))
	{
		FragColor = vec4(0);
		return;
	}

	float avg_lum_mult = smoothstep(0.01, 0.5, pow(AVERAGE_LUMA, 1.3));

	//----------------------------------------------------
	// CRT Pass
	//----------------------------------------------------
	// Get the CRT pass and make it in linear space & mask the area outside the screen
	vec4 crt_linear = texture(PostCRTPass, UNFLIPPED_VIEWPORT_COORD.xy);

	vec4 blurred_reflection_color = HHLP_GetBilinearTextureSample(BR_MirrorBlurredPass, UNFLIPPED_VIEWPORT_COORD.xy, params.BR_MirrorBlurredPassSize);

	vec2 tube_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, 1, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 1);
	vec2 tube_curved_coord_ctr = tube_curved_coord - 0.5;
	vec2 edge_mask_coord = tube_curved_coord_ctr * (1 - (HSM_BZL_INNER_EDGE_THICKNESS / vec2(TUBE_DIFFUSE_ASPECT, 1))) + 0.5;

	float bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	if(HSM_BZL_USE_INDEPENDENT_CURVATURE > 0)
		bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;

	float edge_mask =  HSM_GetCornerMask(edge_mask_coord, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, HSM_BZL_INNER_EDGE_SHARPNESS);

	float outside_tube_mask = 1 - HSM_GetCornerMask((tube_curved_coord - 0.5) * 1.002 + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);
	TUBE_MASK = 1 - outside_tube_mask;

	OUTSIDE_BEZEL_MASK = 1 - HSM_GetCornerMask(BEZEL_OUTSIDE_CURVED_COORD, TUBE_DIFFUSE_ASPECT, HSM_GLOBAL_CORNER_RADIUS * HSM_BZL_OUTER_CORNER_RADIUS_SCALE, 0.9);

	//----------------------------------------------------
	//  Calculate Outside mapping Coords
	//----------------------------------------------------
	/* This first big chunk is to get a mapping of the space outside of the screen which is continuous
	This is more complicated than you would expect because since we are using curved coordinates 
	there are discontinuities outside the normal screen corners, e.g. where x > 1 and y > 1
	So instead of trying to use the coordinates from the screen/tube we use a larger space 
	and subtract the screen space to see how far we are outside of the sreen
	*/

	// Additional scale to be applied to the tube scale to create an expanded mapping area 
	float outermap_scale = 2.3;

	// Get a range width from the outer tube edge to the outer edge of the outermap
	float outermap_range = 0.5 * outermap_scale * 0.7;
	vec2 outermap_screen_size_from_center = vec2(0.5, 0.5);
	vec2 outermap_warped_outside_screen_vector = (tube_curved_coord_ctr - clamp(tube_curved_coord_ctr, -0.490, 0.490)) * vec2(1 / TUBE_DIFFUSE_ASPECT, 1);
	float output_aspect = global.OutputSize.x / global.OutputSize.y;
	float outside_ratio_warped = clamp(length(outermap_warped_outside_screen_vector) / outermap_range, 0, 1);
	vec2 outermap_screen_corner_ctr_coord = vec2(0.5, -0.5);

	// Get a coordinate offset so it is centered around the corner
	vec2 outermap_coord_warped_ctr_at_screen_corner = abs(tube_curved_coord_ctr) - vec2(0.5);

	//----------------------------------------------------
	//  Calculate Corner Highlight Mask
	//----------------------------------------------------
	const float pi = 3.1415;

	// Get amount to shift the point at the outer corner to match the overall position offset
	vec2 pos_shift_offset = vec2(0, HSM_BZL_OUTER_POSITION_Y) * TUBE_DIFFUSE_SCALE.y / outermap_scale;
	pos_shift_offset *= tube_curved_coord.y > 0.5 ? 1 : -1;

	// Get the direction vector from the inner corner of the bezel pointing at the outer corner 
	vec2 corner_crease_dir = (outermap_screen_corner_ctr_coord + pos_shift_offset) / vec2(HSM_BZL_HEIGHT + 1, HSM_BZL_WIDTH + 1) - (outermap_screen_corner_ctr_coord) ;
	corner_crease_dir *= vec2(TUBE_DIFFUSE_ASPECT, 1);

	float aspect_corner_length_scale_offset = TUBE_DIFFUSE_ASPECT > 1 ? 0.9 : 1.5;
	float corner_crease_length = length(corner_crease_dir * aspect_corner_length_scale_offset);

	// A hack to adjust the angle offset, because without it the corner angle isn't pointing exactly at the corner
	// This offset is the opposite direction for vertical and horizontal aspect ratio
	float corner_rotation_offset = (SCREEN_COORD.y < 0.5) ? -HSM_REFLECT_CORNER_ROTATION_OFFSET_TOP : -HSM_REFLECT_CORNER_ROTATION_OFFSET_BOTTOM;

	if (HSM_CURVATURE_MODE == 0)
		// If we are using a 3d Curvature no offset is necessary
		corner_rotation_offset += (TUBE_DIFFUSE_ASPECT > 1) ? 2 : 3;

	// Convert direction vector to an angle so we can rotate the corner crease direction
	float corner_angle_degrees = atan(corner_crease_dir.y / corner_crease_dir.x) / (2 * pi) * 360;

	corner_angle_degrees += corner_rotation_offset;
	float corner_angle_radians = corner_angle_degrees / 360 * 2 * pi;
	corner_crease_dir = vec2(cos(corner_angle_radians), sin(corner_angle_radians));

	// Get the distance perpendicular to the crease direction so we can use it to fade later
	float distance_from_crease = HHLP_GetDistanceToLine(outermap_coord_warped_ctr_at_screen_corner.x, outermap_coord_warped_ctr_at_screen_corner.y, 1, corner_crease_dir.y / corner_crease_dir.x, 0 );

	float fade_out_to_corner = HHLP_QuadraticBezier(clamp(length(outermap_warped_outside_screen_vector) / (corner_crease_length * 2), 0, 1), vec2(0.5, HSM_REFLECT_CORNER_SPREAD_FALLOFF / 100));

	float corner_fade_width_inner = HSM_REFLECT_CORNER_INNER_SPREAD * (TUBE_DIFFUSE_SCALE.x + TUBE_DIFFUSE_SCALE.y) * bezel_corner_radius / 10 / 250 * 1.2;
	float corner_fade_width_outer = HSM_REFLECT_CORNER_OUTER_SPREAD * (TUBE_DIFFUSE_SCALE.x + TUBE_DIFFUSE_SCALE.y) * HSM_GLOBAL_CORNER_RADIUS * HSM_BZL_OUTER_CORNER_RADIUS_SCALE / 10 / 250 * 1.6;
	float corner_fade_width = (corner_fade_width_inner + fade_out_to_corner * (corner_fade_width_outer - corner_fade_width_inner));

	// Get a vector perpendicular to the crease that we can shift the crease to blend between bottom/top and sides
	vec2 corner_crease_perp_dir = normalize(vec2(corner_crease_dir.y, corner_crease_dir.x));
	vec2 corner_coord_shifted = outermap_coord_warped_ctr_at_screen_corner - corner_crease_perp_dir * corner_fade_width / 2;
	vec2 corner_crease_dir_shifted = corner_crease_dir - corner_crease_perp_dir * corner_fade_width / 2;

	// Get the distance to this shifted crease
	float distance_from_crease_shifted = HHLP_GetDistanceToLine(corner_coord_shifted.x, corner_coord_shifted.y, 1, corner_crease_dir_shifted.y / corner_crease_dir_shifted.x, 0 );

	float top_half_mask = smoothstep(0.55, 0.5, tube_curved_coord.y);

	// Get a mask which transitions between sides and top/bottom at the corner crease  
	float top_bottom_vs_sides_mask = dot(normalize(corner_coord_shifted), normalize(corner_crease_dir_shifted)) > 0 ? 1 - smoothstep(0, corner_fade_width / 2, distance_from_crease_shifted) : 1;

	// Masks isolating specific parts
	float sides_mask = 1 - top_bottom_vs_sides_mask;
	float top_mask = top_half_mask * top_bottom_vs_sides_mask;
	float bottom_mask = (1 -top_half_mask) * top_bottom_vs_sides_mask;

	float corner_mask = smoothstep(corner_fade_width / 2, 0, distance_from_crease);

	float top_corner_mask = corner_mask * top_half_mask;
	float bottom_corner_mask = corner_mask * (1 - top_half_mask);

	float frame_inner_edge_mask = (HSM_FRM_INNER_EDGE_THICKNESS == 0) ? 0 : 1 - HSM_GetCornerMask(	(BEZEL_OUTSIDE_CURVED_COORD - 0.5) * (1 + (HSM_FRM_INNER_EDGE_THICKNESS / vec2(TUBE_DIFFUSE_ASPECT, 1))) + 0.5, 
																										TUBE_DIFFUSE_ASPECT, 
																										HSM_GLOBAL_CORNER_RADIUS * HSM_BZL_OUTER_CORNER_RADIUS_SCALE, 
																										0.9);
	float outside_tube_mask_wider = 1 - HSM_GetCornerMask(tube_curved_coord_ctr * 0.996 + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.9);
	float tube_shadow_mask = HSM_GetCornerMask(tube_curved_coord_ctr + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0);
	float tube_edge_shadow_mult = HSM_BZL_INNER_EDGE_SHADOW * (tube_shadow_mask) + (1 - HSM_BZL_INNER_EDGE_SHADOW);

	crt_linear.rgb *= tube_edge_shadow_mult * (1 - outside_tube_mask_wider);

	float edge_highlight_mask = 0;

	// ----------------------------------------------------
	// Generated Bezel
	// ----------------------------------------------------

	float hmbz_bezel_highlight_edge = 0.9;
	float hmbz_bezel_highlight_top = 0.2;
	float hmbz_bezel_highlight_bottom = 0.3;
	float hmbz_bezel_highlight_sides = 0.2;

	float hmbz_bezel_edge_highlight_width = 0.8;
	if (HSM_GLASS_BORDER_ON == 1)
		hmbz_bezel_edge_highlight_width = 0.55;

	float edge_top_center_highlight_mask 		= hmbz_bezel_highlight_top * top_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 0));
	float edge_bottom_center_highlight_mask 	= hmbz_bezel_highlight_bottom * bottom_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 0));
	float edge_sides_highlight_mask 			= hmbz_bezel_highlight_sides * sides_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.y)), vec2(0.8, 0));

	if (HSM_GLASS_BORDER_ON == 1)
	{
		edge_top_center_highlight_mask 		= 0.6 * top_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 1));
		edge_bottom_center_highlight_mask 	= bottom_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 1));
		edge_sides_highlight_mask 			= 0.7 * sides_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.y)), vec2(0.8, 1));
	}

	edge_highlight_mask = hmbz_bezel_highlight_edge * edge_mask * (edge_top_center_highlight_mask + edge_bottom_center_highlight_mask + edge_sides_highlight_mask);

	//----------------------------------------------------
	// Reflection
	//----------------------------------------------------
	vec4 reflection_color = vec4(0);
	vec4 edge_reflection_color = vec4(0);
	vec4 glass_border_edge_color = vec4(0);
	vec4 edge_fullscreen_glow = vec4(0);
	if (HSM_REFLECT_GLOBAL_AMOUNT > 0)
	{
		// Corner Mask for Specular highlights
		float fade_out_to_corner = smoothstep(0 + HSM_REFLECT_CORNER_FADE_DISTANCE / 2, 0 - HSM_REFLECT_CORNER_FADE_DISTANCE / 2, outside_ratio_warped);
		float corner_fade_mask = (top_corner_mask + bottom_corner_mask) * (HSM_REFLECT_CORNER_FADE) * fade_out_to_corner * 2;

		corner_fade_mask *= 1 - frame_inner_edge_mask;

		// Radial fade - fading away from the edges of the screen
		float radial_fade_speed = 100;
		float radial_fade_sides = 1 - HHLP_QuadraticBezier(clamp((outside_ratio_warped / (HSM_REFLECT_RADIAL_FADE_WIDTH)), 0, 1), vec2(1 - (radial_fade_speed / 100), 1));
		float radial_fade_top_bottom = 1 - HHLP_QuadraticBezier(clamp((outside_ratio_warped / (HSM_REFLECT_RADIAL_FADE_HEIGHT)), 0, 1), vec2(1 - (radial_fade_speed / 100), 1));
		float radial_fade_mask = clamp((1 - sides_mask) * radial_fade_top_bottom
									+ sides_mask * radial_fade_sides, 0, 1);

		float radial_inner_fade_mask = clamp(0.3 + 0.7 * HHLP_QuadraticBezier(smoothstep(0.01, 0.3, outside_ratio_warped / (HSM_REFLECT_RADIAL_FADE_WIDTH)), vec2(0.1, 0.3)), 0, 1);

		// Lateral fade - Fading left to right across the bottom or top to bottom along the sides 
		float distance_ratio = smoothstep(0, 0.075, outside_ratio_warped);
		float lateral_outer_fade_distance = HSM_REFLECT_LATERAL_OUTER_FADE_DISTANCE;
		lateral_outer_fade_distance = 0.5 * lateral_outer_fade_distance + distance_ratio * 0.5 * lateral_outer_fade_distance;
		float lateral_fade_mask = (1 - sides_mask) * GetFade(abs(tube_curved_coord_ctr.x) + (HSM_REFLECT_LATERAL_OUTER_FADE_POSITION) / TUBE_DIFFUSE_ASPECT, 
																	outermap_screen_size_from_center.x,
																	outermap_screen_size_from_center.x * lateral_outer_fade_distance)
										+  sides_mask * GetFade(abs(tube_curved_coord_ctr.y) + (HSM_REFLECT_LATERAL_OUTER_FADE_POSITION) / TUBE_DIFFUSE_ASPECT, 
																outermap_screen_size_from_center.y, 
																outermap_screen_size_from_center.y * lateral_outer_fade_distance); 

		vec2 screen_coord_ctr = SCREEN_COORD - 0.5;

		float combined_fade_mask = radial_fade_mask * lateral_fade_mask;

		// Put all the fades together into one mask
		float final_fade_mask = clamp(HSM_REFLECT_FADE_AMOUNT * combined_fade_mask, 0, 1) + 1 - HSM_REFLECT_FADE_AMOUNT;

		float stoichaic_blur_samples = HSM_REFLECT_NOISE_SAMPLES;
		float noise_falloff = 0.3;
		float stoichaic_blur_max = HHLP_QuadraticBezier(outside_ratio_warped, vec2(0, noise_falloff)) * 3;
		float stoichaic_blur_amount = HSM_REFLECT_NOISE_SAMPLE_DISTANCE;

		// vec4 fullscreen_blurred_sampled_color = HSM_GetStoichaicBlurredSample(BR_MirrorFullscreenGlowPass, VIEWPORT_COORD.xy, stoichaic_blur_samples, (1 - corner_mask) * stoichaic_blur_max * 3, stoichaic_blur_amount);
		vec4 fullscreen_blurred_sampled_color = HHLP_GetBilinearTextureSample(BR_MirrorFullscreenGlowPass, UNFLIPPED_VIEWPORT_COORD.xy, params.BR_MirrorFullscreenGlowPassSize);

		vec4 corner_reflection_color = fullscreen_blurred_sampled_color;
		
		vec4 fullscreen_glow_color = pow(fullscreen_blurred_sampled_color, vec4(HSM_REFLECT_FULLSCREEN_GLOW_GAMMA));
		
		vec4 diffused_reflection_color = HHLP_GetBilinearTextureSample(BR_MirrorReflectionDiffusedPass, UNFLIPPED_VIEWPORT_COORD.xy, params.BR_MirrorReflectionDiffusedPassSize);
		vec4 diffused_reflection_scatter_color = HSM_GetStoichaicBlurredSample(BR_MirrorReflectionDiffusedPass, UNFLIPPED_VIEWPORT_COORD.xy, stoichaic_blur_samples, stoichaic_blur_max, stoichaic_blur_amount);
		vec4 diffused_reflection_blended_color = mix(diffused_reflection_color, diffused_reflection_scatter_color, HSM_REFLECT_NOISE_AMOUNT);

		vec4 blurred_reflection_scatter_color = HSM_GetStoichaicBlurredSample(BR_MirrorBlurredPass, UNFLIPPED_VIEWPORT_COORD.xy, stoichaic_blur_samples, stoichaic_blur_max, stoichaic_blur_amount);
		vec4 blurred_reflection_blended_color = mix(blurred_reflection_color, blurred_reflection_scatter_color, HSM_REFLECT_NOISE_AMOUNT);

		// Add Fullscreen Glow
		float lateral_fade_outer_mask_for_glow = (lateral_fade_mask - 1) * 0.8 + 1;

		//Combine diffused and undiffused reflection
		
		vec4 bezel_reflection_color = (HSM_REFLECT_DIRECT_AMOUNT * blurred_reflection_blended_color + HSM_REFLECT_DIFFUSED_AMOUNT * diffused_reflection_blended_color) * final_fade_mask;

		// Add Reflection from corners which is sampled from the fullscreen glow
		bezel_reflection_color += HSM_REFLECT_CORNER_FADE * 100 * corner_reflection_color * corner_fade_mask;

		// Add Fullscreen Glow
		bezel_reflection_color += HSM_REFLECT_FULLSCREEN_GLOW * 1.5 * fullscreen_glow_color * radial_inner_fade_mask * lateral_fade_outer_mask_for_glow;

		// Add Bezel and Edge reflection together
		edge_reflection_color = 1.25 * HSM_REFLECT_BEZEL_INNER_EDGE_AMOUNT * (blurred_reflection_color + 0.50 * diffused_reflection_color);
		float reflection_area_mask = outside_tube_mask;

		// Edge Reflection
		if (HSM_GLASS_BORDER_ON == 1)
		{
			float vignette_shadow_mask = 0.75 * HHLP_QuadraticBezier(1 - HSM_GetVignetteFactor(VIEWPORT_COORD, HSM_REFLECT_VIGNETTE_AMOUNT, HSM_REFLECT_VIGNETTE_SIZE), vec2(1, 0.5));
			vignette_shadow_mask += 0.5 * HSM_REFLECT_VIGNETTE_AMOUNT * HHLP_QuadraticBezier(smoothstep(outside_ratio_warped, 0, 0.1), vec2(1, 0.5));
			reflection_color += (1 - vignette_shadow_mask) * bezel_reflection_color;
			reflection_area_mask *= outside_tube_mask_wider;
		}
		else
		{
			reflection_color += (1 - edge_mask) * bezel_reflection_color;
			reflection_color += edge_mask * (edge_reflection_color + HHLP_EasePowerIn(corner_mask, 1) * corner_reflection_color);
		}

		reflection_color = HSM_REFLECT_GLOBAL_AMOUNT * pow(reflection_color, vec4(HSM_REFLECT_GLOBAL_GAMMA_ADJUST));

		// Mask reflection to only appear inside the bezel
		reflection_color = clamp(reflection_color, 0, 1) * clamp(reflection_area_mask, 0, 1);

		if (HSM_GLASS_BORDER_ON == 1)
		{
			glass_border_edge_color = HSM_REFLECT_GLOBAL_AMOUNT * pow(edge_reflection_color, vec4(HSM_REFLECT_GLOBAL_GAMMA_ADJUST));
			reflection_color = mix(reflection_color, glass_border_edge_color, edge_mask * outside_tube_mask);
		}
		// Edge Full Screen Glow
		// Add Small amount of static glow on the edge (So when the screen is dark there is some highlight) as well as dynamic light
		edge_fullscreen_glow = HSM_REFLECT_BEZEL_INNER_EDGE_FULLSCREEN_GLOW * edge_highlight_mask * outside_tube_mask * (vec4(0.005) 
									+ (avg_lum_mult  + 0.5) * (2 * fullscreen_glow_color + vec4(0.01)));
		
		// Add Diffused reflection on top of the glass inner edge
		if (HSM_GLASS_BORDER_ON == 1)
			edge_fullscreen_glow += HSM_REFLECT_BEZEL_INNER_EDGE_FULLSCREEN_GLOW * 0.5 * edge_highlight_mask * outside_tube_mask * diffused_reflection_color;

		reflection_color += clamp(edge_fullscreen_glow, 0, 1);
		reflection_color.a = 1;

		// Modulate amount of refleciton on frame inner edge
		reflection_color.rgb *= (1 - HSM_REFLECT_FRAME_INNER_EDGE_AMOUNT) * (1 - frame_inner_edge_mask) + HSM_REFLECT_FRAME_INNER_EDGE_AMOUNT;
	}

	crt_linear.rgb *= TUBE_MASK;
	crt_linear = clamp(crt_linear, 0, 1);

#ifdef IS_GLASS_PRESET
	vec4 bg_image = HSM_GetMipmappedTexSample(BackgroundImage, VIEWPORT_COORD, vec2(1), 0);
	
	bg_image = HSM_Linearize(bg_image, DEFAULT_SRGB_GAMMA);

	bg_image.a *= outside_tube_mask_wider * (1 - edge_mask);

	vec4 frag_color_linear = crt_linear;
	frag_color_linear = HSM_BlendModeLayerMix(frag_color_linear, bg_image, HSM_BG_BLEND_MODE, HSM_BG_OPACITY);
	frag_color_linear += reflection_color;

	FragColor = frag_color_linear;

// Apply output gamma if this not an HDR preset because it is the last pass
#ifndef IS_HDR_PRESET
	FragColor = HSM_Delinearize(frag_color_linear, DEFAULT_SRGB_GAMMA);
#endif

// #ifdef IS_HDR_PRESET
// 	vec3 output_colour = vec3(0);
// 	LinearToOutputColor(FragColor.rgb, output_colour);
// 	FragColor = vec4(output_colour, 1.0f);
// #endif

	return;
#endif

FragColor = crt_linear;
FragColor.rgb += reflection_color.rgb;
FragColor *= 1 - OUTSIDE_BEZEL_MASK;
return;
}