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

#include "helper-functions.inc"
#include "base-functions.inc"

#ifndef IS_POTATO_PRESET
#include "royale-geometry-functions.inc"
#endif

float CURVATURE_MODE_OFF = 0;
float CURVATURE_MODE_2D = 1;
float CURVATURE_MODE_2D_CYLINDER = 2;
float CURVATURE_MODE_3D_1 = 3;
float CURVATURE_MODE_3D_2 = 4;
float CURVATURE_MODE_3D_CYLINDER = 5;

float HSM_GetAspectRatioFromMode(float in_aspect_ratio_mode, float in_explicit_aspect)
{                                                  
	float out_explicit_aspect = in_explicit_aspect;

	if (in_aspect_ratio_mode == TEXTURE_ASPECT_MODE_VIEWPORT)
		out_explicit_aspect = global.OutputSize.x / global.OutputSize.y;
	if (in_aspect_ratio_mode == TEXTURE_ASPECT_MODE_4_3)
		out_explicit_aspect = 1.33333;
	if (in_aspect_ratio_mode == TEXTURE_ASPECT_MODE_3_4)
		out_explicit_aspect = 0.75;
	if (in_aspect_ratio_mode == TEXTURE_ASPECT_MODE_16_9)
		out_explicit_aspect = 1.7777;
	if (in_aspect_ratio_mode == TEXTURE_ASPECT_MODE_9_16)
		out_explicit_aspect = 0.5625;

	return out_explicit_aspect;
}

vec2 HSM_GetRotatedCoreOriginalSize()
{
	return abs(HSM_ROTATE_CORE_IMAGE) * global.OriginalSize.yx + (1 - abs(HSM_ROTATE_CORE_IMAGE)) * global.OriginalSize.xy;
}

// Core Prepped Size is the size after DeRez
vec2 HSM_GetDerezedSize()
{
	return global.DerezedPassSize.xy;
}

vec2 HSM_GetRotatedDerezedSize()
{
	return abs(HSM_ROTATE_CORE_IMAGE) * global.DerezedPassSize.yx + (1 - abs(HSM_ROTATE_CORE_IMAGE)) * global.DerezedPassSize.xy;
}

float HSM_GetSwappedScreenIndex(float screen_index)
{
	float out_index = screen_index;

	if (HSM_DUALSCREEN_CORE_IMAGE_SWAP_SCREENS == 1)
	{
		if (screen_index == 1)
		{
			out_index = 2;
		}
		else
		{
			out_index = 1;
		}
	}

	return out_index;
}

vec2 HSM_RotateCoordinate(vec2 in_coord, float rotation)
{
	if (rotation == 0)
		return in_coord;
	
	float abs_rotation = abs(rotation);
	vec2 ctr_coord = in_coord - 0.5;

	ctr_coord = (1 - abs_rotation) * ctr_coord
				+ clamp(abs_rotation, 0, 1) * abs_rotation * vec2(-ctr_coord.y, ctr_coord.x)
				+ abs(clamp(abs_rotation, -1, 0)) * abs_rotation * vec2(ctr_coord.y, -ctr_coord.x);

	if (rotation < 0)
		ctr_coord *= -1;

	return ctr_coord + 0.5;
}

// Texture Sampler function which takes a coordinate in the cropped coordinate space
vec4 HSM_GetTexSampleFromSampleStartAndSize(sampler2D in_sampler, vec2 in_screen_coord, vec2 sample_start_pixel_coord, vec2 window_size)
{
	vec2 core_prepped_size = HSM_GetRotatedDerezedSize();

	if ( HSM_DUALSCREEN_MODE > 0 )
		if (HSM_FLIP_CORE_VERTICAL == -1)
			in_screen_coord.y = 1 - in_screen_coord.y;
			
		// in_screen_coord.y = abs(HSM_FLIP_CORE_VERTICAL) * (1 - in_screen_coord.y) + (1 - abs(HSM_FLIP_CORE_VERTICAL)) * in_screen_coord.y;
		// in_screen_coord.y = HSM_FLIP_CORE_VERTICAL * (in_screen_coord.y - 0.5) + 0.5;
		
	vec2 px_coord = SAMPLE_AREA_START_PIXEL_COORD + in_screen_coord * window_size;

	vec2 sample_coord = px_coord / core_prepped_size;
	sample_coord =  HSM_RotateCoordinate(sample_coord, HSM_ROTATE_CORE_IMAGE);
	
	vec4 out_color = texture(in_sampler, sample_coord);

	return out_color;
}

// Applies the position before scaling from the center
// Should allow having an image where the center of the tube in the graphic is off center
// the position offset moves it so the screen is centered, then the graphic is scaled from the center 
vec2 HSM_AddPosScaleToCoord(vec2 in_base_coord, vec2 in_pos, vec2 in_scale)
{
	vec2 positioned_coord = in_base_coord + in_pos;
	vec2 out_coord = HSM_GetInverseScaledCoord(positioned_coord, in_scale);
	return out_coord;
}

vec2 GetSimpleImageScaledCoord(vec2 in_viewport_coord, vec2 in_viewport_unscaled_coord, vec2 in_tube_coord, vec2 in_tube_scale, in sampler2D in_sampler, vec2 in_pos, float in_inherit_pos, vec2 in_scale, float in_scale_inherit_mode, float in_keep_aspect, float in_mirror_horz, float in_rotate )
{
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;

	vec2 coord_ctr = vec2(1);

	if (in_scale_inherit_mode == 0)
		coord_ctr = in_viewport_unscaled_coord - 0.5;
	if (in_scale_inherit_mode == 1)
		coord_ctr = in_viewport_coord - 0.5;

	if (in_scale_inherit_mode == 2)
	{
		if (in_inherit_pos < 0.5 || HSM_DUALSCREEN_MODE > 0.5)
			coord_ctr = (in_viewport_coord - 0.5) / in_tube_scale * vec2((in_tube_scale.x / in_tube_scale.y), 1) * DEFAULT_SCREEN_HEIGHT; //in_tube_scale.y / (in_tube_scale.y / DEFAULT_SCREEN_HEIGHT);
		else
		 	coord_ctr = (in_tube_coord - 0.5) * vec2((in_tube_scale.x / in_tube_scale.y), 1) * DEFAULT_SCREEN_HEIGHT; // / vec2( 1 / (in_tube_scale.x / in_tube_scale.y), 1);

		// If it's dual screen is on, then the screens are at least half the size,
		// so scale up the image so it covers the whole viewport by default
		if (HSM_DUALSCREEN_MODE > 0.5)
			coord_ctr *= 0.5;
	}

	coord_ctr.x = in_mirror_horz == 1 ? -1 * coord_ctr.x : coord_ctr.x;
	
	in_viewport_coord = HSM_RotateCoordinate(in_viewport_coord, in_rotate);

	vec2 tex_size = textureSize(in_sampler, 0);
	float tex_aspect = in_rotate == 1 ? tex_size.y / tex_size.x : tex_size.x / tex_size.y;

	coord_ctr.x *= in_keep_aspect == 1 ? output_aspect / tex_aspect : 1;
	coord_ctr /= in_rotate > 0.5 ? global.FinalViewportSize.x / global.FinalViewportSize.y : 1;

	if (in_rotate > 0.5)
	{
		coord_ctr = vec2(-coord_ctr.y, -coord_ctr.x);
	}

	return HSM_AddPosScaleToCoord(coord_ctr + 0.5, in_pos, in_scale);
}

vec2 HSM_GetScreenPositionOffset(vec2 placement_image_pos, vec2 screen_scale, float screen_index )
{

	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;


	// If we are not using the image placement then get its offset from the center
	placement_image_pos = HSM_USE_IMAGE_FOR_PLACEMENT == 1 && screen_index == 1 ? placement_image_pos : vec2(0.5);

	vec2 pos_offset = screen_index == 1 ? vec2(HSM_SCREEN_POSITION_X / output_aspect, HSM_SCREEN_POSITION_Y) + (placement_image_pos - 0.5)
										: vec2(HSM_2ND_SCREEN_POS_X / output_aspect, HSM_2ND_SCREEN_POS_Y);

	float split_offset_multiplier = screen_index == 1 ? -1 : 1;

	if (HSM_DUALSCREEN_MODE == 1)
	{
		if (HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE == 1)
			pos_offset.y += HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION + split_offset_multiplier * screen_scale.y * 1.17 / 2;
		else
			pos_offset.y += split_offset_multiplier * 0.25;

		pos_offset.y += split_offset_multiplier * HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS;
	}

	if (HSM_DUALSCREEN_MODE == 2)
	{
		if (HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE == 1)
			pos_offset.x += HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION / output_aspect + split_offset_multiplier * screen_scale.x * 1.17 / 2;
		else
			pos_offset.x += split_offset_multiplier * 0.25 / output_aspect;

		pos_offset.x += split_offset_multiplier * HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS / output_aspect;
	}

	return pos_offset;
}

float HSM_GetAverageLuma(sampler2D Source, vec2 SourceSize)
{
   //////// Calculate Average Luminance ////////// 
   float m = max(log2(global.SourceSize.x), log2(global.SourceSize.y));
	m = max(m - 1.0, 1.0);
	
	float luma_total = 0.0;
	
	float num_samples = 5;
	float sample_dist = 1 / (num_samples - 1);
	vec4 tex_sample = vec4(0);
	for (float i = 0; i <= num_samples; i++)
	{
		for (float j = 0; j <= num_samples; j++)
		{
			tex_sample = textureLod(Source, vec2(sample_dist * i, sample_dist * j), m);
			luma_total += max(0.0, (tex_sample.r + tex_sample.g + tex_sample.g) / 3);
			// luma_total += max(0.0, length(tex_sample.rgb));
		}
	}
	luma_total = pow(0.577350269 * luma_total / (num_samples * num_samples), 0.6);
   return luma_total;
}

vec3 HSM_GetScreenPlacementAndHeight(sampler2D in_sampler_2D, float num_samples)
{
	if (HSM_USE_IMAGE_FOR_PLACEMENT == 1)
	{
		float screen_top_y_pos = 1;
		float screen_bottom_y_pos = 0;

		for (int i=0; i < num_samples; i++)
		{
			float y_pos = i * 1 / num_samples;
			vec4 sample_color = texture(in_sampler_2D, vec2(0.5, y_pos));
			float test_value = 0;
			if (HSM_PLACEMENT_IMAGE_MODE > 0)
				test_value = (sample_color.r + sample_color.b + sample_color.g) / 3;
			else
				test_value = 1 - sample_color.a;

			if (test_value > 0.5)
			{
				screen_top_y_pos = min(screen_top_y_pos, y_pos);
				screen_bottom_y_pos = max(screen_bottom_y_pos, y_pos);
			}
		}

		float screen_left_x_pos = 0.75;
		float screen_right_x_pos = 0.25;

		if (HSM_PLACEMENT_IMAGE_USE_HORIZONTAL == 1)
			for (int i=0; i < num_samples; i++)
			{
				float x_pos = 0.25 + i * 0.5 / num_samples;
				vec4 sample_color = texture(in_sampler_2D, vec2(x_pos, 0.5));
				float test_value = 0;
				if (HSM_PLACEMENT_IMAGE_MODE == 1)
					test_value = (sample_color.r + sample_color.b + sample_color.g) / 3;
				else
					test_value = 1 - sample_color.a;

				if (test_value > 0.5)
				{
					screen_left_x_pos = min(screen_left_x_pos, x_pos);
					screen_right_x_pos = max(screen_right_x_pos, x_pos);
				}
			}

		return vec3((screen_left_x_pos + screen_right_x_pos) / 2, (screen_top_y_pos + screen_bottom_y_pos) / 2, screen_bottom_y_pos - screen_top_y_pos);
	}
	else
		return vec3(0.5, 0.5, 1);
}

vec2 HSM_GetScreenVTexCoord(vec2 in_coord, vec2 in_screen_scale, vec2 position_offset)
{
	return HSM_GetVTexCoordWithArgs(in_coord, in_screen_scale, position_offset);
}

vec2 HSM_GetCurvatureValues(float screen_aspect)
{
	vec2 curvature_values = screen_aspect < 1 ? vec2(2 * HSM_CURVATURE_2D_SCALE_SHORT_AXIS * 2 / 100, HSM_CURVATURE_2D_SCALE_LONG_AXIS * 3 / 100)
											  : vec2(HSM_CURVATURE_2D_SCALE_LONG_AXIS * 3 / 100, 2 * HSM_CURVATURE_2D_SCALE_SHORT_AXIS * 2 / 100);
	return curvature_values;
}


// CRT Geom Curvature
#define FIX(c) max(abs(c), 1e-5)

float intersect(vec2 in_coord , vec2 sinangle, vec2 cosangle, float in_radius, float in_distance)
{
  float A = dot(in_coord, in_coord) + in_distance.x * in_distance.x;
  float B = 2.0 * (in_radius * (dot(in_coord, sinangle) - in_distance.x * cosangle.x * cosangle.y) - in_distance.x * in_distance.x);
  float C = in_distance.x * in_distance.x + 2.0 * in_radius * in_distance.x * cosangle.x * cosangle.y;
  return (-B-sqrt(B * B - 4.0 * A * C)) / (2.0 * A);
}

vec2 bkwtrans(vec2 in_coord, vec2 sinangle, vec2 cosangle, float in_radius, float in_distance)
{
  float c = intersect(in_coord, sinangle, cosangle, in_radius, in_distance);
  vec2 pt = vec2(c) * in_coord;
  pt -= vec2(-in_radius) * sinangle;
  pt /= vec2(in_radius);
  vec2 tang = sinangle / cosangle;
  vec2 poc = pt / cosangle;
  float A = dot(tang, tang) + 1.0;
  float B = -2.0 * dot(poc, tang);
  float C = dot(poc,poc)-1.0;
  float a = (-B + sqrt(B * B - 4.0 * A * C)) / (2.0 * A);
  vec2 uv = (pt - a * sinangle) / cosangle;
  float r = FIX(in_radius * acos(a));
  return uv * r / sin(r / in_radius);
}

vec2 fwtrans(vec2 uv, vec2 sinangle, vec2 cosangle, float in_radius, float in_distance)
{
  float r = FIX(sqrt(dot(uv,uv)));
  uv *= sin(r/in_radius)/r;
  float x = 1.0-cos(r/in_radius);
  float D = in_distance/in_radius + x*cosangle.x*cosangle.y+dot(uv,sinangle);
  return in_distance*(uv*cosangle-x*sinangle)/D;
}

vec3 maxscale(vec2 sinangle, vec2 cosangle, float in_radius, float in_distance, float in_aspect)
{
  vec2 aspect_vec2 = vec2(1, 1/in_aspect);
  vec2 c = bkwtrans(-in_radius * sinangle / (1.0 + in_radius/in_distance*cosangle.x*cosangle.y), sinangle, cosangle, in_radius, in_distance);
  vec2 a = vec2(0.5,0.5)*aspect_vec2.xy;

  vec2 lo = vec2(	fwtrans(vec2(-a.x,c.y), sinangle, cosangle, in_radius, in_distance).x,
		 			fwtrans(vec2(c.x,-a.y), sinangle, cosangle, in_radius, in_distance).y)/aspect_vec2.xy;

  vec2 hi = vec2(	fwtrans(vec2(+a.x,c.y), sinangle, cosangle, in_radius, in_distance).x,
		 			fwtrans(vec2(c.x,+a.y), sinangle, cosangle, in_radius, in_distance).y)/aspect_vec2.xy;

  return vec3((hi+lo)*aspect_vec2.xy*0.5,max(hi.x-lo.x,hi.y-lo.y));
}

vec2 transform(vec2 coord, vec3 stretch, vec2 sinangle, vec2 cosangle, float in_radius, float in_distance, vec2 aspect)
{
  coord = (coord-vec2(0.5))*aspect.xy*stretch.z+stretch.xy;
  return (bkwtrans(coord, sinangle, cosangle, in_radius, in_distance)/aspect.xy+vec2(0.5));
}

// TODO need to rescale so the screen does not shrink
vec2 HSM_GetGeomCurvedCoord(vec2 in_coord, float tilt_x, float tilt_y, float in_radius, float in_distance, float in_aspect)
{
	//default radius = 3.5
    //default distance = 2
	in_distance *= 1.4;
	vec2 ang = vec2(tilt_x, tilt_y);
	vec2 v_sinangle = sin(ang);
	vec2 v_cosangle = cos(ang);
	vec3 v_stretch = maxscale(v_sinangle, v_cosangle, in_radius, in_distance, in_aspect);
	vec2 aspect_vec2 = vec2(1, 1/in_aspect);
	vec2 curved_coord = transform(in_coord, v_stretch, v_sinangle, v_cosangle, in_radius, in_distance, aspect_vec2);

	return curved_coord;
}

vec2 HSM_GetGeomCurvedCoordRetainWidth(vec2 in_coord, float tilt_x, float tilt_y, float in_radius, float in_distance, float in_aspect)
{
	vec2 ctr_curved_coord =  HSM_GetGeomCurvedCoord(in_coord, tilt_x, tilt_y, in_radius, in_distance, in_aspect) - 0.5;
	vec2 right_edge_curved_ctr_coord = HSM_GetGeomCurvedCoord(vec2(1, 0.5), tilt_x, tilt_y, in_radius, in_distance, in_aspect) - 0.5;
	ctr_curved_coord.x = ctr_curved_coord.x * 0.5 / right_edge_curved_ctr_coord.x;
	return ctr_curved_coord + 0.5;
}

/*
vec2 HSM_GetGuestCurvedCoord(vec2 in_coord, vec2 in_curvature, float in_curvature_shape)
{
	vec2 pos = in_coord;
	float warpX = in_curvature.x;
	float warpY = in_curvature.y;
	float c_shape = in_curvature_shape;

	pos  = pos*2.0-1.0;    
	pos  = mix(pos, vec2(pos.x*inversesqrt(1.0-c_shape*pos.y*pos.y), pos.y*inversesqrt(1.0-c_shape*pos.x*pos.x)), vec2(warpX, warpY)/c_shape);
	return pos*0.5 + 0.5;
}
*/

/*
vec2 HSM_GetTorridGristleCurvedCoord(vec2 in_coord, vec2 in_curvature){
	// default curvature is vec2(0.031, 0.041
	vec2 Distortion = in_curvature * 15;// * vec2(0.031, 0.041);

	vec2 curvedCoords = in_coord * 2.0 - 1.0;
	float curvedCoordsDistance = sqrt(curvedCoords.x*curvedCoords.x+curvedCoords.y*curvedCoords.y);

	curvedCoords = curvedCoords / curvedCoordsDistance;

	curvedCoords = curvedCoords * (1.0-pow(vec2(1.0-(curvedCoordsDistance/1.4142135623730950488016887242097)),(1.0/(1.0+Distortion*0.2))));

	curvedCoords = curvedCoords / (1.0-pow(vec2(0.29289321881345247559915563789515),(1.0/(vec2(1.0)+Distortion*0.2))));

	curvedCoords = curvedCoords * 0.5 + 0.5;
	return curvedCoords;
}
*/

vec2 HSM_GetCrtPiCurvedCoord(vec2 in_coord, vec2 in_curvature)
{
	// Barrel distortion shrinks the display area a bit, this will allow us to counteract that.
	in_curvature *= 5;
	vec2 barrelScale = 1.0 - (0.23 * in_curvature);
    in_coord -= vec2(0.5);
    float rsq = in_coord.x * in_coord.x + (HSM_CURVATURE_MODE == 2 ? 0 : in_coord.y * in_coord.y);
    in_coord += in_coord * (in_curvature * rsq);
    in_coord *= barrelScale;
	in_coord += vec2(0.5);
    return in_coord;
}

vec2 HSM_Get2DCurvedCoord(vec2 in_coord, vec2 curvature_values)
{
	vec2 ctr_curved_coord = vec2(0) ;

	ctr_curved_coord = HSM_GetCrtPiCurvedCoord(in_coord, curvature_values) - 0.5;

	vec2 right_edge_curved_ctr_coord = HSM_GetCrtPiCurvedCoord(vec2(1, 0.5), curvature_values) - 0.5;
	ctr_curved_coord.x = ctr_curved_coord.x * 0.5 / right_edge_curved_ctr_coord.x;

	vec2 bottom_edge_curved_ctr_coord =  HSM_GetCrtPiCurvedCoord(vec2(0.5, 1), curvature_values) - 0.5;
	ctr_curved_coord.y = ctr_curved_coord.y * 0.5 / bottom_edge_curved_ctr_coord.y;

	return ctr_curved_coord + 0.5;
}

vec2 HSM_GetCurvedCoord(vec2 in_coord, float curvature_multiplier, float screen_aspect)
{
	if (HSM_CURVATURE_MODE == CURVATURE_MODE_OFF)
		return in_coord;
	
	float epsilon = 0.002;

	vec2 adjusted_coord = in_coord;
	float tilt_angle_y = HSM_CURVATURE_3D_TILT_ANGLE_Y;
	float tilt_angle_x = HSM_CURVATURE_3D_TILT_ANGLE_X;
	float pin_inner_edge = 0;

	vec2 curved_coord = vec2(0);

#ifndef IS_POTATO_PRESET
	if (HSM_CURVATURE_MODE > CURVATURE_MODE_2D_CYLINDER)
	{
		if (HSM_USE_GEOM > 0.5)
			curved_coord = HSM_GetGeomCurvedCoordRetainWidth(in_coord, HSM_CURVATURE_3D_TILT_ANGLE_X, HSM_CURVATURE_3D_TILT_ANGLE_Y, HSM_CURVATURE_3D_RADIUS, HSM_CURVATURE_3D_VIEW_DIST, screen_aspect);
		else
		{
			float geom_radius_with_mult = HSM_CURVATURE_3D_RADIUS;
			// Adjust curvature so 3D mode 1 looks similar to 3D mode 2
			if (HSM_CURVATURE_MODE == CURVATURE_MODE_3D_1) geom_radius_with_mult -= 0.40;
			if (HSM_CURVATURE_MODE == CURVATURE_MODE_3D_CYLINDER) geom_radius_with_mult -= 1;
			geom_radius_with_mult *= (1 / (curvature_multiplier + epsilon));
			mat2x2 pixel_to_video_uv;

			float geom_mode = HSM_CURVATURE_MODE - 2;

			curved_coord = HRG_GetGeomCurvedCoord(	adjusted_coord, 
													geom_mode, 
													geom_radius_with_mult,
													HSM_CURVATURE_3D_VIEW_DIST,
													tilt_angle_x,
													tilt_angle_y,
													screen_aspect,
													pin_inner_edge,
													global.SourceSize.xy,
													global.OutputSize.xy,
													pixel_to_video_uv);
		}
	}
	else
	{
		vec2 curvature_values = curvature_multiplier * HSM_GetCurvatureValues(screen_aspect);
		curved_coord = HSM_Get2DCurvedCoord(adjusted_coord, curvature_values);
	}
#endif

#ifdef IS_POTATO_PRESET
	vec2 curvature_values = curvature_multiplier * HSM_GetCurvatureValues(screen_aspect);
	curved_coord = HSM_Get2DCurvedCoord(adjusted_coord, curvature_values);
#endif

	return curved_coord;
}

vec2 HSM_GetCRTShaderCurvedCoord(vec2 in_coord)
{	
	vec2 out_coord = HSM_GetCurvedCoord(in_coord, 1, SCREEN_ASPECT);
	
	if (HHLP_IsOutsideCoordSpace(out_coord))
	{
		vec2 tube_scale_ratio = TUBE_SCALE / SCREEN_SCALE;
		out_coord = (out_coord - 0.5) / tube_scale_ratio + 0.5;
	}
	else if (HSM_CRT_CURVATURE_SCALE < 100)
		out_coord = HSM_GetCurvedCoord(in_coord, HSM_CRT_CURVATURE_SCALE, SCREEN_ASPECT);

	return out_coord;
}

vec2 HSM_GetMirrorWrappedCoord(vec2 in_coord)
{	
	vec2 ctr_coord = in_coord - 0.5;
	if (abs(ctr_coord.x) > 0.5 || abs(ctr_coord.y) > 0.5 )
		in_coord = ctr_coord / HSM_SCREEN_REFLECTION_SCALE + 0.5 + vec2(HSM_SCREEN_REFLECTION_POS_X, HSM_SCREEN_REFLECTION_POS_Y);

	in_coord = mod(in_coord, 2);
	vec2 ctr_mirror_coord = in_coord - 0.5;

	float mirror_x = clamp(clamp(abs(ctr_mirror_coord.x) - 0.5, 0, 1) * 100000, 0, 1);
	float mirror_y = clamp(clamp(abs(ctr_mirror_coord.y) - 0.5, 0, 1) * 100000, 0, 1);

	ctr_mirror_coord.x = ctr_mirror_coord.x - mirror_x * 2 * sign(ctr_mirror_coord.x) * (abs(ctr_mirror_coord.x) - 0.5);
	ctr_mirror_coord.y = ctr_mirror_coord.y - mirror_y * 2 * sign(ctr_mirror_coord.y) * (abs(ctr_mirror_coord.y) - 0.5);

	return ctr_mirror_coord + 0.5;
}

vec2 HSM_GetMirrorWrapCoord(vec2 in_coord)
{
	vec2 ctr_coord = in_coord - 0.5;
	vec2 ctr_mirror_coord = vec2(0,0);

	float x_is_outside = clamp((clamp(abs(ctr_coord.x), 0.5, 1) - 0.5) * 100000, 0, 1);
	ctr_mirror_coord.x = (1 - x_is_outside) * ctr_coord.x + 
						x_is_outside * (ctr_coord.x - 2 * sign(ctr_coord.x) * (abs(ctr_coord.x) - 0.5));

	float y_is_outside = clamp((clamp(abs(ctr_coord.y), 0.5, 1) - 0.5) * 100000, 0, 1);
	ctr_mirror_coord.y = (1 - y_is_outside) * ctr_coord.y + 
						y_is_outside * (ctr_coord.y - 2 * sign(ctr_coord.y) * (abs(ctr_coord.y) - 0.5));

	return ctr_mirror_coord + 0.5;
}

// Borrowed from cgwg's crt-geom, under GPL
float HSM_GetCornerMask(vec2 in_coord, float screen_aspect, float corner_radius, float edge_sharpness)
// returns 0.0 - 1.0 value used for masking the corner so it looks round
{
	//(0.5 - abs(in_coord - 0.5)) * 2
	vec2 new_coord = min(in_coord, vec2(1.0) - in_coord) * vec2(screen_aspect, 1);
	vec2 corner_distance = vec2(max(corner_radius / 1000.0, (1.0 - edge_sharpness) * 0.01));
	new_coord = (corner_distance - min(new_coord, corner_distance));
	float distance = sqrt(dot(new_coord, new_coord));

	return clamp((corner_distance.x - distance) * (edge_sharpness * 500 + 100), 0.0, 1.0);
}

// Guest improved corner mask, to integrate
// sborder is border intensity
// float corner(vec2 pos) {
// 	vec2 b = vec2(bsize1, bsize1) *  vec2(1.0, OutputSize.x/OutputSize.y) * 0.05;
// 	pos = clamp(pos, 0.0, 1.0);
// 	pos = abs(2.0*(pos - 0.5));
// 	float csize1 = mix(400.0, 7.0,  pow(4.0*csize, 0.10));
// 	float crn = dot(pow(pos, csize1.xx), vec2(1.0, OutputSize.y/OutputSize.x));
// 	crn = (csize == 0.0) ? max(pos.x, pos.y) : pow(crn, 1.0/csize1);
// 	pos = max(pos, crn);
// 	vec2 res = (bsize1 == 0.0) ? 1.0.xx : mix(0.0.xx, 1.0.xx, smoothstep(1.0.xx, 1.0.xx-b, sqrt(pos)));
// 	res = pow(res, sborder.xx);	
// 	return sqrt(res.x*res.y);
// }

vec2 HSM_GetTubeCurvedCoord(vec2 screen_coord, float curvature_scale, vec2 screen_scale, vec2 tube_scale, float screen_aspect, float apply_black_edge_offset)
{
	vec2 black_edge_scale_offset = tube_scale / screen_scale;

	// Get the curved coordinate
	vec2 tube_curved_coord = vec2(0.5, 0.5);

	if (HSM_BZL_USE_INDEPENDENT_CURVATURE == 1)
	{
		vec2 curvature_values = screen_aspect < 1 ? vec2(2 * HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS * 2 / 100, HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS * 3 / 100)
											: vec2(HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS * 3 / 100, 2 * HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS * 2 / 100);
		curvature_values *= curvature_scale * HSM_BZL_INNER_CURVATURE_SCALE;
		tube_curved_coord = HSM_Get2DCurvedCoord(screen_coord, curvature_values);
	}
	else
	{
		tube_curved_coord = HSM_GetCurvedCoord(screen_coord, curvature_scale * HSM_BZL_INNER_CURVATURE_SCALE, screen_aspect);
	}
	if (apply_black_edge_offset == 1)
		tube_curved_coord = HSM_GetInverseScaledCoord(tube_curved_coord, black_edge_scale_offset);

	return tube_curved_coord;
}

/*
Light Illumination
vec3 evaluateLight(in vec3 pos)
{
    vec3 lightPos = LPOS;
    vec3 lightCol = LCOL;
    vec3 L = lightPos-pos;
    return lightCol * 1.0/dot(L,L);
}

vec3 evaluateLight(in vec3 pos, in vec3 normal)
{
    vec3 lightPos = LPOS;
    vec3 L = lightPos-pos;
    float distanceToL = length(L);
    vec3 Lnorm = L/distanceToL;
    return max(0.0,dot(normal,Lnorm)) * evaluateLight(pos);
}
*/

float HSM_rand(inout float r)
{
	r = fract(3712.65 * r + 0.61432);
	return (r - 0.5) * 2.0;
}

vec4 HSM_GetStoichaicBlurredSample(sampler2D in_sampler, vec2 in_coord, float num_samples, float max_blur_size, float blur_ratio)
{
	if (num_samples < 1)
		return texture(in_sampler, in_coord);

	// Common value for max_blur_size is about 40
	float p = blur_ratio * max_blur_size / global.SourceSize.y;
	vec4 blurred_color = vec4(0.0);
	// srand
	float radius = sin(dot(in_coord, vec2(1233.224, 1743.335)));
	vec2 radius_vector;
	
	vec2 sample_coord = vec2(0);

	// The following are all unrolled otherwise they won't compile in D3D11
	if (num_samples < 1.5)
	{
		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 1;
	}

	if (num_samples < 2.5)
	{
		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 2;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 2;
	}

	if (num_samples > 2.5)
	{
		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;

		radius_vector.x = HSM_rand(radius);
		radius_vector.y = HSM_rand(radius);
		sample_coord = in_coord + radius_vector * p;

		blurred_color += texture(in_sampler, abs(sample_coord)) / 12;
	}

	return blurred_color;
}

bool HSM_GetIsInABCompareArea(vec2 viewport_coord)
{
	float test_value = HSM_AB_COMPARE_AREA > 1.5 ? viewport_coord.y : viewport_coord.x;
	float position = mod(HSM_AB_COMPARE_AREA, 2) == 1 ? (1 - HSM_AB_COMPARE_SPLIT_POSITION) : HSM_AB_COMPARE_SPLIT_POSITION;
	return  mod(HSM_AB_COMPARE_AREA, 2) == 0 && test_value < position || 
			mod(HSM_AB_COMPARE_AREA, 2) == 1 && test_value > position;
}

vec4 HSM_GetMipmappedTexSample(sampler2D in_sampler, vec2 in_coord, vec2 in_scale, float in_blend_bias)
{
	vec2 tex_size = textureSize(in_sampler, 0);
	vec2 scaled_tex_size = in_scale * global.FinalViewportSize.xy;
	float mipmap_lod = log2(tex_size.y / scaled_tex_size.y);
	return textureLod(in_sampler, in_coord, mipmap_lod + in_blend_bias);
}

// Texture Sampler function which takes a coordinate in the cropped coordinate space
vec4 HSM_GetCroppedTexSample(sampler2D in_sampler, vec2 in_screen_coord)
{
	return HSM_GetTexSampleFromSampleStartAndSize(in_sampler, in_screen_coord, SAMPLE_AREA_START_PIXEL_COORD, CROPPED_ROTATED_SIZE);
}

float HSM_GetVignetteFactor(vec2 coord, float amount, float size)
{
	float orig_mamehlsl_amount = amount;
	vec2 ctr_coord = coord - 0.5;

	float vignette_length = length(ctr_coord * vec2(0.5 / size * global.OutputSize.x/global.OutputSize.y + 0.5, 1));
	float vignette_blur = (orig_mamehlsl_amount * 0.75) + 0.25;

	// 0.5 full screen fitting circle
	float vignette_radius = 1.0 - (orig_mamehlsl_amount * 0.25);
	float vignette = smoothstep(vignette_radius, vignette_radius - vignette_blur, vignette_length);

	float vignette_multiplier = smoothstep(0, 0.05, amount);
	return 1 - vignette_multiplier + vignette * vignette_multiplier;
}

float HSM_GetScreenVignetteFactor(vec2 in_coord)
{
	vec2 vpos = HSM_GetMirrorWrappedCoord(in_coord);
	vpos = (vpos - 0.5) / 1.01 + 0.5;
	vpos *= 1.0 - vpos.xy;
    float vig = vpos.x * vpos.y * HSM_SCREEN_VIGNETTE_STRENGTH;
    vig = min(pow(vig, HSM_SCREEN_VIGNETTE_POWER), 1.0);

	return vig;
}

bool HSM_GetUseOnCurrentScreenIndex(float dual_screen_vis_mode)
{
	return dual_screen_vis_mode == SHOW_ON_DUALSCREEN_MODE_BOTH || dual_screen_vis_mode == SCREEN_INDEX;
}

bool HSM_GetUseScreenVignette()
{
	return HSM_SCREEN_VIGNETTE_ON > 0.5 && HSM_GetUseOnCurrentScreenIndex(HSM_SCREEN_VIGNETTE_DUALSCREEN_VIS_MODE);
}

vec4 HSM_GetNightLightingMultiplyColor( vec2 in_coord, float hue, float saturation, float value, float contrast, float global_ambient_opacity, in sampler2D NightLightingImage )
{
	vec4 lighting_image = vec4(0);
	// if (HSM_AMBIENT1_DITHERING_SAMPLES > 0.5)
	// {
	// 	// Dithering if needed to reduce banding
	// 	float blur_max_size = 1;
	// 	float blur_amount = 0.2;
	// 	lighting_image = HSM_GetStoichaicBlurredSample(NightLightingImage, in_coord.xy, HSM_AMBIENT1_DITHERING_SAMPLES, blur_max_size, blur_amount);
	// }
	// else
	lighting_image = HSM_GetMipmappedTexSample(NightLightingImage, in_coord.xy, vec2(1), 0);
	lighting_image = HSM_Linearize(lighting_image, DEFAULT_SRGB_GAMMA);

	lighting_image = contrast * (lighting_image - 0.5) + 0.5;

	// Do HSV alterations on the night lighting image
	if (hue != 0 || saturation != 1 || value != 1)
	{
		vec3 night_lighting_image_hsv = HSM_RGBtoHSV(lighting_image.rgb);
		night_lighting_image_hsv.x += hue;
		night_lighting_image_hsv.y *= saturation;
		night_lighting_image_hsv.z *= value;
		lighting_image = vec4(HSM_HSVtoRGB(night_lighting_image_hsv), lighting_image.a);
	}

	lighting_image.rgb = mix( vec3(1), lighting_image.rgb, global_ambient_opacity );

	return lighting_image;
}

bool HSM_Fill_Ambient_Images(vec2 in_viewport_coord, vec2 in_viewport_unscaled_coord, vec2 in_tube_coord, vec2 in_tube_scale, float in_swap_images, in sampler2D in_ambient_sampler, in sampler2D in_ambient2_sampler, inout vec4 ambient_lighting_image, inout vec4 ambient2_lighting_image)
{
		ambient_lighting_image = vec4(1);
		ambient2_lighting_image = vec4(1);

		if (HSM_AMBIENT1_OPACITY > 0)
		{
			float ambient1_scale = HSM_AMBIENT1_SCALE;
			if (HSM_AMBIENT1_SCALE_INHERIT_MODE == 1 && HSM_AMBIENT1_SCALE * HSM_VIEWPORT_ZOOM < 1)
				ambient1_scale = 1 / HSM_VIEWPORT_ZOOM;

			vec2 lighting_coord = GetSimpleImageScaledCoord(in_viewport_coord, 
															in_viewport_unscaled_coord,
															in_tube_coord,
															in_tube_scale,
															in_ambient_sampler,
															vec2(HSM_AMBIENT1_POSITION_X, HSM_AMBIENT1_POSITION_Y),
															HSM_AMBIENT1_POS_INHERIT_MODE, 
															vec2(ambient1_scale * HSM_AMBIENT1_SCALE_X, ambient1_scale), 
															HSM_AMBIENT1_SCALE_INHERIT_MODE, 
															HSM_AMBIENT1_SCALE_KEEP_ASPECT,
															HSM_AMBIENT1_MIRROR_HORZ,
															HSM_AMBIENT1_ROTATE );

			ambient_lighting_image = HSM_GetNightLightingMultiplyColor( lighting_coord,
																		HSM_AMBIENT1_HUE,
																		HSM_AMBIENT1_SATURATION,
																		HSM_AMBIENT1_VALUE,
																		HSM_AMBIENT1_CONTRAST,
																		HSM_AMBIENT1_OPACITY,
																		in_ambient_sampler );
		}

		if (HSM_AMBIENT2_OPACITY > 0)
		{
			float ambient2_scale = HSM_AMBIENT2_SCALE;
			if (HSM_AMBIENT2_SCALE_INHERIT_MODE == 1 && HSM_AMBIENT2_SCALE * HSM_VIEWPORT_ZOOM < 1)
				ambient2_scale = 1 / HSM_VIEWPORT_ZOOM;

			vec2 lighting2_coord = GetSimpleImageScaledCoord(in_viewport_coord, 
															in_viewport_unscaled_coord,
															in_tube_coord,
															in_tube_scale,
															in_ambient2_sampler,
															vec2(HSM_AMBIENT2_POSITION_X, HSM_AMBIENT2_POSITION_Y),
															HSM_AMBIENT2_POS_INHERIT_MODE, 
															vec2(ambient2_scale * HSM_AMBIENT2_SCALE_X, ambient2_scale), 
															HSM_AMBIENT2_SCALE_INHERIT_MODE, 
															HSM_AMBIENT2_SCALE_KEEP_ASPECT,
															HSM_AMBIENT2_MIRROR_HORZ,
															HSM_AMBIENT2_ROTATE );

			ambient2_lighting_image = HSM_GetNightLightingMultiplyColor( lighting2_coord,
																		HSM_AMBIENT2_HUE,
																		HSM_AMBIENT2_SATURATION,
																		HSM_AMBIENT2_VALUE,
																		HSM_AMBIENT2_CONTRAST,
																		HSM_AMBIENT2_OPACITY,
																		in_ambient2_sampler );
		}

	// if (in_swap_images == 1)
	// {
	// 	vec4 temp_image = ambient_lighting_image;
	// 	ambient_lighting_image = ambient2_lighting_image;
	// 	ambient2_lighting_image = temp_image;
	// }

	return true;
}

vec3 ApplyAmbientImages(vec3 base_image, vec3 ambient_image, vec3 ambient2_image, float blend_ambient, float blend_ambient2, float apply_in_add_mode, float layer_blend_mode, float swap_images) 
{
	vec3 outImage = base_image;

	if (swap_images == 1)
		ambient2_image = ambient_image;
	if (swap_images == 2)
		ambient_image = ambient2_image;
	if (swap_images == 3)
	{
		vec3 temp_image = ambient_image;
		ambient_image = ambient2_image;
		ambient2_image = temp_image;
	}

	if ( (HSM_AMBIENT1_OPACITY > 0 || HSM_AMBIENT2_OPACITY > 0) && (blend_ambient > 0 || blend_ambient2 > 0) )
	{
		if(	apply_in_add_mode == 1 || layer_blend_mode != BLEND_MODE_ADD)
		{
			if (blend_ambient > 0)
			{
				outImage = (1 - blend_ambient) * outImage.rgb + blend_ambient * outImage.rgb * ambient_image.rgb;
			}
			if (blend_ambient2 > 0)
			{
				outImage = (1 - blend_ambient2) * outImage.rgb + blend_ambient2 * outImage.rgb * ambient2_image.rgb;
			}
		}
	}

	return outImage;
}

vec4 HSM_ApplyMonochrome(vec4 in_color)
{
	vec4 out_color = in_color;
	out_color.rgb = pow(out_color.rgb, HSM_MONOCHROME_GAMMA.xxx);
	float luma = dot(out_color.rgb, vec3(0.299, 0.587, 0.114));
	luma *= HSM_MONOCHROME_BRIGHTNESS;
	vec3 mcolor = vec3(1.0);

	if (HSM_MONOCHROME_MODE > 1.5) mcolor = (HSM_MONOCHROME_MODE > 2.5) ? vec3(0.2549, 1.0, 0.0) : vec3(1.0, 0.749, 0.0);

	if (HSM_MONOCHROME_HUE_OFFSET != 0 || HSM_MONOCHROME_SATURATION != 0)
	{
		vec3 mcolor_hsv = HSM_RGBtoHSV(mcolor);
		mcolor_hsv.x += HSM_MONOCHROME_HUE_OFFSET;
		mcolor_hsv.y *= HSM_MONOCHROME_SATURATION;
		mcolor = HSM_HSVtoRGB(mcolor_hsv);
	}

	out_color.rgb = pow(luma, 1.0/HSM_MONOCHROME_GAMMA) * mcolor;
	return out_color;
}

float HSM_GetTubeOpacity()
{
	float tube_diffuse_opacity = HSM_TUBE_DIFFUSE_MODE < 1.5 ? HSM_TUBE_OPACITY : 0;

	// If CRT Blend Mode is Multiply (2) then the tube must be fully opaque
	if (HSM_CRT_BLEND_MODE == 2)
		tube_diffuse_opacity = 1;

	return tube_diffuse_opacity;
}

vec3 HSM_ApplyAmbientImage(vec3 base_image, vec3 ambient_image, float layer_blend_amount) 
{
	if (layer_blend_amount > 0)
		return (1 - layer_blend_amount) * base_image.rgb + layer_blend_amount * base_image.rgb * ambient_image.rgb;
	else
		return base_image;
}

bool HSM_GetUseFakeScanlines()
{
	float scane_axis_core_res = USE_VERTICAL_SCANLINES * CROPPED_ROTATED_SIZE_WITH_RES_MULT.x + (1 - USE_VERTICAL_SCANLINES) * CROPPED_ROTATED_SIZE_WITH_RES_MULT.y;
	return HSM_FAKE_SCANLINE_OPACITY > 0.001 && (HSM_FAKE_SCANLINE_MODE == 1 || (HSM_FAKE_SCANLINE_MODE == 2 && scane_axis_core_res > HSM_INTERLACE_TRIGGER_RES));
}

float HSM_GetNoScanlineMode()
{
	// If Use Fake Scanlines is On then turn off Guest Scanlines as they will just look bad
	float scan_res = (1 - USE_VERTICAL_SCANLINES) * CROPPED_ROTATED_SIZE_WITH_RES_MULT.y + 
						USE_VERTICAL_SCANLINES *  CROPPED_ROTATED_SIZE_WITH_RES_MULT.x;

	return global.no_scanlines > 0.5 || HSM_GetUseFakeScanlines() || HSM_INTERLACE_MODE < 0 && HSM_INTERLACE_TRIGGER_RES <= scan_res ? 1 : 0;
}

vec4 HSM_ApplyScanlineMask(vec4 in_color, vec2 screen_scale, vec2 in_coord, vec2 in_screen_curved_coord, vec2 in_tube_curved_coord, float in_scanline_opacity)
{
	//   Stuff to try implementing
    //   Try mame hlsl darkening
    //   Check Lottes tone mapping

	in_coord = mix(in_coord, in_screen_curved_coord, HSM_FAKE_SCANLINE_CURVATURE);

    /* Scanlines */
	float scanline_roll_offset = float(mod(global.FrameCount, 1280)) / 1280 * HSM_FAKE_SCANLINE_ROLL; 

    // float scans = clamp( 0.35+0.18*sin(6.0*time-curved_uv.y*resolution.y*1.5), 0.0, 1.0);
    // float s = pow(scans,0.9);
    // col = col * vec3(s);

	float scan_axis_pos = USE_VERTICAL_SCANLINES > 0.5 ? in_coord.x : in_coord.y;
	scan_axis_pos += scanline_roll_offset;

	vec2 screen_size = global.OutputSize.xy * screen_scale;
	float scan_axis_screen_scale_res = USE_VERTICAL_SCANLINES > 0.5 ? screen_size.x : screen_size.y;
	
	float cropped_rotated_scan_res = USE_VERTICAL_SCANLINES > 0.5 ? CROPPED_ROTATED_SIZE.x : CROPPED_ROTATED_SIZE.y;
	float simulated_scanline_res = HSM_FAKE_SCANLINE_RES_MODE > 0.5 ? HSM_FAKE_SCANLINE_RES : cropped_rotated_scan_res;

	float scanline_size = scan_axis_screen_scale_res / simulated_scanline_res;

	if (HSM_FAKE_SCANLINE_INT_SCALE == 1)
		scanline_size = ceil(scanline_size);

	float scan = mod(scan_axis_pos * scan_axis_screen_scale_res + scanline_size, scanline_size) / scanline_size;

	// Alternate, modulating the scanline width depending on brightness
	//float scanline_mask = HHLP_EasePowerOut(1 - abs(scan - 0.5) * 2, 0.5 + 2 * smoothstep(0.4, 0.9, (in_color.r + in_color.g +  in_color.b) / 3));
	float color_brightness_modulation = HHLP_EasePowerOut(smoothstep(0.4, 0.99, (in_color.r + in_color.g +  in_color.b) / 3), 2);

	float scanline_mask = 1 - abs(scan - 0.5) * 2;
	scanline_mask = pow(1 - scanline_mask, 1);

	float final_scanline_mask = clamp(1 * scanline_mask, 0, 1);

	color_brightness_modulation = HHLP_EasePowerOut(smoothstep(0.4, HSM_FAKE_SCANLINE_BRIGHTNESS_CUTOFF + 1.5, (in_color.r + in_color.g +  in_color.b) / 3), 2);
	final_scanline_mask = clamp(mix(1, mix(final_scanline_mask, 1, color_brightness_modulation), in_scanline_opacity), 0, 1);

	vec4 masked_color = in_color;
	masked_color *= 1 + 0.5 * in_scanline_opacity;
	masked_color = clamp(final_scanline_mask * masked_color, 0, 1);
	masked_color.w = in_color.w;

	// Darken the outside image a bit
	vec4 crt_darkened_color = mix(in_color, in_color * 0.9, HSM_FAKE_SCANLINE_OPACITY);

	// Show scanlines only in the tube area
	float softened_tube_mask = HSM_GetCornerMask((in_tube_curved_coord - 0.5) * 0.995 + 0.5 , TUBE_DIFFUSE_ASPECT, HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS, 0.05);
	vec4 out_color = mix(crt_darkened_color, masked_color, softened_tube_mask);

	return clamp(out_color, 0, 1);
}

vec4 HSM_Apply_Sinden_Lightgun_Border(vec4 in_rgba, vec2 in_tube_diffuse_curved_coord, float in_tube_diffuse_mask, float in_black_edge_corner_radius)
{
	float sinden_gun_mask =  in_tube_diffuse_mask - HSM_GetCornerMask((in_tube_diffuse_curved_coord - 0.5) * (1 + vec2(1 / TUBE_DIFFUSE_ASPECT * HSM_SINDEN_BORDER_THICKNESS, HSM_SINDEN_BORDER_THICKNESS)) + 0.5, TUBE_DIFFUSE_ASPECT, in_black_edge_corner_radius, 0.99);
	vec4 out_rgba = in_rgba;

	vec3 base_rgb = vec3(1);
	out_rgba.rgb = base_rgb * HSM_SINDEN_BORDER_BRIGHTNESS * sinden_gun_mask + (1 - sinden_gun_mask) * out_rgba.rgb;

	// out_rgba.rgb += HSM_SINDEN_BORDER_OPACITY * sinden_gun_mask;
	return out_rgba;
}

vec2 HSM_ApplyOverscan(vec2 in_coord, float overscan_x, float overscan_y)
{
	vec2 ctr_coord = in_coord * 2.0 - 1.0;

	ctr_coord /= vec2(overscan_x, overscan_y) + 1;

	return ctr_coord * 0.5 + 0.5;
}

float HSM_GetRasterBloomScale(float raster_bloom_overscan_mode, float raster_bloom_mult, float screen_avg_luma)
{
	// Define the neutral range around the center
	float neutral_range = HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE;

	// Calculate the bloom scale based on the neutral range
	float bloom_scale = 1.0;
	float neutral_center = HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE_CENTER;
	float neutral_min = neutral_center - neutral_range;
	float neutral_max = neutral_center + neutral_range;
	float adjusted_luma = neutral_center;
	if (screen_avg_luma < 0.5 - neutral_range || screen_avg_luma > 0.5 + neutral_range) 
	{
		if (screen_avg_luma < neutral_min) 
			adjusted_luma = neutral_center * HHLP_EasePowerIn(smoothstep(0, neutral_min, screen_avg_luma), 2);

		if (screen_avg_luma > neutral_max) 
			adjusted_luma = neutral_center + (1 - neutral_center) * HHLP_EasePowerOut(smoothstep(neutral_max, 1, screen_avg_luma), 2);
	}
	bloom_scale = 1.00 + raster_bloom_mult * ((1.0 - 0.5 * raster_bloom_overscan_mode) - adjusted_luma);
	return bloom_scale;
}

// Thanks to Guest.r for this simple yet effective implementation of raster bloom
vec2 HSM_ApplyRasterBloomOverscan(vec2 in_coord, float raster_bloom_overscan_mode, float raster_bloom_mult, float screen_avg_luma)
{
	//raster_bloom_overscan_mode can be 0, 1, or 2 to adjust if the bloom can only expand or shrink

	vec2 ctr_coord = in_coord * 2.0 - 1.0;

	float raster_bloom_factor = HSM_GetRasterBloomScale(raster_bloom_overscan_mode, raster_bloom_mult, screen_avg_luma);
	ctr_coord *= vec2(raster_bloom_factor, raster_bloom_factor);

	return ctr_coord * 0.5 + 0.5;
}

vec2 HSM_GetCrtShaderFinalCoord(vec2 screen_coord,
								vec2 screen_scale,
								float raster_bloom_avg_lum,
								inout vec2 screen_curved_coord
								)

{
	// -- START -- This section repeated in function below
	screen_curved_coord = HSM_GetCRTShaderCurvedCoord(screen_coord);

	// Get the Mirrored coord and then add overscan to it
	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetMirrorWrappedCoord(screen_curved_coord);
	screen_curved_coord_with_overscan_and_mirror = HSM_ApplyOverscan(screen_curved_coord_with_overscan_and_mirror, HSM_OVERSCAN_X, HSM_OVERSCAN_Y);

	if (HSM_OVERSCAN_RASTER_BLOOM_ON > 0.5)
	{
		screen_curved_coord_with_overscan_and_mirror = HSM_ApplyRasterBloomOverscan(screen_curved_coord_with_overscan_and_mirror, 
			HSM_OVERSCAN_RASTER_BLOOM_MODE, 
			HSM_OVERSCAN_RASTER_BLOOM_AMOUNT, 
			raster_bloom_avg_lum);
	}
	// -- END -- Repeated Section

	return screen_curved_coord_with_overscan_and_mirror;
}

vec2 HSM_GetCrtShaderFinalCoordExtraVariables(vec2 screen_coord,
	vec2 screen_scale,
	float raster_bloom_avg_lum,
	inout vec2 screen_curved_coord,
	inout vec2 screen_curved_coord_with_overscan,
	inout vec2 screen_coord_with_overscan,
	inout vec2 screen_scale_with_overscan)

{
	// -- START -- This section repeated from function above
	screen_curved_coord = HSM_GetCRTShaderCurvedCoord(screen_coord);

	// Get the Mirrored coord and then add overscan to it
	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetMirrorWrappedCoord(screen_curved_coord);
	screen_curved_coord_with_overscan_and_mirror = HSM_ApplyOverscan(screen_curved_coord_with_overscan_and_mirror, HSM_OVERSCAN_X, HSM_OVERSCAN_Y);

	if (HSM_OVERSCAN_RASTER_BLOOM_ON > 0.5)
	{
		screen_curved_coord_with_overscan_and_mirror = HSM_ApplyRasterBloomOverscan(screen_curved_coord_with_overscan_and_mirror, 
			HSM_OVERSCAN_RASTER_BLOOM_MODE, 
			HSM_OVERSCAN_RASTER_BLOOM_AMOUNT, 
			raster_bloom_avg_lum);
	}
	// -- END -- Repeated Section

	// Screen Coordinate overscan but without curvature 
	// Screen Curved Coordinate but not Mirrored
	screen_curved_coord_with_overscan = HSM_ApplyOverscan(screen_curved_coord, HSM_OVERSCAN_X, HSM_OVERSCAN_Y);
	screen_scale_with_overscan = screen_scale * (vec2(HSM_OVERSCAN_X, HSM_OVERSCAN_Y) + 1);

	if (HSM_OVERSCAN_RASTER_BLOOM_ON > 0.5)
	{
		screen_curved_coord_with_overscan = HSM_ApplyRasterBloomOverscan(screen_curved_coord_with_overscan, 
			HSM_OVERSCAN_RASTER_BLOOM_MODE, 
			HSM_OVERSCAN_RASTER_BLOOM_AMOUNT, 
			raster_bloom_avg_lum);

		screen_coord_with_overscan = HSM_ApplyRasterBloomOverscan(screen_coord_with_overscan, 
			HSM_OVERSCAN_RASTER_BLOOM_MODE, 
			HSM_OVERSCAN_RASTER_BLOOM_AMOUNT, 
			raster_bloom_avg_lum);

		screen_scale_with_overscan *= HSM_GetRasterBloomScale(HSM_OVERSCAN_RASTER_BLOOM_MODE, 
			HSM_OVERSCAN_RASTER_BLOOM_AMOUNT, 
			raster_bloom_avg_lum);
	}

	screen_coord_with_overscan = HSM_ApplyOverscan(screen_coord, HSM_OVERSCAN_X, HSM_OVERSCAN_Y);

	return screen_curved_coord_with_overscan_and_mirror;
}