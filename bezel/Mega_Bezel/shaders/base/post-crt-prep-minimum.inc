/*
    Mega Bezel - Creates a graphic treatment for the game play area to give a retro feel
    Copyright (C) 2019-2021 HyperspaceMadness - HyperspaceMadness@outlook.com

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

#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 0) out vec2 vTexCoord;
layout(location = 7) out vec2 UNFLIPPED_VIEWPORT_COORD;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.00001;

	UNFLIPPED_VIEWPORT_COORD = vTexCoord;
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 7) in vec2 UNFLIPPED_VIEWPORT_COORD;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D Source;

layout(set = 0, binding = 2) uniform sampler2D InfoCachePass;

layout(set = 0, binding = 7) uniform sampler2D BackgroundImage;
layout(set = 0, binding = 8) uniform sampler2D BackgroundVertImage;

layout(set = 0, binding = 11) uniform sampler2D PostCRTPassFeedback;
#define PassFeedback PostCRTPassFeedback

vec4 HSM_GetPostCrtPreppedColorPotato(vec4 in_color_linear, vec2 VIEWPORT_COORD, vec2 VIEWPORT_COORD_TRANSFORMED, vec2 screen_curved_coord, in sampler2D BackgroundImage, in sampler2D BackgroundVertImage)
{ 
	vec2 tube_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, 1, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 1);
	float bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;

	TUBE_MASK = HSM_GetCornerMask(tube_curved_coord, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);

#ifdef USE_MONOCHROME
	if (HSM_MONOCHROME_MODE > 0.5 && HSM_GetUseOnCurrentScreenIndex(HSM_MONOCHROME_DUALSCREEN_VIS_MODE))
		in_color_linear = HSM_ApplyMonochrome(in_color_linear);
#endif

	vec4 out_color = in_color_linear;
	
	out_color *= HSM_POST_CRT_BRIGHTNESS;

	vec2 mirrored_screen_coord = HSM_GetMirrorWrappedCoord(screen_curved_coord);
	float black_edge_corner_radius = HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	float screen_mask = HSM_GetCornerMask((screen_curved_coord - 0.5) * 0.999 + 0.5, SCREEN_ASPECT, black_edge_corner_radius, 0.9);
	vec2 tube_diffuse_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 0);
	float tube_diffuse_mask =  HSM_GetCornerMask(tube_diffuse_curved_coord, TUBE_DIFFUSE_ASPECT, black_edge_corner_radius, 0.99);

#ifdef USE_VIGNETTE
	if (HSM_GetUseScreenVignette())
	{
		float vignette_factor = HSM_GetScreenVignetteFactor(mirrored_screen_coord);

		if (HSM_SCREEN_VIGNETTE_IN_REFLECTION < 0.5)
			vignette_factor = screen_mask * vignette_factor + (1 - screen_mask);

		out_color.rgb *= vignette_factor;
	}
#endif

	out_color.rgb *= screen_mask;
	out_color *= TUBE_MASK;
	out_color *= tube_diffuse_mask;

	out_color = clamp(out_color, 0, 1);
	
#ifdef USE_BG
	if (HSM_BG_OPACITY > 0)
	{
		//-----------------------------------------------------------------------------------------
		// Background
		//-----------------------------------------------------------------------------------------
		vec2 bg_coord = VIEWPORT_COORD_TRANSFORMED;
		bool bg_use_vert_image = SCREEN_ASPECT < 1 && textureSize(BackgroundVertImage, 0).y > 16 ? true : false; 
		vec2 bg_size = bg_use_vert_image ? textureSize(BackgroundVertImage, 0) : textureSize(BackgroundImage, 0);

		vec4 bg_layer = vec4(0);

		if (bg_size.y > 16)
		{
			// Correct image coordinate for image aspect ratio
			bg_coord.x = (bg_coord.x - 0.5) / ((bg_size.x / bg_size.y) / (global.FinalViewportSize.x/global.FinalViewportSize.y)) + 0.5;

			if (bg_use_vert_image)
				bg_layer = texture(BackgroundVertImage, bg_coord);
			else
				bg_layer = texture(BackgroundImage, bg_coord);
			bg_layer = HSM_Linearize(bg_layer, DEFAULT_SRGB_GAMMA);

			vec3 bg_gel = HSM_POTATO_COLORIZE_CRT_WITH_BG * bg_layer.rgb * HSM_POTATO_COLORIZE_BRIGHTNESS + (1 - HSM_POTATO_COLORIZE_CRT_WITH_BG);

			bg_layer.rgb *= HSM_GLOBAL_GRAPHICS_BRIGHTNESS;
			bg_layer.rgb = pow(bg_layer.rgb, vec3(1/HSM_STATIC_LAYERS_GAMMA));

			bg_layer *= HSM_BG_BRIGHTNESS;

			if (HSM_POTATO_SHOW_BG_OVER_SCREEN == 1)
			{
				out_color.rgb *= bg_gel.rgb;
				out_color = HSM_BlendModeLayerMix(out_color, bg_layer, HSM_BG_BLEND_MODE, HSM_BG_OPACITY);
			}
			else
			{
				bg_layer.rgb *= HSM_BG_OPACITY;
				out_color = HSM_BlendModeLayerMix(bg_layer, out_color, HSM_CRT_SCREEN_BLEND_MODE, 1);
			}
		}
	}
#endif

	if (HSM_SINDEN_BORDER_ON > 0.5)
	{
		out_color = HSM_Apply_Sinden_Lightgun_Border(out_color, tube_diffuse_curved_coord, tube_diffuse_mask, black_edge_corner_radius);
	}

	return out_color;
}

void main()
{
	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);
	
	if (HHLP_IsOutsideCoordSpace(SCREEN_COORD))
	{
		vec4 feedback_color_test = texture(PassFeedback, vec2(0,0));
		if (HSM_CACHE_GRAPHICS_ON > 0.5 && feedback_color_test.a < 0 && !CACHE_INFO_CHANGED)
		{
			FragColor = texture(PassFeedback, UNFLIPPED_VIEWPORT_COORD);
			return;
		}
	}

	FragColor = texture(Source, UNFLIPPED_VIEWPORT_COORD);

	vec2 screen_curved_coord = HSM_GetCurvedCoord(SCREEN_COORD, HSM_CRT_CURVATURE_SCALE, SCREEN_ASPECT);
	FragColor = HSM_GetPostCrtPreppedColorPotato(FragColor, vTexCoord, viewportCoordTransformed, screen_curved_coord, BackgroundImage, BackgroundVertImage);

// #ifndef IS_MEGATRON_PRESET
// 	FragColor = HSM_Delinearize(FragColor, DEFAULT_SRGB_GAMMA);
// #endif
}
