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

/////////////// IMPORTS ///////////////
#include "common/common-functions-bezel.inc"

// Removed because BackgroundImageSize returns vec4(0,0,0,0), this happens with all user defined textures
// layout(push_constant) uniform Push
// {
// 	vec4 BackgroundImageSize;
// } params;

/////////////// DEFINES ///////////////

#define MASK_MODE_ALL 0
#define MASK_MODE_SCREEN 1
#define MASK_MODE_TUBE 2
#define MASK_MODE_INSIDE_BEZEL 3
#define MASK_MODE_BEZEL 4
#define MASK_MODE_OUTSIDE_TUBE 5
#define MASK_MODE_FRAME 6
#define MASK_MODE_OUTSIDE_BEZEL 7
#define MASK_MODE_OUTSIDE_FRAME 8

#define CUTOUT_MODE_OFF 0
#define CUTOUT_MODE_INSIDE 1
#define CUTOUT_MODE_OUTSIDE 2

float FILL_MODE_KEEP_TEXTURE_ASPECT = 0;
float FILL_MODE_SPLIT = 1;
float FILL_MODE_STRETCH = 2;

float USE_INHERITED_COORD_OFF = 0;
float USE_INHERITED_COORD_ON = 1;

vec2 VIEWPORT_COORD = vec2(0.5);

/////////////// Helper Functions ///////////////

// Return the mask for the specific mode
float GetMask(float mask_mode)
{
	float mask = 	(mask_mode == MASK_MODE_ALL) ? 1 :
					(mask_mode == MASK_MODE_SCREEN) ? TUBE_DIFFUSE_MASK :
					(mask_mode == MASK_MODE_TUBE) ? TUBE_MASK :
					(mask_mode == MASK_MODE_INSIDE_BEZEL) ? INSIDE_BEZEL_MASK :
					(mask_mode == MASK_MODE_BEZEL) ? BEZEL_MASK :
					(mask_mode == MASK_MODE_OUTSIDE_TUBE) ? OUTSIDE_TUBE_MASK_FOR_IMAGE :
					(mask_mode == MASK_MODE_FRAME) ? FRAME_MASK :
					(mask_mode == MASK_MODE_OUTSIDE_BEZEL) ? OUTSIDE_BEZEL_MASK :
					(mask_mode == MASK_MODE_OUTSIDE_FRAME) ? OUTSIDE_FRAME_MASK : 0.5;
	return mask;
}

// Assumes Opacity is already encoded in alpha
vec4 BlendModeMaskLayerMix(vec4 color_under, vec4 color_over, float blend_mode, float mask_mode, float cutout_mode, float dualscreen_mode, float layer_opacity)
{
	if ( blend_mode == 0 || (dualscreen_mode != SHOW_ON_DUALSCREEN_MODE_BOTH && dualscreen_mode != SCREEN_INDEX) )
		return color_under;
	
	float cutout_mask = 1;
	if (cutout_mode == CUTOUT_MODE_INSIDE)
		cutout_mask = CUTOUT_MASK;
	if (cutout_mode == CUTOUT_MODE_OUTSIDE)
		cutout_mask = 1 - CUTOUT_MASK;

	if (blend_mode == BLEND_MODE_OFF)
		return color_under;
	
	color_over.a *= layer_opacity * GetMask(mask_mode) * cutout_mask;

	vec4 out_color = vec4(0);

	if (blend_mode == BLEND_MODE_NORMAL)
	{
		color_over.rgb *= color_over.a;
		out_color = HSM_PreMultAlphaBlend(color_under, color_over);
	}
	else
	{
		vec4 blend_color = color_under; 
		if (blend_mode == BLEND_MODE_ADD)  	 		blend_color.rgb = color_under.rgb + color_over.rgb ;
		if (blend_mode == BLEND_MODE_MULTIPLY)  	blend_color.rgb = color_under.rgb * color_over.rgb ;

		out_color = vec4(clamp(mix(color_under.rgb, blend_color.rgb, color_over.a), 0, 1), color_under.a);
	}
	return out_color;
}

vec2 HSM_GetLayerCoord(vec2 in_viewport_coord, float layer_to_follow, float follow_mode, inout vec2 out_placement_scale)
{
	vec2 flat_coord = vec2(0.5);
	vec2 curved_coord = vec2(0.5);
	vec2 out_coord = vec2(0.5);

	if (layer_to_follow == FOLLOW_LAYER_VIEWPORT)
	{
		flat_coord = in_viewport_coord;
		curved_coord = in_viewport_coord;
	}
	else if (layer_to_follow == FOLLOW_LAYER_TUBE_DIFFUSE)
	{
		flat_coord = TUBE_DIFFUSE_COORD;
		curved_coord = TUBE_DIFFUSE_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_BEZEL_OUTSIDE)
	{
		flat_coord = BEZEL_OUTSIDE_COORD;
		curved_coord = BEZEL_OUTSIDE_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_BG)
	{
		flat_coord = BACKGROUND_COORD;
		curved_coord = BACKGROUND_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_DEVICE)
	{
		flat_coord = DEVICE_COORD;
		curved_coord = DEVICE_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_DECAL)
	{
		flat_coord = DECAL_COORD;
		curved_coord = DECAL_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_CAB_GLASS)
	{
		flat_coord = CAB_GLASS_COORD;
		curved_coord = CAB_GLASS_CURVED_COORD;
	}
	else if (layer_to_follow == FOLLOW_LAYER_TOP)
	{
		flat_coord = TOP_IMAGE_COORD;
		curved_coord = TOP_IMAGE_CURVED_COORD;
	}

	out_coord = follow_mode == FOLLOW_MODE_EXACT ? curved_coord : flat_coord;

	return  out_coord;
}

// Takes a viewport coordinate and gives a new coordinate scaled by the specific scale mode
// Takes into account the default sizes of each scale mode
vec2 HSM_GetScaledCoord(vec2 in_viewport_coord,
						vec2 in_viewport_coord_unscaled,
						float texture_aspect_mode,
						float explicit_texture_aspect,
						vec2 offset_pos,
						vec2 offset_scale,
						float layer_to_follow,
						float follow_mode,
						float scale_full_with_zoom,
						float image_fill_mode,
						float split_preserve_center,
						float split_repeat_width,
						bool apply_default_scale_offset,
						// float image_rotation_mode,
						// float explicit_image_rotation,
						inout vec2 out_placement_coord,
						inout vec2 out_placement_scale)
{
	float texture_aspect = HSM_GetAspectRatioFromMode(texture_aspect_mode, explicit_texture_aspect);

	/*
	bool is_rotated = false;
	// float image_rotation_mode = 1; // Modes, 0: explicit, 1: Rotate To Vertical Core Aspect, 2: Rotate To Vertical Viewport Aspect
	// // vert core aspect rotation, vert vertical aspect rotation
	// // rotate with tube ?
	float explicit_image_rotation = 0;
	float image_rotation = explicit_image_rotation;

	// if (image_rotation_mode == 1.0)
	// 	image_rotation = SCREEN_ASPECT < 1 ? 1 : 0;
	
	if (min(abs(image_rotation), 1) > 0.5)
	{
		texture_aspect = 1 / texture_aspect;
		offset_pos = vec2(offset_pos.y, -offset_pos.x);
		offset_scale = offset_scale.yx;
		is_rotated = true;
	}
	*/

	vec2 inherited_coord = in_viewport_coord / 0.5;
	vec2 inherited_placement_coord = in_viewport_coord / 0.5;
	vec2 inherited_final_coord = in_viewport_coord / 0.5;
	vec2 inherited_scale = vec2(0.5);
	vec2 default_offset_scale = vec2(0.5);

	if (layer_to_follow == FOLLOW_LAYER_VIEWPORT)
	{
		if (scale_full_with_zoom > 0.5)	
		{
			inherited_coord = in_viewport_coord;
			inherited_placement_coord = in_viewport_coord;
		}
		else
		{
			inherited_coord = in_viewport_coord_unscaled;
			inherited_placement_coord = in_viewport_coord_unscaled;
		}

		inherited_final_coord = inherited_coord;

		inherited_scale = vec2(1, 1);
		default_offset_scale = vec2(1);
	}
	else if (layer_to_follow == FOLLOW_LAYER_TUBE_DIFFUSE)
	{
/*
		if (is_rotated)
		{
			inherited_coord = TUBE_DIFFUSE_COORD;
			inherited_placement_coord = TUBE_DIFFUSE_COORD;
			inherited_final_coord = TUBE_DIFFUSE_CURVED_COORD;

			inherited_scale = TUBE_DIFFUSE_SCALE;

			default_offset_scale = vec2(1) / DEFAULT_UNCORRECTED_SCREEN_SCALE.y;
			default_offset_scale.x *= texture_aspect / (1 / DEFAULT_SCREEN_ASPECT);
		}
		else
		{
*/
			inherited_coord = TUBE_DIFFUSE_COORD;
			inherited_placement_coord = TUBE_DIFFUSE_COORD;
			inherited_final_coord = TUBE_DIFFUSE_CURVED_COORD;

			inherited_scale = TUBE_DIFFUSE_SCALE;

			default_offset_scale = vec2(1) / (DEFAULT_UNCORRECTED_SCREEN_SCALE.y);
			default_offset_scale.x *= texture_aspect / DEFAULT_SCREEN_ASPECT;
		// }

	}
	else if (layer_to_follow == FOLLOW_LAYER_BEZEL_OUTSIDE)
	{
/*
		if (is_rotated)
		{
			inherited_coord = BEZEL_OUTSIDE_COORD;
			inherited_placement_coord = BEZEL_OUTSIDE_COORD;

			inherited_scale = BEZEL_OUTSIDE_SCALE;
			inherited_final_coord = BEZEL_OUTSIDE_CURVED_COORD;

			default_offset_scale = vec2(1) / DEFAULT_UNCORRECTED_BEZEL_SCALE.y;
			default_offset_scale.x *= texture_aspect / (1 / DEFAULT_BEZEL_ASPECT);
		}
		else
		{
*/
			inherited_coord = BEZEL_OUTSIDE_COORD;
			inherited_placement_coord = BEZEL_OUTSIDE_COORD;

			inherited_scale = BEZEL_OUTSIDE_SCALE;
			inherited_final_coord = BEZEL_OUTSIDE_CURVED_COORD;

			default_offset_scale = vec2(1) / (DEFAULT_UNCORRECTED_BEZEL_SCALE.y);
			default_offset_scale.x *= texture_aspect / DEFAULT_SCREEN_ASPECT;
		// }
	}
	else if (layer_to_follow == FOLLOW_LAYER_BG)
	{
		inherited_coord = BACKGROUND_COORD;
		inherited_placement_coord = BACKGROUND_COORD;
		inherited_final_coord = BACKGROUND_CURVED_COORD;

		inherited_scale = BACKGROUND_SCALE;
		default_offset_scale = vec2(1);
	}
	else if (layer_to_follow == FOLLOW_LAYER_DEVICE)
	{
		inherited_coord = DEVICE_COORD;
		inherited_placement_coord = DEVICE_COORD;
		inherited_final_coord = DEVICE_CURVED_COORD;

		inherited_scale = DEVICE_SCALE;
		default_offset_scale = vec2(1);
	}
	else if (layer_to_follow == FOLLOW_LAYER_DECAL)
	{
		inherited_coord = DECAL_COORD;
		inherited_placement_coord = DECAL_COORD;
		inherited_final_coord = DECAL_CURVED_COORD;

		inherited_scale = DECAL_SCALE;
		default_offset_scale = vec2(1);
	}

	//--------------------------------
	// RETURN if we want to use the exact same coordinate of the layer we follow
	//--------------------------------
	if (follow_mode == FOLLOW_MODE_EXACT)
	{
		out_placement_coord = HSM_AddPosScaleToCoord(inherited_placement_coord, offset_pos, offset_scale);
		out_placement_scale = inherited_scale * offset_scale;
		return HSM_AddPosScaleToCoord(inherited_final_coord, offset_pos, offset_scale);
	}

	if (apply_default_scale_offset)
	{
		offset_scale *= default_offset_scale;
	}

	float output_aspect = global.OutputSize.x / global.OutputSize.y;
	float inherited_aspect = (inherited_scale.x / inherited_scale.y) * (default_offset_scale.x / default_offset_scale.y) * output_aspect;
	
	// Find the aspect difference so the image can be shown without distortion
	// This is before the user edited scale offset
	float inherited_aspect_difference = texture_aspect / inherited_aspect;

	// Get the overall scale for the placement of the texture (No Split/Fill Yet)
	out_placement_scale = inherited_scale;
	if ( image_fill_mode == FILL_MODE_KEEP_TEXTURE_ASPECT )
		out_placement_scale.x *= inherited_aspect_difference;
	out_placement_scale = out_placement_scale * offset_scale;

	// inherited_coord = out_placement_coord;
	out_placement_coord = HSM_AddPosScaleToCoord(inherited_placement_coord, offset_pos, out_placement_scale / inherited_scale);

	vec2 out_coord = vec2(0.5);

	vec2 drawing_scale = out_placement_scale;
	float slide_x = 0;

	if ( image_fill_mode == FILL_MODE_SPLIT )
	{
		float abs_ctr_coord_x = abs(out_placement_coord.x - 0.5);
		// Correct the aspect so it matches the texture and is never stretched
		float placement_aspect = out_placement_scale.x / out_placement_scale.y * output_aspect;
		float placement_aspect_difference = texture_aspect / placement_aspect;
		drawing_scale.x *= placement_aspect_difference;

		float center_width = split_preserve_center * placement_aspect_difference;
		if ( abs_ctr_coord_x > center_width)
		{
			slide_x = ((placement_aspect - texture_aspect) / placement_aspect) / 2.0;
		}

		float repeat_width = split_repeat_width * placement_aspect_difference;
		if (abs_ctr_coord_x > center_width && 
			abs_ctr_coord_x < center_width + slide_x && 
			repeat_width > 0)
		{
			if (clamp(split_repeat_width - 0.001, 0, 1) == 0)
				slide_x = (abs_ctr_coord_x - center_width);
			else
				slide_x = (abs_ctr_coord_x - 0.001 - center_width) - mod(clamp(abs_ctr_coord_x - 0.01 - center_width, 0, 1), repeat_width);
		}

		if ( out_placement_coord.x < 0.5 )
			slide_x *= -1;
		inherited_coord.x -= slide_x;
	}

	// The inherited_coord is already the coord from the inherited space
	// We only need to apply an offset from this
	out_coord = HSM_AddPosScaleToCoord(inherited_coord, offset_pos, drawing_scale / inherited_scale);

/*
	if (is_rotated)
		out_coord = HSM_RotateCoordinate(out_coord, image_rotation);
*/

	return out_coord;
}

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

	UNFLIPPED_VIEWPORT_COORD = vTexCoord;

	VIEWPORT_COORD = vTexCoord * 1.0001;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma stage fragment

layout(location = 6) in vec2 vTexCoord;
layout(location = 7) in vec2 UNFLIPPED_VIEWPORT_COORD;
layout(location = 8) in vec3 BEZEL_FRAME_ORIGINAL_COLOR_RGB;

layout(location = 0) out vec4 FragColor;

// Pass Framebuffer Textures
layout(set = 0, binding = 1)  uniform sampler2D InfoCachePass;

layout(set = 0, binding = 2)  uniform sampler2D BackgroundImage;
layout(set = 0, binding = 3)  uniform sampler2D BackgroundVertImage;
layout(set = 0, binding = 4)  uniform sampler2D NightLightingImage;
layout(set = 0, binding = 5)  uniform sampler2D NightLighting2Image;

layout(set = 0, binding = 6)  uniform sampler2D LEDImage;
layout(set = 0, binding = 7)  uniform sampler2D FrameTextureImage;
layout(set = 0, binding = 8)  uniform sampler2D DeviceImage;
layout(set = 0, binding = 9) uniform sampler2D DeviceVertImage;
layout(set = 0, binding = 10) uniform sampler2D DeviceLEDImage;
layout(set = 0, binding = 11) uniform sampler2D DecalImage;
layout(set = 0, binding = 12) uniform sampler2D CabinetGlassImage;
layout(set = 0, binding = 13) uniform sampler2D TopLayerImage;
layout(set = 0, binding = 14) uniform sampler2D ReflectionMaskImage;

#ifdef LAYERS_OVER_CRT
layout(set = 0, binding = 15) uniform sampler2D BR_LayersOverCRTPassFeedback;
#define PassFeedback BR_LayersOverCRTPassFeedback
#else
layout(set = 0, binding = 15) uniform sampler2D BR_LayersUnderCRTPassFeedback;
#define PassFeedback BR_LayersUnderCRTPassFeedback
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
	if (HSM_AB_COMPARE_FREEZE_GRAPHICS == 1 && HSM_GetIsInABCompareArea(vTexCoord))
	{
		FragColor = texture(PassFeedback, vTexCoord);
		return;
	}

	VIEWPORT_UNSCALED_COORD = HSM_GetViewportCoordWithFlip(vTexCoord);
	VIEWPORT_COORD = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);

	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec4 feedback_color_test = texture(PassFeedback, vec2(0,0));
	if (HSM_CACHE_GRAPHICS_ON > 0.5 && feedback_color_test.a < 0 && !CACHE_INFO_CHANGED)
	{
		FragColor = texture(PassFeedback, UNFLIPPED_VIEWPORT_COORD);
		return;
	}

	// AMBIENT LIGHTING IMAGES
	vec4 ambient_image = vec4(1);
	vec4 ambient2_image = vec4(1);
	HSM_Fill_Ambient_Images(VIEWPORT_COORD, VIEWPORT_UNSCALED_COORD, TUBE_DIFFUSE_COORD, TUBE_DIFFUSE_SCALE, HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE, NightLightingImage, NightLighting2Image, ambient_image, ambient2_image);

	TUBE_DIFFUSE_CURVED_COORD = HSM_GetCurvedCoord(SCREEN_COORD, HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE, TUBE_DIFFUSE_ASPECT);

	// TODO this should probably just use TUBE_COORD
	// TODO should probably use TUBE_ASPECT
	vec2 tube_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, 1, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 1);
	vec2 tube_curved_coord_ctr = tube_curved_coord - 0.5;
	vec2 edge_mask_coord = tube_curved_coord_ctr * (1 - (HSM_BZL_INNER_EDGE_THICKNESS / vec2(TUBE_DIFFUSE_ASPECT, 1))) + 0.5;

	float bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	if(HSM_BZL_USE_INDEPENDENT_CURVATURE > 0)
		bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * DEFAULT_SCREEN_CORNER_RADIUS;
	
	float edge_mask =  HSM_GetCornerMask(edge_mask_coord, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, HSM_BZL_INNER_EDGE_SHARPNESS);

	TUBE_MASK = HSM_GetCornerMask(tube_curved_coord, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);

	// Shrink the mask by 0.001 to clip off outer edge
	TUBE_DIFFUSE_MASK = HSM_GetCornerMask(((TUBE_DIFFUSE_CURVED_COORD - 0.5) * 1.001) + 0.5, TUBE_DIFFUSE_ASPECT, HSM_GLOBAL_CORNER_RADIUS * HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE, HSM_TUBE_BLACK_EDGE_SHARPNESS);

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

	OUTSIDE_BEZEL_MASK = 1 - HSM_GetCornerMask(BEZEL_OUTSIDE_CURVED_COORD, TUBE_DIFFUSE_ASPECT, HSM_GLOBAL_CORNER_RADIUS * HSM_BZL_OUTER_CORNER_RADIUS_SCALE, 0.9);

	// Get color for the frame area outside of the bezel
	vec2 frame_outside_coord_ctr = FRAME_OUTSIDE_CURVED_COORD - 0.5;
	float SHADOW_OUTSIDE_FRAME_MASK = 1 - HSM_GetCornerMask(frame_outside_coord_ctr * 1.01 + 0.5, TUBE_DIFFUSE_ASPECT, HSM_FRM_OUTER_CORNER_RADIUS, 1);
	OUTSIDE_FRAME_MASK = 1 - HSM_GetCornerMask(frame_outside_coord_ctr + 0.5, TUBE_DIFFUSE_ASPECT, HSM_FRM_OUTER_CORNER_RADIUS, 1);
	OUTSIDE_FRAME_MASK_FOR_IMAGE = 1 - HSM_GetCornerMask(frame_outside_coord_ctr * 0.999 + 0.5, TUBE_DIFFUSE_ASPECT, HSM_FRM_OUTER_CORNER_RADIUS, 1);
	// Get masks for shadows, from frame as well as sides and top and bottom of viewport
	INSIDE_BEZEL_MASK = 1 - OUTSIDE_BEZEL_MASK;
	BEZEL_MASK = INSIDE_BEZEL_MASK * (1 - TUBE_MASK);
	FRAME_MASK = OUTSIDE_BEZEL_MASK * (1 - OUTSIDE_FRAME_MASK);

#ifdef LAYERS_UNDER_CRT
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
	float left_half_mask = smoothstep(0.55, 0.5, tube_curved_coord.x);

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
																										HSM_BZL_OUTER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS, 
																										0.9);
	float outside_tube_mask_wider = 1 - HSM_GetCornerMask(tube_curved_coord_ctr * 0.996 + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.9);
	float tube_shadow_mask = HSM_GetCornerMask(tube_curved_coord_ctr + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0);
	float tube_edge_shadow_mult = HSM_BZL_INNER_EDGE_SHADOW * (tube_shadow_mask) + (1 - HSM_BZL_INNER_EDGE_SHADOW);

	float edge_highlight_mask = 0;

	// ----------------------------------------------------
	// Generated Bezel
	// ----------------------------------------------------

	/* This first bit is to get a mapping of the space outside of the screen which is continuous
	This is more complicated than you would expect because since we are using curved coordinates 
	there are discontinuities outside the normal screen corners, e.g. where x > 1 and y > 1
	So instead of trying to use the coordinates from the screen/tube we use a larger space 
	and subtract the screen space to see how far we are outside of the sreen
	*/

	float hmbz_bezel_highlight_edge = 0.9;
	float hmbz_bezel_highlight_top = 0.2;
	float hmbz_bezel_highlight_bottom = 0.3;
	float hmbz_bezel_highlight_sides = 0.2;
	
	float hmbz_bezel_highlight_falloff_speed = 0.5;
	float hmbz_bezel_highlight_width = 0.25;

	float hmbz_bezel_edge_highlight_width = 0.8;
	
	float hmbz_bezel_brightness_frame_outer_edge = 0.0;
	float hmbz_brightness_shadow = 0;
	float hmbz_frame_brightness = 100;

	// Not sure why we need linearize this but it seems to have a smoother range this way
	vec3 base_color = HSM_Linearize(vec4(HSM_HSVtoRGB(vec3(HSM_BZL_COLOR_HUE, HSM_BZL_COLOR_SATURATION, HSM_BZL_COLOR_VALUE)), 1), DEFAULT_SRGB_GAMMA).rgb;
	float noise_mask = clamp(fract(sin(dot(tube_curved_coord_ctr + vec2(0.5, 0.5) + 1, vec2(12.9898, 78.233))) * 43758.5453), 0, 1);
	vec3 base_color_with_noise = mix(base_color, 1.5 * base_color * noise_mask, HSM_BZL_NOISE);
	vec3 top_color = HSM_BZL_BRIGHTNESS_MULT_TOP * HSM_BZL_BRIGHTNESS * base_color_with_noise;
	vec3 bottom_color = HSM_BZL_BRIGHTNESS_MULT_BOTTOM * HSM_BZL_BRIGHTNESS * base_color_with_noise;
	vec3 sides_color = mix(HSM_BZL_BRIGHTNESS_MULT_SIDES * HSM_BZL_BRIGHTNESS_MULT_SIDE_RIGHT * HSM_BZL_BRIGHTNESS * base_color_with_noise,
							HSM_BZL_BRIGHTNESS_MULT_SIDES * HSM_BZL_BRIGHTNESS_MULT_SIDE_LEFT * HSM_BZL_BRIGHTNESS * base_color_with_noise,
							left_half_mask);

	vec3 frame_base_color = base_color;
	vec3 frame_base_color_with_noise = base_color_with_noise;
	if (HSM_FRM_USE_INDEPENDENT_COLOR > 0)
	{
		frame_base_color = HSM_Linearize(vec4(HSM_HSVtoRGB(vec3(HSM_FRM_COLOR_HUE, HSM_FRM_COLOR_SATURATION, HSM_FRM_COLOR_VALUE)), 1), DEFAULT_SRGB_GAMMA).rgb;
		frame_base_color_with_noise = mix(frame_base_color, 1.5 * frame_base_color * noise_mask, HSM_FRM_NOISE);
	}

	vec3 frame_color = hmbz_frame_brightness / 100 * mix(frame_base_color, 1.5 * frame_base_color * noise_mask, 0.6 * HSM_FRM_NOISE);
	vec3 outside_frame_color = hmbz_brightness_shadow * base_color_with_noise;

	vec3 bezel_diffuse_color = mix(sides_color, top_color, top_mask);
	bezel_diffuse_color = mix(bezel_diffuse_color, bottom_color, bottom_mask);

	float top_center_highlight_mask 	= hmbz_bezel_highlight_top * top_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.5, hmbz_bezel_highlight_falloff_speed));
	float bottom_center_highlight_mask 	= hmbz_bezel_highlight_bottom * bottom_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.5, hmbz_bezel_highlight_falloff_speed));
	float sides_highlight_mask 			= hmbz_bezel_highlight_sides * sides_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_highlight_width, 0, abs(tube_curved_coord_ctr.y)), vec2(0.5, hmbz_bezel_highlight_falloff_speed));

	float edge_top_center_highlight_mask 		= hmbz_bezel_highlight_top * top_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 0));
	float edge_bottom_center_highlight_mask 	= hmbz_bezel_highlight_bottom * bottom_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.x)), vec2(0.8, 0));
	float edge_sides_highlight_mask 			= hmbz_bezel_highlight_sides * sides_mask * HHLP_QuadraticBezier(smoothstep(hmbz_bezel_edge_highlight_width, 0, abs(tube_curved_coord_ctr.y)), vec2(0.8, 0));

	edge_highlight_mask = hmbz_bezel_highlight_edge * edge_mask * (edge_top_center_highlight_mask + edge_bottom_center_highlight_mask + edge_sides_highlight_mask);
	edge_highlight_mask *= HSM_BZL_INNER_EDGE_HIGHLIGHT;

	// Combine all the individual highlights into one mask
	float combined_highlight_mask = (1 + 2.5 * HSM_BZL_NOISE) * (1 - noise_mask * 2.5 * HSM_BZL_NOISE) * (top_center_highlight_mask + bottom_center_highlight_mask + sides_highlight_mask);
	float bezel_highlight_multiplier = HSM_BZL_HIGHLIGHT * combined_highlight_mask + HSM_BZL_HIGHLIGHT * edge_highlight_mask;
	vec3 bezel_color = bezel_diffuse_color * (1 + 15 * bezel_highlight_multiplier) + 1 * bezel_highlight_multiplier;

	// Add the inner edge highlight on top of the bezel color which has it's own highlight
	float inner_edge_highlight_multiplier = HSM_FRM_INNER_EDGE_HIGHLIGHT + HSM_BZL_HIGHLIGHT * 10 * HSM_FRM_INNER_EDGE_HIGHLIGHT;
	vec3 frame_inner_edge_color = frame_base_color * (1 + 15 * inner_edge_highlight_multiplier) +	0.5 * inner_edge_highlight_multiplier;

	bezel_color = mix(bezel_color, frame_inner_edge_color, frame_inner_edge_mask);

	float dist_inside_outer_edge = min(0.50 - abs(frame_outside_coord_ctr.x), 0.50 - abs(frame_outside_coord_ctr.y));
	float frame_outer_edge_width = HSM_FRM_OUTER_EDGE_THICKNESS;
	vec3 frame_diffuse_color = mix(frame_color, 0.2 * frame_color, HSM_FRM_OUTER_EDGE_SHADING * smoothstep(frame_outer_edge_width, 0, dist_inside_outer_edge));

	if (HSM_FRM_TEXTURE_OPACITY > 0)
	{
		// TODO need to do Mipmapping sample?
		vec4 frame_texture_color = HSM_Linearize(texture(FrameTextureImage, FRAME_OUTSIDE_CURVED_COORD), DEFAULT_SRGB_GAMMA);
		frame_diffuse_color = HSM_BlendModeLayerMix(vec4(frame_diffuse_color, 1), frame_texture_color, HSM_FRM_TEXTURE_BLEND_MODE, HSM_FRM_TEXTURE_OPACITY).rgb;
	}

	// Composite in color from outside the bezel
	vec3 bezel_and_frame_rgb = mix(bezel_color, frame_diffuse_color, OUTSIDE_BEZEL_MASK);

	// Get masks on side of frame to multiply together to get a shadow around the frame
	// Get vector from the screen edge outward
	float frame_edge = 0.495;
	float dist_outside_frame = length(clamp(abs(frame_outside_coord_ctr * 1.01) - frame_edge, 0, 1) * vec2(TUBE_DIFFUSE_ASPECT, 1));

	vec4 frame_shadow_layer = vec4(0);
	if (HSM_FRM_OPACITY > 0.001)
		frame_shadow_layer.a = SHADOW_OUTSIDE_FRAME_MASK * HHLP_QuadraticBezier(smoothstep(HSM_FRM_SHADOW_WIDTH, 0, dist_outside_frame), vec2(1, 0));

	//----------------------------------------------------
	// Generated Bezel
	//----------------------------------------------------

	vec4 bezel_layer = vec4(0);
	vec4 frame_layer = vec4(0);

	if (HSM_BZL_OPACITY > 0 || HSM_FRM_OPACITY > 0)
	{
		// Create image of bezel & frame with outside of frame transparent
		vec4 bezel_and_frame_rgba = vec4(bezel_and_frame_rgb, 1);

		// Cut out Tube Area
		if (HSM_STATIC_LAYERS_GAMMA != 1)
			bezel_and_frame_rgba = HSM_ApplyGamma(bezel_and_frame_rgba, HSM_STATIC_LAYERS_GAMMA);

		bezel_and_frame_rgba.rgb = ApplyAmbientImages(bezel_and_frame_rgba.rgb,
															ambient_image.rgb,
															ambient2_image.rgb,
															HSM_BZL_AMBIENT_LIGHTING_MULTIPLIER,
															HSM_BZL_AMBIENT2_LIGHTING_MULTIPLIER,
															1,
															HSM_BZL_BLEND_MODE,
															HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);

		// Contract the tube mask to leave a little extra black ring at the edge of the bezel
		// otherwise this will show slivers of the background otherwise
		float tube_mask_contracted = HSM_GetCornerMask((tube_curved_coord - 0.5) * 1.004 + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);
		float FRAME_AND_BEZEL_MASK = (1 - tube_mask_contracted) * (1 - OUTSIDE_FRAME_MASK);

		if (HSM_BZL_OPACITY > 0 || HSM_FRM_OPACITY > 0)
			bezel_layer = clamp(bezel_and_frame_rgba * FRAME_AND_BEZEL_MASK, 0, 1);

		frame_shadow_layer *= HSM_FRM_SHADOW_OPACITY;

		if (HSM_FRM_SHADOW_OPACITY > 0)
			bezel_layer = BlendModeMaskLayerMix(frame_shadow_layer,
													bezel_layer, 
													BLEND_MODE_NORMAL, 
													MASK_MODE_ALL,
													0, 
													SHOW_ON_DUALSCREEN_MODE_BOTH,
													1);


		float bezel_opacity_mult = HSM_BZL_OPACITY + OUTSIDE_BEZEL_MASK * (1 - HSM_BZL_OPACITY);
		float frame_opacity_mult = HSM_FRM_OPACITY + (1 - OUTSIDE_BEZEL_MASK) * (1 - HSM_FRM_OPACITY);

		bezel_layer *= bezel_opacity_mult * frame_opacity_mult;
	}

	float TUBE_MASK_EXPAND = HSM_GetCornerMask((tube_curved_coord - 0.5) * 0.997 + 0.5, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);
	
	// vec4 tube_bg_layer = vec4(0, 0, 0, TUBE_MASK_EXPAND * HSM_GetTubeOpacity());
	vec4 tube_bg_layer = vec4(0, 0, 0, 0);

// end of ifndef LAYERS_UNDER_CRT
#endif

	//-----------------------------------------------------------------------------------------
	// Background
	//-----------------------------------------------------------------------------------------
	bool bg_use_vert_image = SCREEN_ASPECT < 1 && textureSize(BackgroundVertImage, 0).y > 16 ? true : false; 
	vec2 bg_size = bg_use_vert_image ? textureSize(BackgroundVertImage, 0) : textureSize(BackgroundImage, 0);
	BACKGROUND_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
												VIEWPORT_UNSCALED_COORD,
												TEXTURE_ASPECT_MODE_EXPLICIT,
												bg_size.x / bg_size.y,
												vec2(HSM_BG_POS_X, HSM_BG_POS_Y),
												vec2(HSM_BG_SCALE * HSM_BG_SCALE_X, HSM_BG_SCALE),
												HSM_BG_FOLLOW_LAYER,
												HSM_BG_FOLLOW_MODE,
												HSM_BG_FOLLOW_FULL_USES_ZOOM,
												HSM_BG_FILL_MODE,
												HSM_BG_SPLIT_PRESERVE_CENTER,
												HSM_BG_SPLIT_REPEAT_WIDTH,
												true,
												BACKGROUND_COORD,
												BACKGROUND_SCALE);

	// Tile Wrap the background
	if (HSM_BG_WRAP_MODE == 1)
		BACKGROUND_CURVED_COORD = mod(BACKGROUND_CURVED_COORD, 1.0);
	
	// Mirror Wrap the bBackground
	if (HSM_BG_WRAP_MODE == 2)
		BACKGROUND_CURVED_COORD = HSM_GetMirrorWrapCoord(BACKGROUND_CURVED_COORD);

	vec4 bg_image = vec4(0);
	if (HSM_BG_OPACITY > 0 && bg_size.y > 16)
	{
		if (bg_use_vert_image)
			bg_image = HSM_GetMipmappedTexSample(BackgroundVertImage, BACKGROUND_CURVED_COORD, BACKGROUND_SCALE, HSM_BG_MIPMAPPING_BLEND_BIAS);
		else
			bg_image = HSM_GetMipmappedTexSample(BackgroundImage, BACKGROUND_CURVED_COORD, BACKGROUND_SCALE, HSM_BG_MIPMAPPING_BLEND_BIAS);

		// Premultiply Alpha
		bg_image = HSM_GetPreMultipliedColorLinear(bg_image, HSM_BG_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

		// HSV Adjustments
		bg_image.rgb = HSM_ApplyHSVAdjustment(bg_image.rgb, HSM_BG_HUE, HSM_BG_SATURATION, HSM_BG_BRIGHTNESS, HSM_BG_COLORIZE_ON, HSM_BG_GAMMA);

		if (HSM_STATIC_LAYERS_GAMMA != 1)
			bg_image = HSM_ApplyGamma(bg_image, HSM_STATIC_LAYERS_GAMMA);

		bg_image.rgb = ApplyAmbientImages(bg_image.rgb, 
												ambient_image.rgb,
												ambient2_image.rgb,
												HSM_BG_AMBIENT_LIGHTING_MULTIPLIER,
												HSM_BG_AMBIENT2_LIGHTING_MULTIPLIER,
												HSM_BG_APPLY_AMBIENT_IN_ADD_MODE,
												HSM_BG_BLEND_MODE,
												HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
	}

	//----------------------------------------------------
	// Device Image
	//----------------------------------------------------
	bool device_use_vert_image = SCREEN_ASPECT < 1 && textureSize(DeviceVertImage, 0).y > 16 ? true : false; 
	vec2 device_size = device_use_vert_image ? textureSize(DeviceVertImage, 0) : textureSize(DeviceImage, 0);
	DEVICE_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
											VIEWPORT_UNSCALED_COORD,
											TEXTURE_ASPECT_MODE_EXPLICIT,
											device_size.x / device_size.y,
											vec2(HSM_DEVICE_POS_X, HSM_DEVICE_POS_Y),
											vec2(HSM_DEVICE_SCALE * HSM_DEVICE_SCALE_X, HSM_DEVICE_SCALE),
											HSM_DEVICE_FOLLOW_LAYER,
											HSM_DEVICE_FOLLOW_MODE,
											HSM_DEVICE_FOLLOW_FULL_USES_ZOOM,
											HSM_DEVICE_FILL_MODE,
											HSM_DEVICE_SPLIT_PRESERVE_CENTER,
											HSM_DEVICE_SPLIT_REPEAT_WIDTH,
											true,
											DEVICE_COORD,
											DEVICE_SCALE);

	vec4 device_image = vec4(0);
	if (HSM_DEVICE_OPACITY > 0 && device_size.y > 16)
	{
		device_image = device_use_vert_image ? 	HSM_GetMipmappedTexSample(DeviceVertImage, DEVICE_CURVED_COORD, DEVICE_SCALE, HSM_DEVICE_MIPMAPPING_BLEND_BIAS) :
												HSM_GetMipmappedTexSample(DeviceImage, DEVICE_CURVED_COORD, DEVICE_SCALE, HSM_DEVICE_MIPMAPPING_BLEND_BIAS);

		// Premultiply Alpha
		device_image = HSM_GetPreMultipliedColorLinear(device_image, HSM_DEVICE_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

		// HSV Adjustments
		device_image.rgb = HSM_ApplyHSVAdjustment(device_image.rgb, HSM_DEVICE_HUE, HSM_DEVICE_SATURATION, HSM_DEVICE_BRIGHTNESS, HSM_DEVICE_COLORIZE_ON, HSM_DEVICE_GAMMA);

		if (HSM_STATIC_LAYERS_GAMMA != 1)
			device_image = HSM_ApplyGamma(device_image, HSM_STATIC_LAYERS_GAMMA);

		device_image.rgb = ApplyAmbientImages(device_image.rgb, 
													ambient_image.rgb,
													ambient2_image.rgb,
													HSM_DEVICE_AMBIENT_LIGHTING_MULTIPLIER,
													HSM_DEVICE_AMBIENT2_LIGHTING_MULTIPLIER,
													HSM_DEVICE_APPLY_AMBIENT_IN_ADD_MODE,
													HSM_DEVICE_BLEND_MODE,
													HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
	}

	//----------------------------------------------------
	// DeviceLED Image
	//----------------------------------------------------
	vec2 deviceled_size = textureSize(DeviceLEDImage, 0);
	DEVICELED_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
											VIEWPORT_UNSCALED_COORD,
											TEXTURE_ASPECT_MODE_EXPLICIT,
											deviceled_size.x / deviceled_size.y,
											vec2(HSM_DEVICELED_POS_X, HSM_DEVICELED_POS_Y),
											vec2(HSM_DEVICELED_SCALE * HSM_DEVICELED_SCALE_X, HSM_DEVICELED_SCALE),
											HSM_DEVICELED_FOLLOW_LAYER,
											HSM_DEVICELED_FOLLOW_MODE,
											HSM_DEVICELED_FOLLOW_FULL_USES_ZOOM,
											HSM_DEVICELED_FILL_MODE,
											HSM_DEVICELED_SPLIT_PRESERVE_CENTER,
											HSM_DEVICELED_SPLIT_REPEAT_WIDTH,
											true,
											DEVICELED_COORD,
											DEVICELED_SCALE);

	vec4 deviceled_image = vec4(0);
	if (HSM_DEVICELED_OPACITY > 0 && deviceled_size.y > 16)
	{
		deviceled_image = HSM_GetMipmappedTexSample(DeviceLEDImage, DEVICELED_CURVED_COORD, DEVICELED_SCALE, HSM_DEVICELED_MIPMAPPING_BLEND_BIAS);

		// Premultiply Alpha
		deviceled_image = HSM_GetPreMultipliedColorLinear(deviceled_image, HSM_DEVICELED_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

		// HSV Adjustments
		deviceled_image.rgb = HSM_ApplyHSVAdjustment(deviceled_image.rgb, HSM_DEVICELED_HUE, HSM_DEVICELED_SATURATION, HSM_DEVICELED_BRIGHTNESS, HSM_DEVICELED_COLORIZE_ON, HSM_DEVICELED_GAMMA);

		if (HSM_STATIC_LAYERS_GAMMA != 1)
			deviceled_image = HSM_ApplyGamma(deviceled_image, HSM_STATIC_LAYERS_GAMMA);

		deviceled_image.rgb = ApplyAmbientImages(deviceled_image.rgb, 
													ambient_image.rgb,
													ambient2_image.rgb,
													HSM_DEVICELED_AMBIENT_LIGHTING_MULTIPLIER,
													HSM_DEVICELED_AMBIENT2_LIGHTING_MULTIPLIER,
													HSM_DEVICELED_APPLY_AMBIENT_IN_ADD_MODE,
													HSM_DEVICELED_BLEND_MODE,
													HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
	}

	//----------------------------------------------------
	// LED Image
	//----------------------------------------------------
	vec4 led_image = vec4(0);
	if (HSM_LED_OPACITY > 0)
	{
		vec2 led_size = textureSize(LEDImage, 0);
		LED_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
											VIEWPORT_UNSCALED_COORD,
											TEXTURE_ASPECT_MODE_EXPLICIT,
											led_size.x / led_size.y, 
											vec2(HSM_LED_POS_X, HSM_LED_POS_Y), 
											vec2(HSM_LED_SCALE * HSM_LED_SCALE_X, HSM_LED_SCALE),
											HSM_LED_FOLLOW_LAYER,
											HSM_LED_FOLLOW_MODE,
											HSM_LED_FOLLOW_FULL_USES_ZOOM,
											HSM_LED_FILL_MODE,
											HSM_LED_SPLIT_PRESERVE_CENTER,
											HSM_LED_SPLIT_REPEAT_WIDTH,
											true,
											LED_COORD,
											LED_SCALE);

		if (HSM_LED_OPACITY > 0 && led_size.y > 16)
		{
			led_image = HSM_GetMipmappedTexSample(LEDImage, LED_CURVED_COORD, LED_SCALE, HSM_LED_MIPMAPPING_BLEND_BIAS);

			// Premultiply Alpha
			led_image = HSM_GetPreMultipliedColorLinear(led_image, HSM_LED_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

			// HSV Adjustments
			led_image.rgb = HSM_ApplyHSVAdjustment(led_image.rgb, HSM_LED_HUE, HSM_LED_SATURATION, HSM_LED_BRIGHTNESS, HSM_LED_COLORIZE_ON, HSM_LED_GAMMA);

			// STATIC GAMMA
			if (HSM_STATIC_LAYERS_GAMMA != 1)
				led_image = HSM_ApplyGamma(led_image, HSM_STATIC_LAYERS_GAMMA);

			led_image.rgb = ApplyAmbientImages(led_image.rgb, 
													ambient_image.rgb,
													ambient2_image.rgb,
													HSM_LED_AMBIENT_LIGHTING_MULTIPLIER,
													HSM_LED_AMBIENT2_LIGHTING_MULTIPLIER,
													HSM_LED_APPLY_AMBIENT_IN_ADD_MODE,
													HSM_LED_BLEND_MODE,
													HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
		}
	}

	//----------------------------------------------------
	// Decal Image
	//----------------------------------------------------
	vec2 decal_size = textureSize(DecalImage, 0);
	DECAL_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
											VIEWPORT_UNSCALED_COORD,
											TEXTURE_ASPECT_MODE_EXPLICIT,
											decal_size.x / decal_size.y,
											vec2(HSM_DECAL_POS_X, HSM_DECAL_POS_Y), 
											vec2(HSM_DECAL_SCALE * HSM_DECAL_SCALE_X, HSM_DECAL_SCALE),
											HSM_DECAL_FOLLOW_LAYER,
											HSM_DECAL_FOLLOW_MODE,
											HSM_DECAL_FOLLOW_FULL_USES_ZOOM,
											HSM_DECAL_FILL_MODE,
											HSM_DECAL_SPLIT_PRESERVE_CENTER,
											HSM_DECAL_SPLIT_REPEAT_WIDTH,
											true,
											DECAL_COORD,
											DECAL_SCALE);
	vec4 decal_image = vec4(0);
	if (HSM_DECAL_OPACITY > 0 && decal_size.y > 16)
	{
		decal_image = HSM_GetMipmappedTexSample(DecalImage, DECAL_CURVED_COORD, DECAL_SCALE, HSM_DECAL_MIPMAPPING_BLEND_BIAS);
		
		// Premultiply Alpha
		decal_image = HSM_GetPreMultipliedColorLinear(decal_image, HSM_DECAL_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

		// HSV Adjustments
		decal_image.rgb = HSM_ApplyHSVAdjustment(decal_image.rgb, HSM_DECAL_HUE, HSM_DECAL_SATURATION, HSM_DECAL_BRIGHTNESS, HSM_DECAL_COLORIZE_ON, HSM_DECAL_GAMMA);

		// STATIC GAMMA
		if (HSM_STATIC_LAYERS_GAMMA != 1)
			decal_image = HSM_ApplyGamma(decal_image, HSM_STATIC_LAYERS_GAMMA);

		decal_image.rgb = ApplyAmbientImages(decal_image.rgb, 
												ambient_image.rgb,
												ambient2_image.rgb,
												HSM_DECAL_AMBIENT_LIGHTING_MULTIPLIER,
												HSM_DECAL_AMBIENT2_LIGHTING_MULTIPLIER,
												HSM_DECAL_APPLY_AMBIENT_IN_ADD_MODE,
												HSM_DECAL_BLEND_MODE,
												HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
	}

//----------------------------------------------------
//  ADV Get Additional Layers and Composite 
//----------------------------------------------------
	vec2 top_size = textureSize(TopLayerImage, 0);
	TOP_IMAGE_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
												VIEWPORT_UNSCALED_COORD,
												TEXTURE_ASPECT_MODE_EXPLICIT,
												top_size.x / top_size.y,
												vec2(HSM_TOP_POS_X, HSM_TOP_POS_Y), 
												vec2(HSM_TOP_SCALE * HSM_TOP_SCALE_X, HSM_TOP_SCALE),
												HSM_TOP_FOLLOW_LAYER,
												HSM_TOP_FOLLOW_MODE,
												HSM_TOP_FOLLOW_FULL_USES_ZOOM,
												HSM_TOP_FILL_MODE,
												HSM_TOP_SPLIT_PRESERVE_CENTER,
												HSM_TOP_SPLIT_REPEAT_WIDTH,
												true,
												TOP_IMAGE_COORD,
												TOP_IMAGE_SCALE);

	if (HSM_TOP_MIRROR_WRAP == 1)
		TOP_IMAGE_CURVED_COORD = HSM_GetMirrorWrapCoord(TOP_IMAGE_CURVED_COORD);

	vec4 top_image = vec4(0);
	if (HSM_TOP_OPACITY > 0 && top_size.y > 16)
	{
		// Get the top image color and masking values if needed
		top_image = HSM_GetMipmappedTexSample(TopLayerImage, TOP_IMAGE_CURVED_COORD, TOP_IMAGE_SCALE, HSM_TOP_MIPMAPPING_BLEND_BIAS);

		// Premultiply Alpha
		top_image = HSM_GetPreMultipliedColorLinear(top_image, HSM_TOP_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

		// HSV Adjustments
		top_image.rgb = HSM_ApplyHSVAdjustment(top_image.rgb, HSM_TOP_HUE, HSM_TOP_SATURATION, HSM_TOP_BRIGHTNESS, HSM_TOP_COLORIZE_ON, HSM_TOP_GAMMA);

		// STATIC GAMMA
		if (HSM_STATIC_LAYERS_GAMMA != 1)
			top_image = HSM_ApplyGamma(top_image, HSM_STATIC_LAYERS_GAMMA);

		top_image.rgb = ApplyAmbientImages(top_image.rgb, 
											ambient_image.rgb,
											ambient2_image.rgb,
											HSM_TOP_AMBIENT_LIGHTING_MULTIPLIER,
											HSM_TOP_AMBIENT2_LIGHTING_MULTIPLIER,
											HSM_TOP_APPLY_AMBIENT_IN_ADD_MODE,
											HSM_TOP_BLEND_MODE,
											HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
	}

	//----------------------------------------------------
	// Cabinet Glass Image
	//----------------------------------------------------
	vec4 cab_glass_image = vec4(0);

	if (HSM_CAB_GLASS_OPACITY > 0)
	{


		vec2 cab_glass_size = textureSize(CabinetGlassImage, 0);
		CAB_GLASS_CURVED_COORD = HSM_GetScaledCoord(VIEWPORT_COORD,
													VIEWPORT_UNSCALED_COORD,
													TEXTURE_ASPECT_MODE_EXPLICIT,
													cab_glass_size.x / cab_glass_size.y,
													vec2(HSM_CAB_GLASS_POS_X, HSM_CAB_GLASS_POS_Y), 
													vec2(HSM_CAB_GLASS_SCALE * HSM_CAB_GLASS_SCALE_X, HSM_CAB_GLASS_SCALE),
													HSM_CAB_GLASS_FOLLOW_LAYER,
													HSM_CAB_GLASS_FOLLOW_MODE,
													HSM_CAB_GLASS_FOLLOW_FULL_USES_ZOOM,
													HSM_CAB_GLASS_FILL_MODE,
													HSM_CAB_GLASS_SPLIT_PRESERVE_CENTER,
													HSM_CAB_GLASS_SPLIT_REPEAT_WIDTH,
													true,
													CAB_GLASS_COORD,
													CAB_GLASS_SCALE);

		if (HSM_CAB_GLASS_OPACITY > 0 && cab_glass_size.y > 16)
		{
			// Sample Texture
			cab_glass_image = HSM_GetMipmappedTexSample(CabinetGlassImage, CAB_GLASS_CURVED_COORD, CAB_GLASS_SCALE, HSM_CAB_GLASS_MIPMAPPING_BLEND_BIAS);

			// Premultiply Alpha
			cab_glass_image = HSM_GetPreMultipliedColorLinear(cab_glass_image, HSM_CAB_GLASS_SOURCE_MATTE_TYPE, DEFAULT_SRGB_GAMMA);

			// HSV Adjustments
			cab_glass_image.rgb = HSM_ApplyHSVAdjustment(cab_glass_image.rgb, HSM_CAB_GLASS_HUE, HSM_CAB_GLASS_SATURATION, HSM_CAB_GLASS_BRIGHTNESS, HSM_CAB_GLASS_COLORIZE_ON, HSM_CAB_GLASS_GAMMA);

			// STATIC GAMMA
			if (HSM_STATIC_LAYERS_GAMMA != 1)
				cab_glass_image = HSM_ApplyGamma(cab_glass_image, HSM_STATIC_LAYERS_GAMMA);

			cab_glass_image.rgb = ApplyAmbientImages(cab_glass_image.rgb, 
														ambient_image.rgb,
														ambient2_image.rgb,
														HSM_CAB_GLASS_AMBIENT_LIGHTING_MULTIPLIER,
														HSM_CAB_GLASS_AMBIENT2_LIGHTING_MULTIPLIER,
														HSM_CAB_GLASS_APPLY_AMBIENT_IN_ADD_MODE,
														HSM_CAB_GLASS_BLEND_MODE,
														HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE);
		}
	}


	//-----------------------------------------------------------------------------------------
	// CUTOUT MASK
	//-----------------------------------------------------------------------------------------
	CUTOUT_MASK = 1;

	vec2 temp_scale = vec2(0.5);
	vec2 temp_coord = vec2(0.5);

	vec2 cutout_base_scale_offset = vec2(0.5);

	if ( HSM_CUTOUT_FOLLOW_LAYER == FOLLOW_LAYER_VIEWPORT )
		cutout_base_scale_offset *= vec2(DEFAULT_UNCORRECTED_BEZEL_SCALE.y * TUBE_DIFFUSE_ASPECT / output_aspect, DEFAULT_BEZEL_SCALE.y);

	vec2 cutout_coord = HSM_GetScaledCoord(VIEWPORT_COORD,
											VIEWPORT_UNSCALED_COORD,
										  	HSM_CUTOUT_ASPECT_MODE,
											HSM_CUTOUT_EXPLICIT_ASPECT,
											vec2(HSM_CUTOUT_POS_X, HSM_CUTOUT_POS_Y),
											vec2(vec2(HSM_CUTOUT_SCALE * HSM_CUTOUT_SCALE_X, HSM_CUTOUT_SCALE) * cutout_base_scale_offset),
											HSM_CUTOUT_FOLLOW_LAYER,
											FOLLOW_MODE_SCALE_AND_POS,
											HSM_CUTOUT_FOLLOW_FULL_USES_ZOOM,
											FILL_MODE_STRETCH,
											0,
											0,
											false,
											temp_coord,
											temp_scale);
	
	CUTOUT_MASK = 1 - HSM_GetCornerMask(cutout_coord, TUBE_DIFFUSE_ASPECT, HSM_CUTOUT_CORNER_RADIUS, 0.8);

	//-----------------------------------------------------------------------------------------
	// Full Viewport Vignette
	//-----------------------------------------------------------------------------------------
	vec4 vignette_layer = vec4(0);
	if (HSM_VIEWPORT_VIGNETTE_OPACITY > 0)
	{
		vec2 vignette_coord = HSM_GetScaledCoord(VIEWPORT_COORD,
												VIEWPORT_UNSCALED_COORD,
												TEXTURE_ASPECT_MODE_VIEWPORT,
												1,
												vec2(HSM_VIEWPORT_VIGNETTE_POS_X, HSM_VIEWPORT_VIGNETTE_POS_Y), 
												vec2(vec2(HSM_VIEWPORT_VIGNETTE_SCALE * HSM_VIEWPORT_VIGNETTE_SCALE_X, HSM_VIEWPORT_VIGNETTE_SCALE)), 
												HSM_VIEWPORT_VIGNETTE_FOLLOW_LAYER,
												FOLLOW_MODE_SCALE_AND_POS,
												FOLLOW_LAYER_VIEWPORT,
												FILL_MODE_STRETCH,
												0,
												0,
												false,
												temp_coord,
												temp_scale);

		vignette_layer.a += 0.75 * HHLP_QuadraticBezier(1 - HSM_GetVignetteFactor(vignette_coord, HSM_VIEWPORT_VIGNETTE_OPACITY, 1), vec2(1, 0.5));
	}


	//-----------------------------------------------------------------------------------------
	// COMPOSITE ALL LAYERS
	//-----------------------------------------------------------------------------------------

	vec4 frag_color_linear = vec4(0);

	int start_layer = 0;
	int end_layer = int(MAX_LAYER_ORDER);

#ifdef LAYERS_OVER_CRT
	start_layer = int(HSM_CRT_LAYER_ORDER);
	end_layer = int(MAX_LAYER_ORDER);
#else
	start_layer = 0;
	end_layer = int(HSM_CRT_LAYER_ORDER - 1);
#endif

	OUTSIDE_TUBE_MASK_FOR_IMAGE = 1 - HSM_GetCornerMask(tube_curved_coord_ctr * 1.003 + 0.5, TUBE_DIFFUSE_ASPECT, HSM_FRM_OUTER_CORNER_RADIUS, 1);

	for(int i=start_layer; i <= end_layer; i++)
	{
		// BACKGROUND
		if (HSM_BG_LAYER_ORDER == i)
			frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
													bg_image, 
													HSM_BG_BLEND_MODE, 
													HSM_BG_MASK_MODE, 
													HSM_BG_CUTOUT_MODE,
													HSM_BG_DUALSCREEN_VIS_MODE,
													HSM_BG_OPACITY);

		// VIGNETTE
		if (HSM_VIEWPORT_VIGNETTE_LAYER_ORDER == i)
			frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
													vignette_layer, 
													BLEND_MODE_NORMAL, 
													HSM_VIEWPORT_VIGNETTE_MASK_MODE, 
													HSM_VIEWPORT_VIGNETTE_CUTOUT_MODE,
													SHOW_ON_DUALSCREEN_MODE_BOTH,
													HSM_VIEWPORT_VIGNETTE_OPACITY);
		// LED IMAGE
		if (HSM_LED_LAYER_ORDER == i && HSM_LED_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														led_image, 
														HSM_LED_BLEND_MODE, 
														HSM_LED_MASK_MODE, 
														HSM_LED_CUTOUT_MODE, 
														HSM_LED_DUALSCREEN_VIS_MODE,
														HSM_LED_OPACITY);

		// DEVICE IMAGE
		if (HSM_DEVICE_LAYER_ORDER == i && HSM_DEVICE_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														device_image, 
														HSM_DEVICE_BLEND_MODE, 
														HSM_DEVICE_MASK_MODE, 
														HSM_DEVICE_CUTOUT_MODE, 
														HSM_DEVICE_DUALSCREEN_VIS_MODE,
														HSM_DEVICE_OPACITY);

		// DEVICELED IMAGE
		if (HSM_DEVICELED_LAYER_ORDER == i && HSM_DEVICELED_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														deviceled_image, 
														HSM_DEVICELED_BLEND_MODE, 
														HSM_DEVICELED_MASK_MODE, 
														HSM_DEVICELED_CUTOUT_MODE, 
														HSM_DEVICELED_DUALSCREEN_VIS_MODE,
														HSM_DEVICELED_OPACITY);

		// DECAL IMAGE
		if (HSM_DECAL_LAYER_ORDER == i && HSM_DECAL_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														decal_image, 
														HSM_DECAL_BLEND_MODE, 
														HSM_DECAL_MASK_MODE, 
														HSM_DECAL_CUTOUT_MODE, 
														HSM_DECAL_DUALSCREEN_VIS_MODE,
														HSM_DECAL_OPACITY);

		// CABINET GLASS
		if (HSM_CAB_GLASS_LAYER_ORDER == i && HSM_CAB_GLASS_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														cab_glass_image, 
														HSM_CAB_GLASS_BLEND_MODE, 
														HSM_CAB_GLASS_MASK_MODE, 
														HSM_CAB_GLASS_CUTOUT_MODE, 
														HSM_CAB_GLASS_DUALSCREEN_VIS_MODE,
														HSM_CAB_GLASS_OPACITY);


		// Top Layer
		if (HSM_TOP_LAYER_ORDER == i && HSM_TOP_OPACITY > 0)
				frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
														top_image, 
														HSM_TOP_BLEND_MODE, 
														HSM_TOP_MASK_MODE, 
														HSM_TOP_CUTOUT_MODE, 
														HSM_TOP_DUALSCREEN_VIS_MODE,
														HSM_TOP_OPACITY);
	}

#ifdef LAYERS_UNDER_CRT

	// Add background behind tube
	frag_color_linear = HSM_PreMultAlphaBlend(frag_color_linear, tube_bg_layer);
	// GENERATED BEZEL LAYER
	if (HSM_BZL_OPACITY > 0 || HSM_FRM_OPACITY > 0)
		frag_color_linear = BlendModeMaskLayerMix(frag_color_linear, 
												bezel_layer, 
												HSM_BZL_BLEND_MODE, 
												MASK_MODE_ALL, 
												0, 
												SHOW_ON_DUALSCREEN_MODE_BOTH,
												1);
#endif

	//-----------------------------------------------------------------------------------------
	// MASK DEBUG DISPLAY
	//-----------------------------------------------------------------------------------------
	// Show a red overlay on the screen showing the mask for each mask mode
	if (HSM_LAYERING_DEBUG_MASK_MODE != -1)
	{
		float debug_mask = 1;
		if (HSM_LAYERING_DEBUG_MASK_MODE == -2)
			debug_mask = CUTOUT_MASK;
		else
			debug_mask = GetMask(HSM_LAYERING_DEBUG_MASK_MODE);

		frag_color_linear = HSM_PreMultAlphaBlend(frag_color_linear, vec4(1, 0, 0, 1) * 0.15 * debug_mask);
		frag_color_linear = HSM_PreMultAlphaBlend(frag_color_linear, vec4(0.05, 0.05, 0.05, 1) * 0.15 * (1 - debug_mask));
		frag_color_linear = clamp(frag_color_linear, 0, 1);
	}

	#ifdef LAYERS_UNDER_CRT
	// Store reflection mask in the alpha channel
	// Usually used to show the uneven brightness of a bumpy surface
	if (HSM_REFLECT_MASK_IMAGE_AMOUNT > 0)
	{
		vec2 reflect_mask_scale = vec2(0.5);
		vec2 reflect_mask_coord = HSM_GetLayerCoord(VIEWPORT_COORD, HSM_REFLECT_MASK_FOLLOW_LAYER, HSM_REFLECT_MASK_FOLLOW_MODE, reflect_mask_scale);
		float reflect_mask = HSM_GetMipmappedTexSample(ReflectionMaskImage, reflect_mask_coord, reflect_mask_scale, HSM_REFLECT_MASK_MIPMAPPING_BLEND_BIAS).r;
		reflect_mask = clamp(HSM_REFLECT_MASK_BLACK_LEVEL * (reflect_mask - 1) + 1, 0, 1);
		reflect_mask *= HSM_REFLECT_MASK_BRIGHTNESS;

		frag_color_linear.a = HSM_REFLECT_MASK_IMAGE_AMOUNT * reflect_mask + (1 - HSM_REFLECT_MASK_IMAGE_AMOUNT);
	}
	else
		frag_color_linear.a = 1;

	#endif

	FragColor = frag_color_linear;

	// If we have calculated an image then set -1 as a flag to show that we have
	if (UNFLIPPED_VIEWPORT_COORD.x < (2 / global.OutputSize.x) && UNFLIPPED_VIEWPORT_COORD.y < (2 / global.OutputSize.y))
		FragColor.a = -1;

	return;
}
