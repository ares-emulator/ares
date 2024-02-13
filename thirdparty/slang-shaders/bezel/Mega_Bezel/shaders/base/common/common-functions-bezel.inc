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

vec2 HSM_GetOuterBezelScale(vec2 tube_diffuse_scale, float screen_aspect)
{
	vec2 bezel_outer_scale_offset = vec2(HSM_BZL_WIDTH / screen_aspect + 1, HSM_BZL_HEIGHT + 1);
	return bezel_outer_scale_offset;
}

vec2 GetDefaultScreenScale()
{
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;
	vec2 out_placement_scale = DEFAULT_UNCORRECTED_SCREEN_SCALE;
	out_placement_scale.x /= output_aspect;
	return out_placement_scale;
}

vec2 GetDefaultBezelScale()
{
	float output_aspect = global.FinalViewportSize.x / global.FinalViewportSize.y;
	vec2 out_placement_scale = DEFAULT_UNCORRECTED_BEZEL_SCALE;
	out_placement_scale.x /= output_aspect;
	return out_placement_scale;
}

float HSM_GetSimpleBezelCoords(vec2 tube_diffuse_coord, 
								vec2 tube_diffuse_scale, 
								vec2 tube_scale, 
								float screen_aspect,
								inout vec2 bezel_outside_coord, 
								inout vec2 frame_outside_coord)
{
	float output_aspect = global.OutputSize.x / global.OutputSize.y;

	vec2 bezel_outer_pos_offset = vec2(0, HSM_BZL_OUTER_POSITION_Y);
	vec2 bezel_outer_scale_offset = HSM_GetOuterBezelScale(tube_diffuse_scale, screen_aspect);

	bezel_outside_coord = tube_diffuse_coord + bezel_outer_pos_offset;

	vec2 black_edge_scale_offset = tube_scale / tube_diffuse_scale;

	bezel_outside_coord = HSM_GetInverseScaledCoord(bezel_outside_coord, black_edge_scale_offset * bezel_outer_scale_offset) + vec2(0, HSM_BZL_OUTER_POSITION_Y);
	frame_outside_coord = (bezel_outside_coord + 
							vec2(0, HSM_FRM_OUTER_POS_Y) - 0.5) / 
							vec2((HSM_FRM_THICKNESS * HSM_FRM_THICKNESS_SCALE_X) / 
							(tube_diffuse_scale.x / tube_diffuse_scale.y * output_aspect) + 1, 
							HSM_FRM_THICKNESS + 1) + 0.5;
	return 0;
}

float HSM_GetBezelCoords(vec2 tube_diffuse_coord, 
							vec2 tube_diffuse_scale, 
							vec2 tube_scale, 
							float screen_aspect, 
							bool curve_coords_on,
							inout vec2 bezel_outside_scale,
							inout vec2 bezel_outside_coord, 
							inout vec2 bezel_outside_curved_coord, 
							inout vec2 frame_outside_curved_coord)
{
	float output_aspect = global.OutputSize.x / global.OutputSize.y;

	vec2 bezel_outer_pos_offset = vec2(0, HSM_BZL_OUTER_POSITION_Y);
	vec2 bezel_outer_scale_offset = HSM_GetOuterBezelScale(tube_diffuse_scale, screen_aspect);

	bezel_outside_coord = tube_diffuse_coord + bezel_outer_pos_offset;
	bezel_outside_curved_coord = bezel_outside_coord;

	// Only run curved coordinates if requested or we are using tilt
	if (curve_coords_on)
		if ((HSM_CURVATURE_MODE > 0) && ((HSM_CURVATURE_3D_TILT_ANGLE_X != 0) || (HSM_CURVATURE_3D_TILT_ANGLE_Y != 0)))
			bezel_outside_curved_coord = HSM_GetCurvedCoord(bezel_outside_curved_coord, 0, bezel_outer_scale_offset.x * output_aspect / bezel_outer_scale_offset.y);

	vec2 black_edge_scale_offset = tube_scale / tube_diffuse_scale;

	bezel_outside_scale.xy = tube_diffuse_scale * black_edge_scale_offset * bezel_outer_scale_offset;
	// bezel_outside_transform.zw = -vec2(HSM_SCREEN_POSITION_X, HSM_SCREEN_POSITION_Y) + bezel_outer_pos_offset * tube_diffuse_scale;

	bezel_outside_coord = HSM_GetInverseScaledCoord(bezel_outside_coord, black_edge_scale_offset * bezel_outer_scale_offset) + vec2(0, HSM_BZL_OUTER_POSITION_Y);
	bezel_outside_curved_coord = HSM_GetInverseScaledCoord(bezel_outside_curved_coord, black_edge_scale_offset * bezel_outer_scale_offset) + vec2(0, HSM_BZL_OUTER_POSITION_Y);

	frame_outside_curved_coord = (bezel_outside_curved_coord + 
							vec2(0, HSM_FRM_OUTER_POS_Y) - 0.5) / 
							vec2((HSM_FRM_THICKNESS * HSM_FRM_THICKNESS_SCALE_X) / 
							(tube_diffuse_scale.x / tube_diffuse_scale.y * output_aspect) + 1, 
							HSM_FRM_THICKNESS + 1) + 0.5;

	if (HSM_BZL_OUTER_CURVATURE_SCALE > 0)
	{
		if (curve_coords_on)
			bezel_outside_curved_coord = HSM_GetTubeCurvedCoord(bezel_outside_curved_coord, 
																HSM_BZL_OUTER_CURVATURE_SCALE, 
																tube_diffuse_scale, 
																tube_scale, 
																bezel_outer_scale_offset.x * global.OutputSize.x / global.OutputSize.y / bezel_outer_scale_offset.y, 
																0);

		if (curve_coords_on)
			if ( HSM_FRM_OUTER_CURVATURE_SCALE > 0)
				frame_outside_curved_coord = HSM_GetTubeCurvedCoord(frame_outside_curved_coord, 
																	HSM_BZL_OUTER_CURVATURE_SCALE * HSM_FRM_OUTER_CURVATURE_SCALE, 
																	tube_diffuse_scale, 
																	tube_scale, 
																	bezel_outer_scale_offset.x * global.OutputSize.x / global.OutputSize.y / bezel_outer_scale_offset.y, 
																	0);
	}
	
	DEFAULT_SCREEN_SCALE = GetDefaultScreenScale();
	DEFAULT_BEZEL_SCALE = GetDefaultBezelScale();

	return 0;
}

float MAX_LAYER_ORDER = 12;

bool HSM_GetUseTubeStaticReflection()
{
	return HSM_TUBE_STATIC_REFLECTION_IMAGE_ON > 0.5 && HSM_GetUseOnCurrentScreenIndex(HSM_TUBE_STATIC_REFLECTION_IMAGE_DUALSCREEN_VIS_MODE);
}

bool HSM_GetUseTubeDiffuseImage()
{
	return HSM_TUBE_DIFFUSE_MODE == 1 && HSM_GetUseOnCurrentScreenIndex(HSM_TUBE_DIFFUSE_IMAGE_DUALSCREEN_VIS_MODE);
}

bool HSM_GetUseTubeColoredGelImage()
{
	return HSM_TUBE_COLORED_GEL_IMAGE_ON > 0.5 && HSM_GetUseOnCurrentScreenIndex(HSM_TUBE_COLORED_GEL_IMAGE_DUALSCREEN_VIS_MODE);
}