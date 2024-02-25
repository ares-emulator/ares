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

#include "common/globals-and-reflection-params.inc"
#include "common/common-functions.inc"
#include "common/common-functions-bezel.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 VIEWPORT_COORD;

void main()
{
	gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;
	VIEWPORT_COORD = vTexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 VIEWPORT_COORD;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D BR_MirrorLowResPass;

layout(set = 0, binding = 4) uniform sampler2D InfoCachePass;

float clamp_0_1(float in_value)
{
	return clamp(in_value, 0, 1);
}

void main()
{
	if (HSM_REFLECT_BLUR_NUM_SAMPLES > 0)
	{
		vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
		HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

		vec2 tube_curved_coord_ctr = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, 1, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 1) - 0.5;

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
		vec2 outermap_scale = vec2(1, 1) * (1.3 + 1);

		// Get a range width from the outer tube edge to the outer edge of the outermap
		float outermap_range = 0.5 * (outermap_scale.y) * 0.7;
		vec2 outermap_warped_coord_ctr = tube_curved_coord_ctr;
		vec2 outermap_warped_outside_screen_vector = outermap_warped_coord_ctr - clamp(outermap_warped_coord_ctr, -0.490, 0.490);
		float outside_ratio_warped = clamp(length(outermap_warped_outside_screen_vector) / outermap_range, 0, 1);

		float hbl_sharpness_falloff_distance = 7 * HSM_REFLECT_BLUR_FALLOFF_DISTANCE;
		if (HSM_GLASS_BORDER_ON == 1)
			hbl_sharpness_falloff_distance = 20;

		float blur_ratio = clamp_0_1(outside_ratio_warped / (hbl_sharpness_falloff_distance / 100));
		float blur_midpoint = 0.12;
		float hbl_sharpness_falloff_speed = 1;
		blur_ratio = HHLP_QuadraticBezier (clamp_0_1(blur_ratio - blur_midpoint), vec2(0.05, hbl_sharpness_falloff_speed));


		blur_ratio = HSM_REFLECT_BLUR_MIN + blur_ratio * (HSM_REFLECT_BLUR_MAX - HSM_REFLECT_BLUR_MIN);

		vec3 col = vec3(0.0);
		
		#ifdef VERTICAL_BLUR
			float dx = 0;
			float dy = global.SourceSize.w;
		#else
			float dx = global.SourceSize.z;
			float dy = 0;
		#endif

		// This bizarre bit is to try to take the non linear nature of the blur falloff value and make it into a more linear behavior
		float last_blur_ratio_blend_point = 0.6;

		float max_blur_range = 7;
		float min_blur_range = 10;
		float blur_falloff = (  0.5 +
								max_blur_range * smoothstep(1, last_blur_ratio_blend_point, blur_ratio) +
								min_blur_range * smoothstep(last_blur_ratio_blend_point, 0, blur_ratio)
								) / 100;

		float blend_with_unblurred = smoothstep(-0.3, 0.3, HHLP_QuadraticBezier(blur_ratio, vec2(0.5, 0.5)));

		float k_total = 0.;
		for (float i = -HSM_REFLECT_BLUR_NUM_SAMPLES; i <= HSM_REFLECT_BLUR_NUM_SAMPLES; i++)
		{
			float k = exp(-blur_falloff * (i) * (i));
			k_total += k;
			
			vec2 sample_coord = vTexCoord + vec2(float(i) * dx, float(i) * dy);
			vec4 sampled_color = texture(Source, sample_coord);
			
			col += k * sampled_color.rgb;
		}
		vec4 blurred_color = vec4(col / k_total, 1.0);

		vec4 unblurred_color = texture(BR_MirrorLowResPass, vTexCoord);
		FragColor = mix(unblurred_color, blurred_color, blend_with_unblurred);
	}
	else
	{
		FragColor = texture(Source, vTexCoord);
	}
}