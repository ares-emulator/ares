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

// Returns 1 for vertical split, 2 for horizontal split
float HSM_GetCoreImageSplitDirection()
{
	float core_image_split_direction = 1;

	if (HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE == 0)
	{
		if (HSM_DUALSCREEN_MODE == 1)
			core_image_split_direction = 1;
		if (HSM_DUALSCREEN_MODE == 2)
			core_image_split_direction = 2;
	}
	else
	{
		core_image_split_direction = HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE;
	}
	return core_image_split_direction;
}

vec2 HSM_GetCoordWithPositionOffset(vec2 in_coord, vec2 position_offset)
{
	return in_coord - position_offset;
}

vec2 HSM_GetInverseScaledCoord(vec2 in_coord, vec2 in_scale)
{
	vec2 middle = vec2(0.49999, 0.49999);
	vec2 diff = in_coord.xy - middle;
	vec2 screen_inverse_scale = 1.0 / in_scale;
	vec2 scaled_coord = middle + diff * screen_inverse_scale;

	return scaled_coord;
}

vec2 HSM_GetVTexCoordWithArgs(vec2 in_coord, vec2 in_scale, vec2 position_offset)
{
	return HSM_GetInverseScaledCoord(HSM_GetCoordWithPositionOffset(in_coord, position_offset), in_scale);
}

// Rows and Columns are 1 based
vec4 HSM_GetCacheSampleRange(float column_index, float row_index)
{
	float num_rows = 8;
	float num_columns = 8;

	float range_width = 1 / num_columns;
	float range_height = 1 / num_rows;

	float zero_based_row_index = row_index - 1;
	float zero_based_column_index = column_index - 1;

	vec4 out_sample_range = vec4(0);

	out_sample_range.x = zero_based_column_index * range_width;
	out_sample_range.y = zero_based_row_index * range_height;
	out_sample_range.z = out_sample_range.x + range_width;
	out_sample_range.w = out_sample_range.y + range_height;
	
	return out_sample_range;
}

vec2 HSM_GetCacheSampleCoord(float column_index, float row_index)
{
	float num_rows = 8;
	float num_columns = 8;

	float range_width = 1 / num_columns;
	float range_height = 1 / num_rows;

	vec4 sample_range = HSM_GetCacheSampleRange(column_index, row_index);
	return vec2(sample_range.x + range_width/2, sample_range.y + range_height/2);
}

vec2 HSM_GetViewportCoordWithFlip(vec2 viewport_coord)
{
	vec2 out_coord = viewport_coord;

	// out_coord.y = HSM_FLIP_VIEWPORT_VERTICAL * (out_coord.y - 0.5) + 0.5;
	// out_coord.x = HSM_FLIP_VIEWPORT_HORIZONTAL * (out_coord.x - 0.5) + 0.5;

	if (HSM_FLIP_VIEWPORT_VERTICAL == -1)
		out_coord.y = 1 - out_coord.y;
	
	if (HSM_FLIP_VIEWPORT_HORIZONTAL == -1)
		out_coord.x =  1 - out_coord.x;

	return out_coord;
}

vec2 HSM_GetViewportCoordWithZoomAndPan(vec2 viewport_coord)
{
	vec2 out_coord = HSM_GetViewportCoordWithFlip(viewport_coord);

	out_coord = (out_coord - 0.5) / HSM_VIEWPORT_ZOOM + 0.5;
	out_coord.x += HSM_VIEWPORT_POSITION_X;
	out_coord.y -= HSM_VIEWPORT_POSITION_Y; 

	return out_coord;
}

float HSM_GetScreenIndex(vec2 viewport_coord)
{
	float out_index = 1;
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;

	if (HSM_DUALSCREEN_MODE == 0)
		out_index = 1;
	if (HSM_DUALSCREEN_MODE == 1)
		out_index = (viewport_coord.y < 0.5 + HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION / output_aspect) ? 1 : 2;
	if (HSM_DUALSCREEN_MODE == 2)
		out_index = (viewport_coord.x < 0.5 + HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION / output_aspect) ? 1 : 2;

	return out_index;
}

vec4 HSM_UpdateGlobalScreenValuesFromCache(sampler2D in_cache_pass, vec2 vTexCoord)
{
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;
	vec2 flipped_viewport_coord = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	SCREEN_INDEX = HSM_GetScreenIndex(flipped_viewport_coord);
	vec2 sample_coord = vec2(0);
	vec4 texture_sample = vec4(0);

	// Sample 1, 1
	sample_coord = HSM_GetCacheSampleCoord(1, 1);
	texture_sample = texture(in_cache_pass, sample_coord);
	AVERAGE_LUMA = texture_sample.a;
	SAMPLING_SCANLINE_DIR_MULT = texture_sample.r;
	SAMPLING_OPPOSITE_DIR_MULT = texture_sample.g;

	float res_mult_size_sum = 0;
	float res_mult_size2_sum = 0;

	if (SCREEN_INDEX == 1)
	{
		// Sample 2, 1
		// r SCREEN_ASPECT
		// ba SCREEN_SCALE
		sample_coord = HSM_GetCacheSampleCoord(2, 1);
		texture_sample = texture(in_cache_pass, sample_coord);
		SCREEN_ASPECT = texture_sample.r;
		SCREEN_SCALE = texture_sample.ba;

		// Sample 3, 1
		// rg TUBE_SCALE 
		// ba SCREEN_POS_OFFSET
		sample_coord = HSM_GetCacheSampleCoord(3, 1);
		texture_sample = texture(in_cache_pass, sample_coord);
		TUBE_SCALE = texture_sample.rg;
		SCREEN_POS_OFFSET = texture_sample.ba;

		// Sample 3, 4
		// rg TUBE_DIFFUSE_SCALE 
		sample_coord = HSM_GetCacheSampleCoord(3, 4);
		texture_sample = texture(in_cache_pass, sample_coord);
		TUBE_DIFFUSE_SCALE = texture_sample.rg;
		TUBE_DIFFUSE_ASPECT = TUBE_DIFFUSE_SCALE.x / TUBE_DIFFUSE_SCALE.y * output_aspect;

		// Sample 4, 1
		// rg CROPPED_ROTATED_SIZE_WITH_RES_MULT
		sample_coord = HSM_GetCacheSampleCoord(4, 1);
		texture_sample = texture(in_cache_pass, sample_coord);
		CROPPED_ROTATED_SIZE_WITH_RES_MULT = texture_sample.rg;
		res_mult_size_sum = CROPPED_ROTATED_SIZE_WITH_RES_MULT.x + CROPPED_ROTATED_SIZE_WITH_RES_MULT.y;
		ROTATED_CORE_PREPPED_SIZE = texture_sample.ba;

		// Sample 1, 2
		// rg CROPPED_ROTATED_SIZE
		// ba SAMPLE_AREA_START_PIXEL_COORD
		sample_coord = HSM_GetCacheSampleCoord(1, 2);
		texture_sample = texture(in_cache_pass, sample_coord);
		CROPPED_ROTATED_SIZE = texture_sample.rg;
		SAMPLE_AREA_START_PIXEL_COORD = texture_sample.ba;

		// // Sample 5, 1
		// // rg CROPPED_UNROTATED_SIZE
		// sample_coord = HSM_GetCacheSampleCoord(5, 1);
		// CROPPED_UNROTATED_SIZE = texture_sample.rg;
		// CROPPED_UNROTATED_SIZE_WITH_RES_MULT = texture_sample.ba;

		// Sample 4, 4
		// rg screen size first screen
		// ba screen size second screen
		sample_coord = HSM_GetCacheSampleCoord(4, 4);
		texture_sample = texture(in_cache_pass, sample_coord);
		SCREEN_SIZE = texture_sample.rg;
	}
	// If we are in the section of the viewport which is the second screen
	if (SCREEN_INDEX == 2)
	{
		// Sample 2, 2 Sample - 2nd Screen
		// r SCREEN_ASPECT
		// ba SCREEN_SCALE
		sample_coord = HSM_GetCacheSampleCoord(2, 2);
		texture_sample = texture(in_cache_pass, sample_coord);
		SCREEN_ASPECT = texture_sample.r;
		SCREEN_SCALE = texture_sample.gb;

		// Sample 3, 2 - 2nd Screen
		// rg TUBE_SCALE
		// ba SCREEN_POS_OFFSET
		sample_coord = HSM_GetCacheSampleCoord(3, 2);
		texture_sample = texture(in_cache_pass, sample_coord);
		TUBE_SCALE = 		texture_sample.rg;
		SCREEN_POS_OFFSET = texture_sample.ba;

		// TODO need to add TUBE_DIFFUSE_ASPECT & deal with 2nd Screen
		// Sample 3, 4
		// ba TUBE_DIFFUSE_SCALE 
		sample_coord = HSM_GetCacheSampleCoord(3, 4);
		texture_sample = texture(in_cache_pass, sample_coord);
		TUBE_DIFFUSE_SCALE = texture_sample.ba;
		TUBE_DIFFUSE_ASPECT = TUBE_DIFFUSE_SCALE.x / TUBE_DIFFUSE_SCALE.y * output_aspect;

		// Sample 4, 2
		// rg CROPPED_ROTATED_SIZE_WITH_RES_MULT
		sample_coord = HSM_GetCacheSampleCoord(4, 2);
		texture_sample = texture(in_cache_pass, sample_coord);
		CROPPED_ROTATED_SIZE_WITH_RES_MULT = texture_sample.rg;
		res_mult_size2_sum = CROPPED_ROTATED_SIZE_WITH_RES_MULT.x + CROPPED_ROTATED_SIZE_WITH_RES_MULT.y;

		// Sample 1, 3
		// rg CROPPED_ROTATED_SIZE
		// ba SAMPLE_AREA_START_PIXEL_COORD
		sample_coord = HSM_GetCacheSampleCoord(1, 3);
		texture_sample = texture(in_cache_pass, sample_coord);
		CROPPED_ROTATED_SIZE = texture_sample.rg;
		SAMPLE_AREA_START_PIXEL_COORD = texture_sample.ba;

		// Sample 4, 4
		// rg screen size first screen
		// ba screen size second screen
		sample_coord = HSM_GetCacheSampleCoord(4, 4);
		texture_sample = texture(in_cache_pass, sample_coord);
		SCREEN_SIZE = texture_sample.ba;
	}

	// Sample 3, 1
	// rg TUBE_SCALE 
	// ba SCREEN_POS_OFFSET
	sample_coord = HSM_GetCacheSampleCoord(3, 1);
	texture_sample = texture(in_cache_pass, sample_coord);
	// TUBE_SCALE_1ST_SCREEN = texture_sample.rg;
	SCREEN_POS_OFFSET_1ST_SCREEN = texture_sample.ba;

	// Sample 3, 4
	// rg TUBE_DIFFUSE_SCALE_1ST_SCREEN 
	sample_coord = HSM_GetCacheSampleCoord(3, 4);
	texture_sample = texture(in_cache_pass, sample_coord);
	TUBE_DIFFUSE_SCALE_1ST_SCREEN = texture_sample.rg;
	// TUBE_DIFFUSE_ASPECT_1ST_SCREEN = TUBE_DIFFUSE_SCALE_1ST_SCREEN.x / TUBE_DIFFUSE_SCALE_1ST_SCREEN.y * output_aspect;

	// Sample 3, 2 - 2nd Screen
	// rg TUBE_SCALE_2ND_SCREEN
	// ba SCREEN_POS_OFFSET_2ND_SCREEN
	sample_coord = HSM_GetCacheSampleCoord(3, 2);
	texture_sample = texture(in_cache_pass, sample_coord);
	// TUBE_SCALE_2ND_SCREEN = 		texture_sample.rg;
	SCREEN_POS_OFFSET_2ND_SCREEN = texture_sample.ba;

	// TODO need to add TUBE_DIFFUSE_ASPECT & deal with 2nd Screen
	// Sample 3, 4
	// ba TUBE_DIFFUSE_SCALE_2ND_SCREEN 
	sample_coord = HSM_GetCacheSampleCoord(3, 4);
	texture_sample = texture(in_cache_pass, sample_coord);
	TUBE_DIFFUSE_SCALE_2ND_SCREEN = texture_sample.ba;

	// Sample 2, 3 Sample
	// rg CORE_SIZE
	sample_coord = HSM_GetCacheSampleCoord(2, 3);
	texture_sample = texture(in_cache_pass, sample_coord);
	CORE_SIZE = texture_sample.rg;
	ROTATED_CORE_ORIGINAL_SIZE = texture_sample.ba;

	// Sample 3, 3
	// rg VIEWPORT_SCALE
	// ba VIEWPORT_POS
	sample_coord = HSM_GetCacheSampleCoord(3, 3);
	texture_sample = texture(in_cache_pass, sample_coord);
	VIEWPORT_SCALE = texture_sample.rg;
	VIEWPORT_POS = texture_sample.ba;

	// Sample 4, 3
	// rg SCREEN_SCALE_2ND_SCREEN
	// ba SCREEN_POS_OFFSET_2ND_SCREEN
	sample_coord = HSM_GetCacheSampleCoord(4, 3);
	texture_sample = texture(in_cache_pass, sample_coord);
	SCREEN_SCALE_2ND_SCREEN = texture_sample.rg;
	SCREEN_POS_OFFSET_2ND_SCREEN = texture_sample.ba;

	// Sample 1, 4
	// g CURRENT_FRAME_FROM_CACHE_INFO
	// b ROTATED_DEREZED_SIZE
	sample_coord = HSM_GetCacheSampleCoord(1, 4);
	texture_sample = texture(in_cache_pass, sample_coord);
	CURRENT_FRAME_FROM_CACHE_INFO = texture_sample.g;
	ROTATED_DEREZED_SIZE = texture_sample.ba;

	// Sample 2, 4 
	// r NEGATIVE_CROP_EXPAND_MULTIPLIER
	// g MAX_NEGATIVE_CROP
	sample_coord = HSM_GetCacheSampleCoord(2, 4);
	texture_sample = texture(in_cache_pass, sample_coord);
	NEGATIVE_CROP_EXPAND_MULTIPLIER = texture_sample.r;
	MAX_NEGATIVE_CROP = texture_sample.g;
	USE_VERTICAL_SCANLINES = texture_sample.b;

	// Sample 8, 8
	// r CACHE_INFO_CHANGED
	sample_coord = HSM_GetCacheSampleCoord(8, 8);
	texture_sample = texture(in_cache_pass, sample_coord);
	CACHE_INFO_CHANGED = texture_sample.r > 0.5 ? true : false;

	SCREEN_SCALE_WITH_ZOOM = SCREEN_SCALE * HSM_VIEWPORT_ZOOM;
	SCREEN_COORD = HSM_GetVTexCoordWithArgs(flipped_viewport_coord, SCREEN_SCALE, SCREEN_POS_OFFSET);
	TUBE_DIFFUSE_COORD = HSM_GetVTexCoordWithArgs(flipped_viewport_coord, TUBE_DIFFUSE_SCALE, SCREEN_POS_OFFSET);
	TUBE_DIFFUSE_COORD_MIXED_POS = HSM_GetVTexCoordWithArgs(flipped_viewport_coord, TUBE_DIFFUSE_SCALE_1ST_SCREEN, (SCREEN_POS_OFFSET_1ST_SCREEN + SCREEN_POS_OFFSET_2ND_SCREEN) / 2.0);

	return vec4(0);
}