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
    along with this program.  If not, see [http://www.gnu.org/licenses/].
*/

#pragma format R32G32B32A32_SFLOAT

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
layout(set = 0, binding = 1) uniform sampler2D Source;
layout(set = 0, binding = 2) uniform sampler2D InfoCachePassFeedback;
layout(set = 0, binding = 3) uniform sampler2D ScreenPlacementImage;

// Get the original size with split area added
vec2 HSM_GetScreenCorePreppedSize(float screen_index)
{
	vec2 core_prepped_size = global.DerezedPassSize.xy;

	if (HSM_DUALSCREEN_MODE > 0)
		if (HSM_GetCoreImageSplitDirection() == 1)
			core_prepped_size.y *= 0.5;
		else
			core_prepped_size.x *= 0.5;

	return core_prepped_size;
}

vec2 HSM_GetRotatedScreenCorePreppedSize(float screen_index)
{
	vec2 original_size = HSM_GetScreenCorePreppedSize(screen_index);
	return abs(HSM_ROTATE_CORE_IMAGE) * original_size.yx + (1 - abs(HSM_ROTATE_CORE_IMAGE)) * original_size.xy;
}

vec2 HSM_GetRotatedScreenDerezedSize()
{
	vec2 pass_size = global.DerezedPassSize.xy;

	if (HSM_DUALSCREEN_MODE > 0)
		if (HSM_GetCoreImageSplitDirection() == 1)
			pass_size.y *= 0.5;
		else
			pass_size.x *= 0.5;

	return abs(HSM_ROTATE_CORE_IMAGE) * pass_size.yx + (1 - abs(HSM_ROTATE_CORE_IMAGE)) * pass_size.xy;
}

float HSM_GetUseVerticalScanlines(float screen_aspect)
{
	float auto_use_vert_scanlines = screen_aspect < 1 ? 1 : 0;
	float out_scanline_direction = 0;
	
	if (HSM_SCANLINE_DIRECTION < 1)
		out_scanline_direction = screen_aspect < 1 ? 1 : 0;
	else if (HSM_SCANLINE_DIRECTION > 1.5)
		out_scanline_direction = 1;

	return out_scanline_direction;
}

vec2 HSM_GetScreenScaleWithEdgeHeight(vec2 edge_thickness, vec2 screen_scale)
// Edge thickness is a 0 to 1 percentage of the screen height
{
	float output_aspect_ratio = global.FinalViewportSize.x / global.FinalViewportSize.y;
	
	float aspect_ratio = screen_scale.x / screen_scale.y;
	
	vec2 edge_width_height_as_scale = vec2(0, 0);
	edge_width_height_as_scale.x = 1 + (edge_thickness.x / screen_scale.y) / aspect_ratio / output_aspect_ratio;
	edge_width_height_as_scale.y = 1 + edge_thickness.y / screen_scale.y;

	return screen_scale * edge_width_height_as_scale;
}

// Same as the HSM_GetScreenScale, but adds the width of the black edge
// Used for adding thickness around the screen like the black edge
vec2 HSM_GetTubeScale(vec2 screen_scale, float image_placement_screen_height, vec2 edge_thickness)
{
	// Add switch for independent scale
	if (HSM_BZL_USE_INDEPENDENT_SCALE == 1)
	{
		if (HSM_USE_IMAGE_FOR_PLACEMENT > 0.5)
			screen_scale = screen_scale / screen_scale.y * image_placement_screen_height * HSM_BZL_INDEPENDENT_SCALE / DEFAULT_SCREEN_HEIGHT;
		else
			screen_scale = screen_scale / screen_scale.y * HSM_BZL_INDEPENDENT_SCALE;
	}

	float normalized_screen_height = screen_scale.y / DEFAULT_UNCORRECTED_SCREEN_SCALE.y;
	return HSM_GetScreenScaleWithEdgeHeight(edge_thickness * vec2(1.2 / 100.0 * normalized_screen_height), screen_scale * HSM_BZL_SCALE_OFFSET);
}

// TODO need to get the screen scale before integer scale so we can derive automatic scanlines from it
// HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE
vec2 HSM_GetScreenScale(float screen_aspect, float screen_height_from_image, vec2 cropped_size)
{
	if (HSM_ASPECT_RATIO_MODE > 5.5)
	{
		return vec2(1, 1);
	}
	else
	{
		float output_aspect_ratio = global.FinalViewportSize.x / global.FinalViewportSize.y;

		bool viewport_is_vertical = (global.FinalViewportSize.x < global.FinalViewportSize.y);

		float vertical_preset_scale_mult = HSM_VERTICAL_PRESET > 0.5 || viewport_is_vertical ? DEFAULT_SCREEN_HEIGHT_PORTRAIT_MULTIPLIER : 1;
		float screen_height = HSM_NON_INTEGER_SCALE * vertical_preset_scale_mult;

		if (HSM_DUALSCREEN_MODE > 0)
			screen_height *= 0.5;

		if (HSM_INT_SCALE_MODE == 0)
		{
			if (HSM_USE_PHYSICAL_SIZE_FOR_NON_INTEGER == 1)
			{
				float monitor_aspect_with_rotation = HSM_VERTICAL_PRESET > 0.5 || viewport_is_vertical ? 1 / HSM_PHYSICAL_MONITOR_ASPECT_RATIO : HSM_PHYSICAL_MONITOR_ASPECT_RATIO;
				float monitor_height = HSM_PHYSICAL_MONITOR_DIAGONAL_SIZE / sqrt(monitor_aspect_with_rotation * monitor_aspect_with_rotation + 1);
				float sim_screen_height = HSM_PHYSICAL_SIM_TUBE_DIAGONAL_SIZE / sqrt(screen_aspect * screen_aspect + 1);

				screen_height = 0.012 + sim_screen_height / monitor_height;
			}

			if (HSM_USE_IMAGE_FOR_PLACEMENT == 1)
				screen_height = screen_height_from_image;

			screen_height *= HSM_NON_INTEGER_SCALE_OFFSET;

			// If the integer tolerance is greater than zero see if we can snap to the nearest integer multiple
			if (HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE > 0)
			{
				float integer_scale_multiple_vert = screen_height * global.FinalViewportSize.y / cropped_size.y;
				float int_scale_remainder = fract(integer_scale_multiple_vert);
				int_scale_remainder = (int_scale_remainder < 1 - int_scale_remainder) ? int_scale_remainder : 1 - int_scale_remainder;
				float remainder_percent_of_screen_height = (int_scale_remainder * cropped_size.y) / (screen_height * global.FinalViewportSize.y);
				if (remainder_percent_of_screen_height < HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE)
				{
					integer_scale_multiple_vert = round(integer_scale_multiple_vert);
					screen_height = integer_scale_multiple_vert * cropped_size.y / global.FinalViewportSize.y;
				}
			}

			return vec2(screen_aspect / output_aspect_ratio, 1) * screen_height;
		}

		// Get the maximum height that the integer scale needs to fit into
		float viewport_res_y_without_border = global.FinalViewportSize.y - (1 - HSM_INT_SCALE_MAX_HEIGHT) * global.FinalViewportSize.y;
		float viewport_res_x_without_border = global.FinalViewportSize.x - (1 - HSM_INT_SCALE_MAX_HEIGHT) * global.FinalViewportSize.x;

		if (HSM_DUALSCREEN_MODE == 1)
			viewport_res_y_without_border = global.FinalViewportSize.y / 2 - 0.5 * (1 - HSM_INT_SCALE_MAX_HEIGHT) * global.FinalViewportSize.y;

		if (HSM_DUALSCREEN_MODE == 2)
			viewport_res_x_without_border *= global.FinalViewportSize.x / 2 - 0.5 * (1 - HSM_INT_SCALE_MAX_HEIGHT) * global.FinalViewportSize.x;

		// If the viewport is taller than it is wide then get the height from the corresponding available width
		if (viewport_is_vertical) viewport_res_y_without_border = viewport_res_x_without_border / screen_aspect;

		// If the screen is too high
		if ((viewport_res_y_without_border * screen_aspect) > global.FinalViewportSize.x)
		{
			viewport_res_y_without_border = (1 - 2 * (1 - HSM_INT_SCALE_MAX_HEIGHT)) * global.FinalViewportSize.x / screen_aspect; 
		}

		float integer_scale_multiple_vert = clamp(floor(viewport_res_y_without_border / cropped_size.y) + HSM_INT_SCALE_MULTIPLE_OFFSET, 1, 100);
		float integer_scale_vert = integer_scale_multiple_vert * cropped_size.y / global.FinalViewportSize.y;

		// Get the horizontal scale from the vertical scale and aspect ratio
		float integer_scale_horz_from_aspect = screen_aspect / output_aspect_ratio * integer_scale_vert;

		// Get the scale as a multiple of the original x-size
		float integer_scale_multiple_horz = integer_scale_horz_from_aspect * global.FinalViewportSize.x / cropped_size.x;

		// If we are using vertical scanlines or integer scale is set to both directions make the horizontal multiple an integer
		float final_int_scale_mode = HSM_INT_SCALE_MODE;
		if (HSM_INT_SCALE_MODE > 0.5)
		{
			if (HSM_GetUseVerticalScanlines(screen_aspect) == 1 || HSM_INT_SCALE_MODE == 2)
			{
				integer_scale_multiple_horz = round(integer_scale_multiple_horz);
				final_int_scale_mode = 2;
			}
		}

		float both_axes = clamp((final_int_scale_mode - 1) * 10000, 0, 1);
		integer_scale_multiple_vert += both_axes * abs(clamp((screen_aspect - 1) * 10000, -1, 0)) * HSM_INT_SCALE_MULTIPLE_OFFSET_LONG;
		integer_scale_multiple_horz += both_axes * abs(clamp((screen_aspect - 1) * 10000,  0, 1)) * HSM_INT_SCALE_MULTIPLE_OFFSET_LONG;

		integer_scale_vert = integer_scale_multiple_vert * cropped_size.y / global.FinalViewportSize.y;
		float integer_scale_horz = integer_scale_multiple_horz * cropped_size.x / global.FinalViewportSize.x;

		return vec2(integer_scale_horz, integer_scale_vert);
	}
}

vec2 HSM_GetScreenScaleFor2ndScreen(vec2 screen_scale, float screen_aspect)
{
	vec2 out_screen_scale = screen_scale;
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;

	if (HSM_2ND_SCREEN_INDEPENDENT_SCALE == 1)
		out_screen_scale = DEFAULT_UNCORRECTED_SCREEN_SCALE.y * 0.5 * vec2(screen_aspect / output_aspect, 1);
	else
		out_screen_scale = screen_scale.y * vec2(screen_aspect / output_aspect, 1);

	out_screen_scale *= HSM_2ND_SCREEN_SCALE_OFFSET;

	return out_screen_scale;
}

float HSM_GetIsCorePreppedSizeVertical(float screen_index)
{
	vec2 rotated_original_size = HSM_GetRotatedScreenCorePreppedSize(screen_index);
	float aspect_ratio = rotated_original_size.x / rotated_original_size.y;
	return aspect_ratio < 1 ? 1 : 0;
}

vec2 HSM_GetResMult(float use_vert_scanlines)
{
	vec2 sampling_mult = vec2(HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR, HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR);
	
	if (use_vert_scanlines == 1 && HSM_ROTATE_CORE_IMAGE == 0 || use_vert_scanlines == 0 && abs(HSM_ROTATE_CORE_IMAGE) == 1)
		sampling_mult = sampling_mult.yx;

	return sampling_mult;
}

bool HSM_ResolutionIsEqual(vec2 in_res, vec2 match_res)
{
	return (in_res == match_res);
}

// if return_rough_aspect is on we only get a rough aspect which doesn't 
// require the cropped_size (which implicates the use_vertical_scanlines)
float HSM_GetScreenAspect(float screen_index, vec2 cropped_size)
{
	vec2 original_size = HSM_GetRotatedCoreOriginalSize();
	vec2 rotated_original_size = HSM_GetRotatedCoreOriginalSize();
	float core_aspect_ratio = rotated_original_size.x / rotated_original_size.y;
	float core_aspect_horizontal = (core_aspect_ratio < 1) ? 1 / core_aspect_ratio : core_aspect_ratio;
	float horizontal_aspect = 0;

	vec2 atari_lynx_res = 					vec2(160, 102);
	vec2 atari_2600_res = 					vec2(160, 228);
	vec2 atari_2600_crop_res = 				vec2(152, 228);
	vec2 nintendo_gameboy_advance_res = 	vec2(240, 160);
	vec2 nintendo_gameboy_res = 			vec2(160, 144);
	vec2 nintendo_ds_res = 					vec2(256, 192);
	vec2 nintendo_ds_top_bottom_res = 		vec2(256, 384);
	vec2 nintendo_ds_side_by_side_res = 	vec2(512, 192);
	vec2 nintendo_3ds_top_res = 			vec2(400, 240);
	vec2 nintendo_3ds_bottom_res = 			vec2(320, 240);
	vec2 nintendo_3ds_top_bottom_res = 		vec2(400, 480);
	vec2 nintendo_3ds_side_by_side_res = 	vec2(720, 240);
	vec2 sega_saturn_fmv_res = 				vec2(352, 480);
	vec2 sony_psp = 						vec2(480, 272);
	vec2 sony_ps_fmv_res = 					vec2(320, 480);
	vec2 sony_ps_fmv_res_2 = 				vec2(512, 480);

	// This should handle some PS games with weird resolutions like Tekken and Driver 
	float sony_ps_height = 448;
	
	if (HSM_ASPECT_RATIO_MODE == 0)
	{
		// If the vertical res is larger than 580 is is probably a modern square pixel resolution
		// 576 seems to be PAL vertical resolution used sometimes
 		if (original_size.y > 580) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(sony_psp, 						original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_gameboy_advance_res, 	original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_gameboy_res, 			original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_ds_res, 				original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_ds_top_bottom_res, 		original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_ds_side_by_side_res, 	original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_3ds_top_res, 			original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_3ds_bottom_res, 		original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_3ds_top_bottom_res, 	original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(nintendo_3ds_side_by_side_res, 	original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(atari_lynx_res, 					original_size)) horizontal_aspect = core_aspect_horizontal;
		else if (HSM_ResolutionIsEqual(atari_2600_res, 					original_size)) horizontal_aspect = 1.333;
		else if (HSM_ResolutionIsEqual(atari_2600_crop_res, 			original_size)) horizontal_aspect = 1.333;
		else if (HSM_ResolutionIsEqual(sony_ps_fmv_res, 				original_size)) horizontal_aspect = 1.333;
		else if (HSM_ResolutionIsEqual(sony_ps_fmv_res_2, 				original_size)) horizontal_aspect = 1.333;
		else if (original_size.y == sony_ps_height) 									horizontal_aspect = 1.333;
		// Fall back to the explicit ratio
		else horizontal_aspect = HSM_ASPECT_RATIO_EXPLICIT;
	}
	else 
		if (HSM_ASPECT_RATIO_MODE == 1) horizontal_aspect = HSM_ASPECT_RATIO_EXPLICIT;
		else if (HSM_ASPECT_RATIO_MODE == 2) horizontal_aspect = 1.3333;
		else if (HSM_ASPECT_RATIO_MODE == 3) horizontal_aspect = 1.5;
		else if (HSM_ASPECT_RATIO_MODE == 4) horizontal_aspect = 1.7777;
		else if (HSM_ASPECT_RATIO_MODE == 5) horizontal_aspect = cropped_size.x / cropped_size.y;
		else if (HSM_ASPECT_RATIO_MODE == 6) horizontal_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;
		else horizontal_aspect = 1.333;

	// Find what the vertical aspect would be, either the current horizontal_aspect (if it's already vertical)
	// Or changing the horizontal aspect to vertical by taking the reciprocal
	float vertical_aspect = 1 / horizontal_aspect;
	float final_orientation = HSM_ASPECT_RATIO_ORIENTATION;

	if (HSM_ASPECT_RATIO_ORIENTATION < 0.5)
	{
		// Catch for Atari 2600 - Stella Emulator which would otherwise show up as a vertical aspect ratio
		if (
				HSM_ResolutionIsEqual(atari_2600_res, original_size) || 
		   		HSM_ResolutionIsEqual(atari_2600_crop_res, original_size) ||
		   		HSM_ResolutionIsEqual(sega_saturn_fmv_res, original_size) ||
				HSM_ResolutionIsEqual(sony_ps_fmv_res, original_size) ||
				HSM_ResolutionIsEqual(sony_ps_fmv_res_2, original_size) ||
				original_size.y == sony_ps_height
		   ) 
		{
		   final_orientation = 1;
		}
		else
		{
			final_orientation = (HSM_GetIsCorePreppedSizeVertical(screen_index) > 0.5) ? 2 : 1;
		}
	}
	
	float final_aspect_ratio = (final_orientation < 1.5) ? horizontal_aspect : vertical_aspect;

	return final_aspect_ratio;
}

int HSM_IsCoordIn2DRange(vec2 in_coord, vec4 in_2d_range)
{
	return (in_coord.x > in_2d_range.x && 
			in_coord.y > in_2d_range.y && 
			in_coord.x < in_2d_range.z && 
			in_coord.y < in_2d_range.w ) ? 1 : 0;
}

// HSM_CROP_BLACK_ONLY
vec4 HSM_GetBlackOnlyCropInPixels(sampler2D in_sampler_2D, vec2 sample_start_pixel_coord, vec2 window_size, float num_samples, vec4 max_crop)
{
	// HSM_GetTexSampleFromSampleStartAndSize(sampler2D in_sampler, vec2 in_screen_coord, vec2 sample_start_pixel_coord, vec2 window_size)

	// Working except Bottom and Right edges jump back and forth and are not very accurate

	vec4 tex_sample = vec4(0);
	float brightness = 0;
	float test_crop = 0;

	float crop_left_px = max_crop.x;
	float crop_top_px = max_crop.y;
	float crop_right_px = max_crop.z;
	float crop_bottom_px = max_crop.w;

	// Should use as many samples as crop pixels
	float final_crop_left_px = crop_left_px;
	test_crop = crop_left_px;
	for (int i=0; i < crop_left_px; i++)
	{
		tex_sample = HSM_GetTexSampleFromSampleStartAndSize(in_sampler_2D, vec2((test_crop - 0.5) / window_size.x, 0.5), sample_start_pixel_coord, window_size);
		brightness = tex_sample.r + tex_sample.g + tex_sample.b;
		if (brightness > HSM_CROP_BLACK_THRESHOLD)
		{
			final_crop_left_px = min(final_crop_left_px, test_crop);
		}
		test_crop -= 1;
	}
	final_crop_left_px -= 1;

	float final_crop_top_px = crop_top_px;
	test_crop = crop_top_px;
	for (int i=0; i < crop_top_px; i++)
	{
		tex_sample = HSM_GetTexSampleFromSampleStartAndSize(in_sampler_2D, vec2(0.5, (test_crop - 0.5) / window_size.y), sample_start_pixel_coord, window_size);
		brightness = tex_sample.r + tex_sample.g + tex_sample.b;
		if (brightness > HSM_CROP_BLACK_THRESHOLD)
		{
			final_crop_top_px = test_crop;
		}
		test_crop -= 1;
	}
	final_crop_top_px -= 1;

	float final_crop_right_px = crop_right_px;
	test_crop = crop_right_px;
	for (int i=0; i < crop_right_px; i++)
	{
		tex_sample = HSM_GetTexSampleFromSampleStartAndSize(in_sampler_2D, vec2((window_size.x - test_crop + 0.5) / window_size.x, 0.5), sample_start_pixel_coord, window_size);
		brightness = tex_sample.r + tex_sample.g + tex_sample.b;
		if (brightness > HSM_CROP_BLACK_THRESHOLD)
		{
			final_crop_right_px = test_crop;
		}
		test_crop -= 1;
	}
	final_crop_right_px -= 2;

	float final_crop_bottom_px = crop_bottom_px;
	test_crop = crop_bottom_px;
	for (int i=0; i < crop_bottom_px; i++)
	{
		tex_sample = HSM_GetTexSampleFromSampleStartAndSize(in_sampler_2D, vec2(0.5, (window_size.y - test_crop + 0.5) / window_size.y), sample_start_pixel_coord, window_size);
		brightness = tex_sample.r + tex_sample.g + tex_sample.b;
		if (brightness > 0)
		{
			final_crop_bottom_px = test_crop;
		}
		test_crop -= 1;
	}
	final_crop_bottom_px -= 2;

	return clamp(vec4(final_crop_left_px, final_crop_top_px, final_crop_right_px, final_crop_bottom_px), 0, 512);
}

void HSM_GetCroppedRotatedSizeAndPixelSampleAreaStart(float screen_index, sampler2D original_pass, inout vec2 cropped_rotated_size, inout vec2 cropped_sample_area_start)
{
	screen_index = HSM_GetSwappedScreenIndex(screen_index);
	vec2 rotated_negative_crop_added_size = HSM_GetRotatedDerezedSize();

	float default_crop_left_px = floor(MAX_NEGATIVE_CROP * rotated_negative_crop_added_size.x);
	float default_crop_top_px = floor(MAX_NEGATIVE_CROP * rotated_negative_crop_added_size.y);
	float default_crop_right_px = floor(MAX_NEGATIVE_CROP * rotated_negative_crop_added_size.x);
	float default_crop_bottom_px = floor(MAX_NEGATIVE_CROP * rotated_negative_crop_added_size.y);

	float zoom_crop_left_px = floor(HSM_CROP_PERCENT_ZOOM * rotated_negative_crop_added_size.x);
	float zoom_crop_top_px = floor(HSM_CROP_PERCENT_ZOOM * rotated_negative_crop_added_size.y);
	float zoom_crop_right_px = floor(HSM_CROP_PERCENT_ZOOM * rotated_negative_crop_added_size.x);
	float zoom_crop_bottom_px = floor(HSM_CROP_PERCENT_ZOOM * rotated_negative_crop_added_size.y);

	float crop_left_px = floor(HSM_CROP_PERCENT_LEFT * rotated_negative_crop_added_size.x);
	float crop_top_px = floor(HSM_CROP_PERCENT_TOP * rotated_negative_crop_added_size.y);
	float crop_right_px = floor(HSM_CROP_PERCENT_RIGHT * rotated_negative_crop_added_size.x);
	float crop_bottom_px = floor(HSM_CROP_PERCENT_BOTTOM * rotated_negative_crop_added_size.y);

	float final_crop_left_px = 0;
	float final_crop_top_px = 0;
	float final_crop_right_px = 0;
	float final_crop_bottom_px = 0;

	if (HSM_DUALSCREEN_MODE > 0)
	{
		float zoom_crop = 0;
		if (screen_index == 2)
		{
			crop_left_px = floor(HSM_2ND_SCREEN_CROP_PERCENT_LEFT * rotated_negative_crop_added_size.x);
			crop_top_px = floor(HSM_2ND_SCREEN_CROP_PERCENT_TOP * rotated_negative_crop_added_size.y);
			crop_right_px = floor(HSM_2ND_SCREEN_CROP_PERCENT_RIGHT * rotated_negative_crop_added_size.x);
			crop_bottom_px = floor(HSM_2ND_SCREEN_CROP_PERCENT_BOTTOM * rotated_negative_crop_added_size.y);
			zoom_crop = HSM_2ND_SCREEN_CROP_PERCENT_ZOOM;

			if (HSM_GetCoreImageSplitDirection() == 1)
			{
				default_crop_top_px = floor(rotated_negative_crop_added_size.y * (0.5 - HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET));
				crop_top_px = floor(crop_top_px / 2);
				crop_bottom_px = floor(crop_bottom_px / 2);
			}
			else
			{
				default_crop_left_px = floor(rotated_negative_crop_added_size.x * (0.5 - HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET));
				crop_left_px = floor(crop_left_px / 2);
				crop_right_px = floor(crop_right_px / 2);
			}
		}
		else
		{
			zoom_crop = HSM_CROP_PERCENT_ZOOM;
			if (HSM_GetCoreImageSplitDirection() == 1)
			{
				default_crop_bottom_px = floor(rotated_negative_crop_added_size.y * (0.5 + HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET));
				crop_top_px = floor(crop_top_px / 2);
				crop_bottom_px = floor(crop_bottom_px / 2);
			}
			else
			{
				default_crop_right_px = floor(rotated_negative_crop_added_size.x * (0.5 + HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET));
				crop_left_px = floor(crop_left_px / 2);
				crop_right_px = floor(crop_right_px / 2);
			}	
		}

		vec2 base_cropped_size = rotated_negative_crop_added_size - vec2(final_crop_left_px + final_crop_right_px, final_crop_top_px + final_crop_bottom_px);

		if (HSM_GetCoreImageSplitDirection() == 1)
		{
			zoom_crop_top_px = floor(base_cropped_size.x * zoom_crop / 2);
		}
		else
		{
			zoom_crop_left_px = floor(base_cropped_size.y * zoom_crop / 2);
		}

		zoom_crop_right_px = zoom_crop_left_px;
		zoom_crop_bottom_px = zoom_crop_top_px;

		// TODO currently overwriting split screen
		final_crop_top_px += default_crop_top_px + crop_top_px + zoom_crop_top_px;
		final_crop_left_px += default_crop_left_px + crop_left_px + zoom_crop_left_px;
		final_crop_right_px += default_crop_right_px + crop_right_px + zoom_crop_right_px;
		final_crop_bottom_px += default_crop_bottom_px + crop_bottom_px + zoom_crop_bottom_px;
	}
	else
	{
		// No Cropping
		if (HSM_CROP_MODE == 0)
		{
			final_crop_left_px = default_crop_left_px;
			final_crop_top_px = default_crop_top_px;
			final_crop_right_px = default_crop_right_px;
			final_crop_bottom_px = default_crop_bottom_px;
		}
		// Crop Only Black
		else if (HSM_CROP_MODE == 1)
		{
			// vec4 max_crop = vec4(crop_left_px + zoom_crop_left_px, crop_top_px + zoom_crop_top_px, crop_right_px + zoom_crop_right_px, crop_bottom_px + zoom_crop_bottom_px);
			vec4 max_crop = vec4(default_crop_left_px + crop_left_px + zoom_crop_left_px, default_crop_top_px + crop_top_px + zoom_crop_top_px, default_crop_right_px + crop_right_px + zoom_crop_right_px, default_crop_bottom_px + crop_bottom_px + zoom_crop_bottom_px);

			// Start and window where we are going to test for the black area
			// vec2 black_sample_start_coord_px = vec2(default_crop_left_px, default_crop_top_px);
			// vec2 black_sample_window = rotated_negative_crop_added_size - vec2(default_crop_left_px + default_crop_right_px, default_crop_top_px + default_crop_bottom_px);

			vec2 black_sample_start_coord_px = vec2(0);
			vec2 black_sample_window = rotated_negative_crop_added_size;

			vec4 cropping = HSM_GetBlackOnlyCropInPixels(original_pass, black_sample_start_coord_px, black_sample_window, 100, max_crop);

			final_crop_left_px = cropping.x;
			final_crop_top_px = cropping.y;
			final_crop_right_px = cropping.z;
			final_crop_bottom_px = cropping.w;
		}
		else
		{
			final_crop_top_px = default_crop_top_px + crop_top_px + zoom_crop_top_px;
			final_crop_left_px = default_crop_left_px + crop_left_px + zoom_crop_left_px;
			final_crop_bottom_px = default_crop_bottom_px + crop_bottom_px + zoom_crop_bottom_px;
			final_crop_right_px = default_crop_right_px + crop_right_px + zoom_crop_right_px;
		}

	}
 
	// Make sure we don't actually negatively crop
	final_crop_top_px = clamp(final_crop_top_px, 0, 20000);
	final_crop_bottom_px = clamp(final_crop_bottom_px, 0, 20000);
	final_crop_left_px = clamp(final_crop_left_px, 0, 20000);
	final_crop_right_px = clamp(final_crop_right_px, 0, 20000);

	cropped_rotated_size = rotated_negative_crop_added_size - vec2(final_crop_left_px + final_crop_right_px, final_crop_top_px + final_crop_bottom_px);
	cropped_sample_area_start = vec2(final_crop_left_px, final_crop_top_px);
}

vec2 HSM_GetCroppedRotatedSizeWithResMult(vec2 cropped_rotated_size, vec2 sampling_mult)
{
	sampling_mult = abs(HSM_ROTATE_CORE_IMAGE) == 1 ? sampling_mult.yx : sampling_mult.xy;
	return ceil(cropped_rotated_size * sampling_mult.xy);
}

float HSM_GetParameterSum()
{
	float out_sum = (0
					+ global.FinalViewportSize.x
					+ global.FinalViewportSize.y

					+ HSM_GLOBAL_GRAPHICS_BRIGHTNESS * 100
					+ HSM_STATIC_LAYERS_GAMMA

					+ HSM_SINDEN_BORDER_ON
					+ HSM_SINDEN_BORDER_THICKNESS
					+ HSM_SINDEN_BORDER_EMPTY_TUBE_COMPENSATION

					+ HSM_CACHE_GRAPHICS_ON
					
					// Night Lighting
					+ HSM_AMBIENT_LIGHTING_OPACITY * 100
					+ HSM_AMBIENT1_OPACITY * 100
					+ HSM_AMBIENT2_OPACITY * 100
					+ HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE

					//    Zoom & Pan
					+ HSM_VIEWPORT_ZOOM * 100
					+ HSM_VIEWPORT_POSITION_X * 1000
					+ HSM_VIEWPORT_POSITION_Y * 1000

					//    FLIP & ROTATE
					+ HSM_FLIP_VIEWPORT_VERTICAL
					+ HSM_FLIP_VIEWPORT_HORIZONTAL
					// + HSM_FLIP_CORE_VERTICAL
					// + HSM_FLIP_CORE_HORIZONTAL
					+ HSM_ROTATE_CORE_IMAGE

					//    ASPECT RATIO
					+ HSM_ASPECT_RATIO_ORIENTATION
					+ HSM_ASPECT_RATIO_MODE
					+ HSM_ASPECT_RATIO_EXPLICIT

					//    SCALING
					+ HSM_INT_SCALE_MODE
					+ HSM_VERTICAL_PRESET

					// Integer Scale
					+ HSM_INT_SCALE_MAX_HEIGHT
					+ HSM_INT_SCALE_MULTIPLE_OFFSET
					+ HSM_INT_SCALE_MULTIPLE_OFFSET_LONG

					// Non Integer Scale
					+ HSM_NON_INTEGER_SCALE

					+ HSM_USE_PHYSICAL_SIZE_FOR_NON_INTEGER
					+ HSM_PHYSICAL_MONITOR_ASPECT_RATIO
					+ HSM_PHYSICAL_MONITOR_DIAGONAL_SIZE
					+ HSM_PHYSICAL_SIM_TUBE_DIAGONAL_SIZE

					//    Extended Scale
					+ HSM_USE_IMAGE_FOR_PLACEMENT
					+ HSM_PLACEMENT_IMAGE_USE_HORIZONTAL
					+ HSM_PLACEMENT_IMAGE_MODE

					// Non Integer Scale Offset
					+ HSM_NON_INTEGER_SCALE_OFFSET * 100

					// Snap to Integer Scale
					+ HSM_USE_SNAP_TO_CLOSEST_INT_SCALE
					+ HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE

					//    Position
					+ HSM_SCREEN_POSITION_X * 1000
					+ HSM_SCREEN_POSITION_Y * 1000

					//    CROPPING
					+ HSM_CROP_MODE
					+ HSM_CROP_PERCENT_ZOOM * 100
					+ HSM_CROP_PERCENT_TOP * 100
					+ HSM_CROP_PERCENT_BOTTOM * 100
					+ HSM_CROP_PERCENT_LEFT * 100
					+ HSM_CROP_PERCENT_RIGHT * 100
					+ HSM_CROP_BLACK_THRESHOLD * 100

					// + HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR
					// + HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR

					//    CURVATURE
					+ HSM_CURVATURE_MODE
					+ HSM_CURVATURE_2D_SCALE_LONG_AXIS * 100
					+ HSM_CURVATURE_2D_SCALE_SHORT_AXIS * 100
					+ HSM_CURVATURE_3D_RADIUS * 100
					+ HSM_CURVATURE_3D_VIEW_DIST * 100
					+ HSM_CURVATURE_3D_TILT_ANGLE_X * 100
					+ HSM_CURVATURE_3D_TILT_ANGLE_Y * 100

					// + HSM_AB_COMPARE_SHOW_MODE
					+ HSM_AB_COMPARE_AREA
					+ HSM_AB_COMPARE_SPLIT_POSITION
					+ HSM_AB_COMPARE_FREEZE_GRAPHICS

					// //    TUBE DIFFUSE
					// + HSM_TUBE_OPACITY
					// + HSM_TUBE_DIFFUSE_MODE
					// + HSM_TUBE_DIFFUSE_IMAGE_DUALSCREEN_VIS_MODE
					// + HSM_TUBE_DIFFUSE_IMAGE_COLORIZE_ON
					// + HSM_TUBE_DIFFUSE_IMAGE_HUE
					// + HSM_TUBE_DIFFUSE_IMAGE_SATURATION
					// + HSM_TUBE_DIFFUSE_IMAGE_BRIGHTNESS
					// + HSM_TUBE_DIFFUSE_IMAGE_GAMMA
					+ HSM_TUBE_DIFFUSE_FORCE_ASPECT
					+ HSM_TUBE_EXPLICIT_ASPECT
					+ HSM_TUBE_EMPTY_THICKNESS
					+ HSM_TUBE_EMPTY_THICKNESS_X_SCALE

					// SCREEN BLACK EDGE
					+ HSM_GLOBAL_CORNER_RADIUS
					+ HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE * 100
					+ HSM_TUBE_BLACK_EDGE_SHARPNESS * 100
					+ HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE * 100
					+ HSM_TUBE_BLACK_EDGE_THICKNESS * 100
					+ HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE * 100

					// //    DUAL SCREEN
					+ HSM_DUALSCREEN_MODE
					+ HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE
					+ HSM_DUALSCREEN_CORE_IMAGE_SWAP_SCREENS
					+ HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET * 1000
					+ HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION * 1000
					+ HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE
					+ HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS * 1000
					+ HSM_2ND_SCREEN_ASPECT_RATIO_MODE
					+ HSM_2ND_SCREEN_INDEPENDENT_SCALE
					+ HSM_2ND_SCREEN_SCALE_OFFSET * 100
					+ HSM_2ND_SCREEN_POS_X * 1000
					+ HSM_2ND_SCREEN_POS_Y * 1000
					+ HSM_2ND_SCREEN_CROP_PERCENT_ZOOM * 100
					+ HSM_2ND_SCREEN_CROP_PERCENT_TOP * 100
					+ HSM_2ND_SCREEN_CROP_PERCENT_BOTTOM * 100
					+ HSM_2ND_SCREEN_CROP_PERCENT_LEFT * 100
					+ HSM_2ND_SCREEN_CROP_PERCENT_RIGHT * 100

					// AMBIENT LIGHTING 1
					+ HSM_AMBIENT1_HUE * 360
					+ HSM_AMBIENT1_SATURATION * 100
					+ HSM_AMBIENT1_VALUE * 100
					+ HSM_AMBIENT1_CONTRAST
					+ HSM_AMBIENT1_SCALE_INHERIT_MODE
					+ HSM_AMBIENT1_SCALE * 100
					+ HSM_AMBIENT1_SCALE_X * 100
					+ HSM_AMBIENT1_ROTATE
					+ HSM_AMBIENT1_MIRROR_HORZ
					+ HSM_AMBIENT1_POS_INHERIT_MODE
					+ HSM_AMBIENT1_POSITION_X * 1000
					+ HSM_AMBIENT1_POSITION_Y * 1000
					+ HSM_AMBIENT1_DITHERING_SAMPLES

					// AMBIENT LIGHTING 2
					+ HSM_AMBIENT2_HUE * 360
					+ HSM_AMBIENT2_SATURATION * 100
					+ HSM_AMBIENT2_VALUE * 100
					+ HSM_AMBIENT2_CONTRAST
					+ HSM_AMBIENT2_SCALE_INHERIT_MODE
					+ HSM_AMBIENT2_SCALE * 100
					+ HSM_AMBIENT2_SCALE_X * 100
					+ HSM_AMBIENT2_ROTATE
					+ HSM_AMBIENT2_MIRROR_HORZ
					+ HSM_AMBIENT2_POS_INHERIT_MODE
					+ HSM_AMBIENT2_POSITION_X * 1000
					+ HSM_AMBIENT2_POSITION_Y * 1000

					//    BEZEL INDEPENDENT SCALE
					+ HSM_BZL_USE_INDEPENDENT_SCALE
					+ HSM_BZL_INDEPENDENT_SCALE * 100
					+ HSM_BZL_USE_INDEPENDENT_CURVATURE
					+ HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS * 100
					+ HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS * 100

					//    BEZEL GENERAL
					+ HSM_BZL_OPACITY * 100
					+ HSM_BZL_BLEND_MODE
					+ HSM_BZL_WIDTH /  0.0008624
					+ HSM_BZL_HEIGHT / 0.0008732
					+ HSM_BZL_SCALE_OFFSET * 100
					+ HSM_BZL_INNER_CURVATURE_SCALE * 100
					+ HSM_BZL_INNER_CORNER_RADIUS_SCALE * 100

#ifdef HAS_BEZEL_PARAMS

					// Reflection Image Mask
					+ HSM_REFLECT_MASK_IMAGE_AMOUNT
					+ HSM_REFLECT_MASK_FOLLOW_LAYER
					+ HSM_REFLECT_MASK_FOLLOW_MODE
					+ HSM_REFLECT_MASK_BRIGHTNESS * 100
					+ HSM_REFLECT_MASK_BLACK_LEVEL

					//	Bezel Params not in Screen Scale
					+ HSM_BZL_INNER_EDGE_THICKNESS / 0.00007
					+ HSM_BZL_INNER_EDGE_SHARPNESS * 100
					+ HSM_BZL_OUTER_POSITION_Y * 2000
					+ HSM_BZL_OUTER_CURVATURE_SCALE * 100
					+ HSM_BZL_OUTER_CORNER_RADIUS_SCALE * 100
					+ HSM_BZL_BRIGHTNESS * 100
					+ HSM_BZL_BRIGHTNESS_MULT_TOP * 100
					+ HSM_BZL_BRIGHTNESS_MULT_BOTTOM * 100
					+ HSM_BZL_BRIGHTNESS_MULT_SIDES * 100
					+ HSM_BZL_BRIGHTNESS_MULT_SIDE_LEFT * 100
					+ HSM_BZL_BRIGHTNESS_MULT_SIDE_RIGHT * 100
					+ HSM_BZL_HIGHLIGHT * 100
					+ HSM_BZL_INNER_EDGE_HIGHLIGHT * 100
					+ HSM_BZL_NOISE * 100
					+ HSM_BZL_INNER_EDGE_SHADOW * 100

					// Bezel Color
					+ HSM_BZL_COLOR_HUE * 360
					+ HSM_BZL_COLOR_SATURATION * 100
					+ HSM_BZL_COLOR_VALUE * 100
					+ HSM_BZL_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_BZL_AMBIENT2_LIGHTING_MULTIPLIER * 100

					// Frame Color
					+ HSM_FRM_USE_INDEPENDENT_COLOR
					+ HSM_FRM_COLOR_HUE * 360
					+ HSM_FRM_COLOR_SATURATION * 100
					+ HSM_FRM_COLOR_VALUE * 100

					// Generated Frame
					+ HSM_FRM_OPACITY * 100
					+ HSM_FRM_BLEND_MODE
					+ HSM_FRM_TEXTURE_OPACITY * 100
					+ HSM_FRM_TEXTURE_BLEND_MODE
					+ HSM_FRM_NOISE * 100
					+ HSM_FRM_INNER_EDGE_THICKNESS / 0.00003
					+ HSM_FRM_INNER_EDGE_HIGHLIGHT * 1000
					+ HSM_FRM_THICKNESS / 0.0007
					+ HSM_FRM_THICKNESS_SCALE_X * 100
					+ HSM_FRM_OUTER_POS_Y * 100
					+ HSM_FRM_OUTER_CURVATURE_SCALE * 100
					+ HSM_FRM_OUTER_CORNER_RADIUS
					+ HSM_FRM_OUTER_EDGE_THICKNESS / 0.00006
					+ HSM_FRM_OUTER_EDGE_SHADING * 100
					+ HSM_FRM_SHADOW_OPACITY * 100
					+ HSM_FRM_SHADOW_WIDTH * 1000

					// Corner
					+ HSM_REFLECT_CORNER_FADE * 100
					+ HSM_REFLECT_CORNER_FADE_DISTANCE * 100
					+ HSM_REFLECT_CORNER_INNER_SPREAD * 100
					+ HSM_REFLECT_CORNER_OUTER_SPREAD * 100
					+ HSM_REFLECT_CORNER_ROTATION_OFFSET_TOP
					+ HSM_REFLECT_CORNER_ROTATION_OFFSET_BOTTOM
					+ HSM_REFLECT_CORNER_SPREAD_FALLOFF
#endif

#ifdef HAS_IMAGE_LAYER_PARAMS

					// Layer Order
					+ HSM_BG_LAYER_ORDER
					+ HSM_VIEWPORT_VIGNETTE_LAYER_ORDER
					+ HSM_CRT_LAYER_ORDER
					+ HSM_DEVICE_LAYER_ORDER
					+ HSM_DEVICELED_LAYER_ORDER
					+ HSM_CAB_GLASS_LAYER_ORDER
					+ HSM_DECAL_LAYER_ORDER
					+ HSM_LED_LAYER_ORDER
					+ HSM_TOP_LAYER_ORDER

					// Background
					+ HSM_BG_OPACITY
					+ HSM_BG_COLORIZE_ON
					+ HSM_BG_HUE * 360
					+ HSM_BG_SATURATION * 100
					+ HSM_BG_BRIGHTNESS * 100
					+ HSM_BG_GAMMA
					+ HSM_BG_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_BG_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_BG_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_BG_BLEND_MODE
					+ HSM_BG_SOURCE_MATTE_TYPE
					+ HSM_BG_MASK_MODE
					+ HSM_BG_CUTOUT_MODE
					+ HSM_BG_DUALSCREEN_VIS_MODE
					+ HSM_BG_FOLLOW_LAYER
					+ HSM_BG_FOLLOW_MODE
					+ HSM_BG_FOLLOW_FULL_USES_ZOOM
					+ HSM_BG_FILL_MODE
					+ HSM_BG_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_BG_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_BG_SCALE * 100
					+ HSM_BG_SCALE_X * 100
					+ HSM_BG_POS_X * 100
					+ HSM_BG_POS_Y * 100
					+ HSM_BG_WRAP_MODE
					+ HSM_BG_MIPMAPPING_BLEND_BIAS

					// Background Vignette
					+ HSM_VIEWPORT_VIGNETTE_OPACITY * 100
					+ HSM_VIEWPORT_VIGNETTE_MASK_MODE
					+ HSM_VIEWPORT_VIGNETTE_CUTOUT_MODE
					+ HSM_VIEWPORT_VIGNETTE_FOLLOW_LAYER
					+ HSM_VIEWPORT_VIGNETTE_SCALE * 100
					+ HSM_VIEWPORT_VIGNETTE_SCALE_X * 100
					+ HSM_VIEWPORT_VIGNETTE_POS_X * 100
					+ HSM_VIEWPORT_VIGNETTE_POS_Y * 100

					// Cutout
					+ HSM_CUTOUT_ASPECT_MODE
					+ HSM_CUTOUT_EXPLICIT_ASPECT
					+ HSM_CUTOUT_FOLLOW_LAYER
					+ HSM_CUTOUT_FOLLOW_FULL_USES_ZOOM
					+ HSM_CUTOUT_SCALE * 100
					+ HSM_CUTOUT_SCALE_X * 100
					+ HSM_CUTOUT_POS_X * 400
					+ HSM_CUTOUT_POS_Y * 400
					+ HSM_CUTOUT_CORNER_RADIUS

					// LED
					+ HSM_LED_OPACITY * 100
					+ HSM_LED_COLORIZE_ON
					+ HSM_LED_HUE * 360
					+ HSM_LED_SATURATION * 100
					+ HSM_LED_BRIGHTNESS * 100
					+ HSM_LED_GAMMA
					+ HSM_LED_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_LED_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_LED_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_LED_BLEND_MODE
					+ HSM_LED_SOURCE_MATTE_TYPE
					+ HSM_LED_MASK_MODE
					+ HSM_LED_CUTOUT_MODE
					+ HSM_LED_DUALSCREEN_VIS_MODE
					+ HSM_LED_FOLLOW_LAYER
					+ HSM_LED_FOLLOW_MODE
					+ HSM_LED_FOLLOW_FULL_USES_ZOOM
					+ HSM_LED_FILL_MODE
					+ HSM_LED_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_LED_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_LED_SCALE * 100
					+ HSM_LED_SCALE_X * 100
					+ HSM_LED_POS_X * 100
					+ HSM_LED_POS_Y * 100
					+ HSM_LED_MIPMAPPING_BLEND_BIAS

					// Device
					+ HSM_DEVICE_OPACITY * 100
					+ HSM_DEVICE_COLORIZE_ON
					+ HSM_DEVICE_HUE * 360
					+ HSM_DEVICE_SATURATION * 100
					+ HSM_DEVICE_BRIGHTNESS * 100
					+ HSM_DEVICE_GAMMA
					+ HSM_DEVICE_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_DEVICE_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_DEVICE_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_DEVICE_BLEND_MODE
					+ HSM_DEVICE_SOURCE_MATTE_TYPE
					+ HSM_DEVICE_MASK_MODE
					+ HSM_DEVICE_CUTOUT_MODE
					+ HSM_DEVICE_DUALSCREEN_VIS_MODE
					+ HSM_DEVICE_FOLLOW_LAYER
					+ HSM_DEVICE_FOLLOW_MODE
					+ HSM_DEVICE_FOLLOW_FULL_USES_ZOOM
					+ HSM_DEVICE_FILL_MODE
					+ HSM_DEVICE_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_DEVICE_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_DEVICE_SCALE * 100
					+ HSM_DEVICE_SCALE_X * 100
					+ HSM_DEVICE_POS_X * 100
					+ HSM_DEVICE_POS_Y * 100
					+ HSM_DEVICE_MIPMAPPING_BLEND_BIAS

					// Device LED
					+ HSM_DEVICELED_OPACITY * 100
					+ HSM_DEVICELED_COLORIZE_ON
					+ HSM_DEVICELED_HUE * 360
					+ HSM_DEVICELED_SATURATION * 100
					+ HSM_DEVICELED_BRIGHTNESS * 100
					+ HSM_DEVICELED_GAMMA
					+ HSM_DEVICELED_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_DEVICELED_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_DEVICELED_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_DEVICELED_BLEND_MODE
					+ HSM_DEVICELED_SOURCE_MATTE_TYPE
					+ HSM_DEVICELED_MASK_MODE
					+ HSM_DEVICELED_CUTOUT_MODE
					+ HSM_DEVICELED_DUALSCREEN_VIS_MODE
					+ HSM_DEVICELED_FOLLOW_LAYER
					+ HSM_DEVICELED_FOLLOW_MODE
					+ HSM_DEVICELED_FOLLOW_FULL_USES_ZOOM
					+ HSM_DEVICELED_FILL_MODE
					+ HSM_DEVICELED_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_DEVICELED_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_DEVICELED_SCALE * 100
					+ HSM_DEVICELED_SCALE_X * 100
					+ HSM_DEVICELED_POS_X * 100
					+ HSM_DEVICELED_POS_Y * 100
					+ HSM_DEVICELED_MIPMAPPING_BLEND_BIAS

					// Decal
					+ HSM_DECAL_OPACITY * 100
					+ HSM_DECAL_COLORIZE_ON
					+ HSM_DECAL_HUE * 360
					+ HSM_DECAL_SATURATION * 100
					+ HSM_DECAL_BRIGHTNESS * 100
					+ HSM_DECAL_GAMMA
					+ HSM_DECAL_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_DECAL_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_DECAL_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_DECAL_BLEND_MODE
					+ HSM_DECAL_SOURCE_MATTE_TYPE
					+ HSM_DECAL_MASK_MODE
					+ HSM_DECAL_CUTOUT_MODE
					+ HSM_DECAL_DUALSCREEN_VIS_MODE
					+ HSM_DECAL_FOLLOW_LAYER
					+ HSM_DECAL_FOLLOW_MODE
					+ HSM_DECAL_FOLLOW_FULL_USES_ZOOM
					+ HSM_DECAL_FILL_MODE
					+ HSM_DECAL_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_DECAL_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_DECAL_SCALE * 100
					+ HSM_DECAL_SCALE_X * 100
					+ HSM_DECAL_POS_X * 100
					+ HSM_DECAL_POS_Y * 100
					+ HSM_DECAL_MIPMAPPING_BLEND_BIAS

					// // Cab Glass
					+ HSM_CAB_GLASS_OPACITY * 100
					+ HSM_CAB_GLASS_COLORIZE_ON
					+ HSM_CAB_GLASS_HUE * 360
					+ HSM_CAB_GLASS_SATURATION * 100
					+ HSM_CAB_GLASS_BRIGHTNESS * 100
					+ HSM_CAB_GLASS_GAMMA
					+ HSM_CAB_GLASS_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_CAB_GLASS_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_CAB_GLASS_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_CAB_GLASS_BLEND_MODE
					+ HSM_CAB_GLASS_SOURCE_MATTE_TYPE
					+ HSM_CAB_GLASS_MASK_MODE
					+ HSM_CAB_GLASS_CUTOUT_MODE
					+ HSM_CAB_GLASS_DUALSCREEN_VIS_MODE
					+ HSM_CAB_GLASS_FOLLOW_LAYER
					+ HSM_CAB_GLASS_FOLLOW_MODE
					+ HSM_CAB_GLASS_FOLLOW_FULL_USES_ZOOM
					+ HSM_CAB_GLASS_FILL_MODE
					+ HSM_CAB_GLASS_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_CAB_GLASS_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_CAB_GLASS_SCALE * 100
					+ HSM_CAB_GLASS_SCALE_X * 100
					+ HSM_CAB_GLASS_POS_X * 100
					+ HSM_CAB_GLASS_POS_Y * 100
					+ HSM_CAB_GLASS_MIPMAPPING_BLEND_BIAS

					// Top Image
					+ HSM_TOP_OPACITY * 100
					+ HSM_TOP_COLORIZE_ON
					+ HSM_TOP_HUE * 360
					+ HSM_TOP_SATURATION * 100
					+ HSM_TOP_BRIGHTNESS * 100
					+ HSM_TOP_GAMMA
					+ HSM_TOP_AMBIENT_LIGHTING_MULTIPLIER * 100
					+ HSM_TOP_AMBIENT2_LIGHTING_MULTIPLIER * 100
					+ HSM_TOP_APPLY_AMBIENT_IN_ADD_MODE
					+ HSM_TOP_BLEND_MODE
					+ HSM_TOP_SOURCE_MATTE_TYPE
					+ HSM_TOP_MASK_MODE
					+ HSM_TOP_CUTOUT_MODE
					+ HSM_TOP_DUALSCREEN_VIS_MODE
					+ HSM_TOP_FOLLOW_LAYER
					+ HSM_TOP_FOLLOW_MODE
					+ HSM_TOP_FOLLOW_FULL_USES_ZOOM
					+ HSM_TOP_FILL_MODE
					+ HSM_TOP_SPLIT_PRESERVE_CENTER * 1000
					+ HSM_TOP_SPLIT_REPEAT_WIDTH * 1000
					+ HSM_TOP_SCALE * 100
					+ HSM_TOP_SCALE_X * 100
					+ HSM_TOP_POS_X * 100
					+ HSM_TOP_POS_Y * 100
					+ HSM_TOP_MIRROR_WRAP
					+ HSM_TOP_MIPMAPPING_BLEND_BIAS

					+ HSM_RENDER_SIMPLE_MODE
					+ HSM_LAYERING_DEBUG_MASK_MODE
#endif

					);
	return out_sum;
}

vec2 GetScreenSizeWithZoom(vec2 screen_scale)
{
	vec2 screen_size = global.FinalViewportSize.xy * screen_scale * HSM_VIEWPORT_ZOOM;
	// Round this so we get proper integers

	// TODO try + eps and floor
	screen_size = vec2(ceil(screen_size.x), ceil(screen_size.y));
	return screen_size;
}

vec4 HSM_GetColorForScreenInfoCache(vec2 viewport_coord, sampler2D feedback_pass, sampler2D original_pass, sampler2D screen_placement_image)
{
	NEGATIVE_CROP_EXPAND_MULTIPLIER = global.DerezedPassSize.y / global.DerezedPassSize.y;
	MAX_NEGATIVE_CROP = ((1 - (1 / NEGATIVE_CROP_EXPAND_MULTIPLIER)) / 2);

	vec4 out_color = vec4(0);
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;
	vec2 rotated_derezed_size = HSM_GetRotatedScreenDerezedSize();
	vec2 cropped_rotated_size = vec2(100);
	vec2 cropped_sample_area_start_pixel_coord = vec2(100);
	HSM_GetCroppedRotatedSizeAndPixelSampleAreaStart(1, original_pass, cropped_rotated_size, cropped_sample_area_start_pixel_coord);
	// vec2 cropped_unrotated_size = abs(HSM_ROTATE_CORE_IMAGE) > 0.5 ? cropped_rotated_size.yx : cropped_rotated_size.xy;

	float screen_aspect = HSM_GetScreenAspect(1, cropped_rotated_size);
	float use_vertical_scanlines = HSM_GetUseVerticalScanlines(screen_aspect);
	vec2 sampling_dir_mult = HSM_GetResMult(use_vertical_scanlines);
	vec2 cropped_rotated_size_with_res_mult = HSM_GetCroppedRotatedSizeWithResMult(cropped_rotated_size, sampling_dir_mult);
	// vec2 cropped_unrotated_size_with_res_mult = abs(HSM_ROTATE_CORE_IMAGE) > 0.5 ? cropped_rotated_size_with_res_mult.yx : cropped_rotated_size_with_res_mult.xy;

	// First Screen
	vec3 screen_pos_and_height = HSM_GetScreenPlacementAndHeight(screen_placement_image, 60);
	vec2 screen_scale = HSM_GetScreenScale(screen_aspect, screen_pos_and_height.z, cropped_rotated_size);
	vec2 screen_size = GetScreenSizeWithZoom(screen_scale);

	// Adjust tube empty thickness for Sinden lightgun bright edge border if needed
	float tube_sinden_border_thickness =  HSM_SINDEN_BORDER_ON > 0.5 && HSM_SINDEN_BORDER_EMPTY_TUBE_COMPENSATION > 0.5 ? 0.6 * HSM_SINDEN_BORDER_THICKNESS * 60 : 0;

	vec2 tube_diffuse_scale = HSM_GetTubeScale(screen_scale, screen_pos_and_height.z, vec2(HSM_TUBE_EMPTY_THICKNESS * HSM_TUBE_EMPTY_THICKNESS_X_SCALE + tube_sinden_border_thickness, HSM_TUBE_EMPTY_THICKNESS + screen_aspect * tube_sinden_border_thickness));	
	float tube_diffuse_aspect = tube_diffuse_scale.x / tube_diffuse_scale.y * output_aspect;

	// TODO need to adjust height in screen_pos_and_height with the tube extra thickness

	if (HSM_TUBE_DIFFUSE_FORCE_ASPECT > 0)
	{
		if ( HSM_TUBE_DIFFUSE_FORCE_ASPECT == 1)
			tube_diffuse_aspect = screen_aspect;
		if ( HSM_TUBE_DIFFUSE_FORCE_ASPECT == 2)
			tube_diffuse_aspect = screen_aspect > 1 ? HSM_TUBE_EXPLICIT_ASPECT : 1.0 / HSM_TUBE_EXPLICIT_ASPECT;

		tube_diffuse_scale = vec2( tube_diffuse_scale.y * tube_diffuse_aspect / output_aspect, tube_diffuse_scale.y );
	}

	vec2 tube_scale = HSM_GetTubeScale(tube_diffuse_scale, screen_pos_and_height.z, vec2(HSM_TUBE_BLACK_EDGE_THICKNESS * HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE, HSM_TUBE_BLACK_EDGE_THICKNESS));	

	vec2 pos_offset = HSM_GetScreenPositionOffset(screen_pos_and_height.xy, screen_scale, 1);

	vec2 rotated_core_preppezd_size = HSM_GetRotatedScreenCorePreppedSize(1);

	vec2 cropped_size_2nd_screen = vec2(100);
	vec2 sample_area_start_pixel_coord_2nd_screen = vec2(100);
	HSM_GetCroppedRotatedSizeAndPixelSampleAreaStart(2, original_pass, cropped_size_2nd_screen, sample_area_start_pixel_coord_2nd_screen);
	float screen_aspect_2nd_screen = HSM_2ND_SCREEN_ASPECT_RATIO_MODE == 1 ? cropped_size_2nd_screen.x/cropped_size_2nd_screen.y : screen_aspect;
	vec2 screen_scale_2nd_screen = HSM_GetScreenScaleFor2ndScreen(screen_scale, screen_aspect_2nd_screen);
	vec2 screen_size_2nd_screen = GetScreenSizeWithZoom(screen_scale_2nd_screen);
	vec2 cropped_size_with_res_mult_2nd_screen = HSM_GetCroppedRotatedSizeWithResMult(cropped_size_2nd_screen, sampling_dir_mult);

	vec2 tube_diffuse_scale_2nd_screen = 	HSM_GetTubeScale(screen_scale_2nd_screen, 		DEFAULT_UNCORRECTED_SCREEN_SCALE.y, vec2(HSM_TUBE_EMPTY_THICKNESS * HSM_TUBE_EMPTY_THICKNESS_X_SCALE, 			HSM_TUBE_EMPTY_THICKNESS));	
	vec2 tube_scale_2nd_screen = 			HSM_GetTubeScale(tube_diffuse_scale_2nd_screen, DEFAULT_UNCORRECTED_SCREEN_SCALE.y, vec2(HSM_TUBE_BLACK_EDGE_THICKNESS * HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE, HSM_TUBE_BLACK_EDGE_THICKNESS));	
	float tube_aspect_2nd_screen = tube_scale_2nd_screen.x / tube_scale_2nd_screen.y * output_aspect;

	// TODO need to add tube diffuse aspect to cache data

	vec2 pos_offset_2nd_screen = HSM_GetScreenPositionOffset(vec2(0.5, 0.5), screen_scale_2nd_screen, 2);

	vec4 sample_2d_range = vec4(0);
	float parameter_sum = HSM_GetParameterSum();

	// Sample 1, 1
	// r AVERAGE_LUMA
	sample_2d_range = HSM_GetCacheSampleRange(1, 1);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = sampling_dir_mult;
		out_color.a = HSM_GetAverageLuma(original_pass, global.DerezedPassSize.xy);
	}

	// Sample 2, 1
	// r SCREEN_ASPECT
	// ba SCREEN_SCALE
	sample_2d_range = HSM_GetCacheSampleRange(2, 1);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{ 
		out_color.r = screen_aspect;
		out_color.ba = screen_scale;
	}

	// Sample 3, 1
	// rg TUBE_SCALE 
	// ba SCREEN_POS_OFFSET
	sample_2d_range = HSM_GetCacheSampleRange(3, 1);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = tube_scale;
		out_color.ba = pos_offset;
	}

	// Sample 4, 1
	// rg CROPPED_ROTATED_SIZE_WITH_RES_MULT
	sample_2d_range = HSM_GetCacheSampleRange(4, 1);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = cropped_rotated_size_with_res_mult;
		out_color.ba = HSM_GetRotatedDerezedSize();
	}

	// Sample 1, 2
	// rg CROPPED_ROTATED_SIZE
	// ba SAMPLE_AREA_START_PIXEL_COORD
	sample_2d_range = HSM_GetCacheSampleRange(1, 2);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = cropped_rotated_size;
		out_color.ba = cropped_sample_area_start_pixel_coord;
	}

	// // Sample 5, 1
	// // rg CROPPED_UNROTATED_SIZE
	// sample_2d_range = HSM_GetCacheSampleRange(5, 1);
	// if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	// {
	// 	out_color.rg = cropped_unrotated_size;
	// 	out_color.ba = cropped_unrotated_size_with_res_mult;
	// }

// TODO Need to add unrotated sizes

	// Sample 2, 2
	// r SCREEN_ASPECT_2ND_SCREEN
	// g PARAMETER_SUM_SCREEN_SCALE
	// ba SCREEN_SCALE_2ND_SCREEN
	sample_2d_range = HSM_GetCacheSampleRange(2, 2);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.r = screen_aspect_2nd_screen;
		out_color.gb = screen_scale_2nd_screen;
	}

	// Sample 3, 2
	// rg TUBE_SCALE
	// ba SCREEN_POS_OFFSET
	sample_2d_range = HSM_GetCacheSampleRange(3, 2);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = tube_scale_2nd_screen;
		out_color.ba = pos_offset_2nd_screen;
	}

	// Sample 4, 2
	// rg CROPPED_ROTATED_SIZE_WITH_RES_MULT 2nd screen
	sample_2d_range = HSM_GetCacheSampleRange(4, 2);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = cropped_size_with_res_mult_2nd_screen;
	}

	// Sample 1, 3
	// rg CROPPED_ROTATED_SIZE 2nd screen
	// ba SAMPLE_AREA_START_PIXEL_COORD 2nd screen
	sample_2d_range = HSM_GetCacheSampleRange(1, 3);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = cropped_size_2nd_screen;
		out_color.ba = sample_area_start_pixel_coord_2nd_screen;
	}

	// Sample 2, 3
	// rg CORE_SIZE
	sample_2d_range = HSM_GetCacheSampleRange(2, 3);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = global.OriginalSize.xy;
		out_color.ba = HSM_GetRotatedCoreOriginalSize();
	}

	// Sample 3, 3
	// rg VIEWPORT_SCALE
	// ba VIEWPORT_POS
	sample_2d_range = HSM_GetCacheSampleRange(3, 3);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = vec2(HSM_VIEWPORT_ZOOM, HSM_VIEWPORT_ZOOM);
		out_color.ba = vec2(HSM_VIEWPORT_POSITION_X, HSM_VIEWPORT_POSITION_Y);
	}

	// Sample 4, 3
	// rg SCREEN_SCALE_2ND_SCREEN
	// ba SCREEN_POS_OFFSET_2ND_SCREEN
	sample_2d_range = HSM_GetCacheSampleRange(4, 3);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = screen_scale_2nd_screen;
		out_color.ba = pos_offset_2nd_screen;
	}

	// Sample 1, 4
	sample_2d_range = HSM_GetCacheSampleRange(1, 4);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.r = parameter_sum;

		vec2 sample_coord = HSM_GetCacheSampleCoord(1, 4);
		vec4 texture_sample = texture(feedback_pass, sample_coord);

		float last_frame = floor(texture_sample.g);
		out_color.g = last_frame + global.FrameDirection * 1;
		out_color.ba = rotated_derezed_size;
	}

	// Sample 2, 4
	// r = NEGATIVE_CROP_EXPAND_MULTIPLIER
	// g = MAX_NEGATIVE_CROP;
	sample_2d_range = HSM_GetCacheSampleRange(2, 4);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.r = NEGATIVE_CROP_EXPAND_MULTIPLIER;
		out_color.g = MAX_NEGATIVE_CROP;
		out_color.b = use_vertical_scanlines;
	}

	// TODO need to add TUBE_DIFFUSE_ASPECT & deal with 2nd Screen
	// Sample 3, 4
	// rg TUBE_DIFFUSE_SCALE
	// ba TUBE_DIFFUSE_SCALE_2ND_SCREEN
	sample_2d_range = HSM_GetCacheSampleRange(3, 4);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = tube_diffuse_scale;
		out_color.ba = tube_diffuse_scale_2nd_screen;
	}

	// Sample 4, 4
	// rg SCREEN_SIZE
	// ba SCREEN_SIZE_2ND_SCREEN
	sample_2d_range = HSM_GetCacheSampleRange(4, 4);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.rg = screen_size;
		out_color.ba = screen_size_2nd_screen;
	}


	vec2 sample_coord = vec2(0); 
	vec4 texture_sample = vec4(0); 

	// Check the difference in parameter sum values to see if the cache has changed
	// Sample 1, 4 Feedback
	// r parameter_sum_feedback
	sample_coord = HSM_GetCacheSampleCoord(1, 4);
	texture_sample = texture(feedback_pass, sample_coord);
	float parameter_sum_feedback = texture_sample.r;

	//-----------------------------------------------------
	// Screen 1 resolution
	float res_mult_size_sum = cropped_rotated_size_with_res_mult.x + cropped_rotated_size_with_res_mult.y;

	// Sample 4, 1 Feedback
	// rg parameter_sum_feedback
	sample_coord = HSM_GetCacheSampleCoord(4, 1);
	texture_sample = texture(feedback_pass, sample_coord);
	CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK = texture_sample.rg;
	float res_mult_size_sum_feedback = CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK.x + CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK.y;

	//-----------------------------------------------------
	// Screen 2 resolution
	// r res_mult_size2_sum
	float res_mult_size2_sum = cropped_size_with_res_mult_2nd_screen.x + cropped_size_with_res_mult_2nd_screen.y;

	sample_coord = HSM_GetCacheSampleCoord(4, 2);
	texture_sample = texture(feedback_pass, sample_coord);
	CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK = texture_sample.rg;
	float res_mult_size2_sum_feedback = CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK.x + CROPPED_ROTATED_SIZE_WITH_RES_MULT_FEEDBACK.y;

	// Need to check croped sample resolutions of both screens
	bool cache_changed = res_mult_size_sum != res_mult_size_sum_feedback || 
						 res_mult_size2_sum != res_mult_size2_sum_feedback ||
						 abs((parameter_sum) - (parameter_sum_feedback)) > 0.0000001 ? true : false ||
						 HSM_CACHE_GRAPHICS_ON < 0.5;

	//-----------------------------------------------------
	// Store if the cache has changed since last frame
	// Sample 8, 8
	// r CACHE_INFO_CHANGED
	sample_2d_range = HSM_GetCacheSampleRange(8, 8);
	if (HSM_IsCoordIn2DRange(viewport_coord, sample_2d_range) == 1)
	{
		out_color.r = cache_changed ? 1 : 0;
	}

	return out_color;
}

void main()
{
   FragColor = HSM_GetColorForScreenInfoCache(vTexCoord, InfoCachePassFeedback, Source, ScreenPlacementImage);
}