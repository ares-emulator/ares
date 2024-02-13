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

#include "../base/common/common-functions-bezel.inc"

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

layout(set = 0, binding = 4) uniform sampler2D TubeDiffuseImage;
layout(set = 0, binding = 5) uniform sampler2D TubeShadowImage;
layout(set = 0, binding = 6) uniform sampler2D TubeColoredGelImage;
layout(set = 0, binding = 7) uniform sampler2D TubeStaticReflectionImage;

layout(set = 0, binding = 8) uniform sampler2D BackgroundImage;
layout(set = 0, binding = 9) uniform sampler2D BackgroundVertImage;

layout(set = 0, binding = 10) uniform sampler2D NightLightingImage;
layout(set = 0, binding = 11) uniform sampler2D NightLighting2Image;
layout(set = 0, binding = 12) uniform sampler2D DerezedPass;

layout(set = 0, binding = 13) uniform sampler2D PostCRTPassFeedback;
#define PassFeedback PostCRTPassFeedback

// Torridgristle - ScanlineSimple pass - Public domain

vec4 HSM_GetTubeHighlight(vec2 tube_curved_coord, bool apply_to_mirror, in sampler2D TubeStaticReflectionImage)
{
	tube_curved_coord = HSM_GetViewportCoordWithFlip(tube_curved_coord);
	vec4 out_color = vec4(0);

	vec2 mirrored_tube_coord = tube_curved_coord;
	if (apply_to_mirror)
		mirrored_tube_coord = HSM_GetMirrorWrappedCoord(tube_curved_coord);

	float bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	if(HSM_BZL_USE_INDEPENDENT_CURVATURE > 0)
		bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * DEFAULT_SCREEN_CORNER_RADIUS;

	vec2 tube_highlight_coord = (mirrored_tube_coord - 0.5) / vec2(HSM_TUBE_STATIC_SCALE * HSM_TUBE_STATIC_SCALE_X, HSM_TUBE_STATIC_SCALE) + 0.5;
	tube_highlight_coord = (tube_highlight_coord - 0.5) / vec2(HSM_TUBE_STATIC_SCALE_X, 1) + 0.5;

	tube_highlight_coord.x = (tube_highlight_coord.x + HSM_TUBE_STATIC_POS_X - 0.5) * HSM_FLIP_VIEWPORT_HORIZONTAL + 0.5;
	tube_highlight_coord.y = (tube_highlight_coord.y + HSM_TUBE_STATIC_POS_Y - 0.5) * HSM_FLIP_VIEWPORT_VERTICAL + 0.5;

	// Mask the very outer edge of the tube for a very small shadowing effect
	float tube_highlight_mask = HSM_GetCornerMask((tube_curved_coord - 0.5) + 0.5 , TUBE_DIFFUSE_ASPECT, HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS, 0.9);

	vec4 tube_highlight_image = vec4(0);

	if (HSM_TUBE_STATIC_DITHER_SAMPLES > 0)
	{
		// Dithering for the glass reflection if needed to reduce banding
		tube_highlight_image = HSM_GetStoichaicBlurredSample(TubeStaticReflectionImage, tube_highlight_coord, HSM_TUBE_STATIC_DITHER_SAMPLES, HSM_TUBE_STATIC_DITHER_DISTANCE, HSM_TUBE_STATIC_DITHER_AMOUNT);
	}
	else
	{
		tube_highlight_image = HSM_GetMipmappedTexSample(TubeStaticReflectionImage, tube_highlight_coord, TUBE_SCALE, 0);
	}
	tube_highlight_image = HSM_Linearize(tube_highlight_image, DEFAULT_SRGB_GAMMA) * tube_highlight_mask;

	out_color = HSM_TUBE_STATIC_REFLECTION_IMAGE_OPACITY * tube_highlight_image * tube_highlight_image.a;
	out_color.rgb *= HSM_GLOBAL_GRAPHICS_BRIGHTNESS;

	return out_color;
}

vec4 HSM_GetPostCrtPreppedColor(vec4 in_color_linear, vec2 VIEWPORT_COORD, vec2 screen_curved_coord, in sampler2D source_pass, in sampler2D TubeDiffuseImage, in sampler2D TubeColoredGelImage, in sampler2D TubeStaticReflectionImage, in sampler2D BackgroundImage, in sampler2D BackgroundVertImage, in sampler2D NightLightingImage, in sampler2D NightLighting2Image)
{
	if (HSM_MONOCHROME_MODE > 0.5 && HSM_GetUseOnCurrentScreenIndex(HSM_MONOCHROME_DUALSCREEN_VIS_MODE))
		in_color_linear = HSM_ApplyMonochrome(in_color_linear);

	vec4 crt_color = in_color_linear;

	crt_color *= HSM_POST_CRT_BRIGHTNESS;

	vec2 mirrored_screen_coord = HSM_GetMirrorWrappedCoord(screen_curved_coord);

	vec4 out_color = vec4(0);

	// TODO we see artifacts of the reflection at the edge of the screen when there is extra tube thickness and the reflection is scaled
	float screen_mask = HSM_GetCornerMask((screen_curved_coord - 0.5) * 0.999 + 0.5, SCREEN_ASPECT, HSM_GLOBAL_CORNER_RADIUS * HSM_SCREEN_CORNER_RADIUS_SCALE, 0.9);
						
	vec2 tube_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, 1, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 1);
	vec2 mirrored_tube_coord = HSM_GetMirrorWrappedCoord(tube_curved_coord);
	
	float vignette_factor = HSM_GetScreenVignetteFactor(mirrored_tube_coord);
	if (HSM_GetUseScreenVignette())
	{
		float vignette_factor_outside_screen = HSM_SCREEN_VIGNETTE_IN_REFLECTION * vignette_factor + (1 - HSM_SCREEN_VIGNETTE_IN_REFLECTION) * 1;
		vignette_factor = screen_mask * vignette_factor + (1 - screen_mask) * vignette_factor_outside_screen;

		crt_color *= vignette_factor;
	}

	if (HSM_AB_COMPARE_SHOW_MODE == 1 && HSM_GetIsInABCompareArea(VIEWPORT_COORD))
	{
		vec2 ab_screen_coord = HSM_CRT_CURVATURE_SCALE * screen_curved_coord + (1 - HSM_CRT_CURVATURE_SCALE) * SCREEN_COORD;
		ab_screen_coord = HSM_GetMirrorWrappedCoord(ab_screen_coord);

		vec4 source_color = HSM_GetCroppedTexSample(source_pass, ab_screen_coord);

		source_color = HSM_Linearize(source_color, GAMMA_INPUT);

		crt_color = source_color;
	}
	
	float bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	if(HSM_BZL_USE_INDEPENDENT_CURVATURE > 0)
		bezel_corner_radius = HSM_BZL_INNER_CORNER_RADIUS_SCALE * DEFAULT_SCREEN_CORNER_RADIUS;

	float tube_mask =  HSM_GetCornerMask(tube_curved_coord, TUBE_DIFFUSE_ASPECT, bezel_corner_radius, 0.99);
	float screen_to_tube_mask_invert = 1 - (tube_mask - screen_mask);

	float black_edge_corner_radius = HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE * HSM_GLOBAL_CORNER_RADIUS;
	vec2 tube_diffuse_curved_coord = HSM_GetTubeCurvedCoord(TUBE_DIFFUSE_COORD, HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE, TUBE_DIFFUSE_SCALE, TUBE_SCALE, TUBE_DIFFUSE_ASPECT, 0);
	float tube_diffuse_mask =  HSM_GetCornerMask(tube_diffuse_curved_coord, TUBE_DIFFUSE_ASPECT, black_edge_corner_radius, 0.99);

	float black_edge_mask_invert = 1 - (tube_mask - tube_diffuse_mask);

	crt_color.rgb *= screen_to_tube_mask_invert;

	// AMBIENT LIGHTING IMAGES
	vec4 ambient_lighting_image = vec4(1);
	vec4 ambient2_lighting_image = vec4(1);
	HSM_Fill_Ambient_Images(VIEWPORT_COORD, 
							VIEWPORT_UNSCALED_COORD, 
							TUBE_DIFFUSE_COORD_MIXED_POS, 
							TUBE_DIFFUSE_SCALE_1ST_SCREEN, 
							HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE, 
							NightLightingImage, 
							NightLighting2Image, 
							ambient_lighting_image, 
							ambient2_lighting_image);

	vec3 tube_shadow = vec3(1);
	if (HSM_TUBE_SHADOW_IMAGE_ON > 0.5 && (HSM_GetUseTubeDiffuseImage() || HSM_GetUseTubeColoredGelImage() || HSM_GetUseTubeStaticReflection()))
	{
		vec2 shadow_coord = tube_diffuse_curved_coord;
		shadow_coord = HSM_GetMirrorWrappedCoord(shadow_coord);

		// TODO Shadow Coordinate isn't right with 3D Curvature

		shadow_coord.x = (shadow_coord.x - HSM_TUBE_SHADOW_IMAGE_POS_X - 0.5) / HSM_TUBE_SHADOW_IMAGE_SCALE_X + 0.5;
		shadow_coord.y = (shadow_coord.y + HSM_TUBE_SHADOW_IMAGE_POS_Y) / HSM_TUBE_SHADOW_IMAGE_SCALE_Y;

		vec2 curvature_values = HSM_TUBE_SHADOW_CURVATURE_SCALE * HSM_GetCurvatureValues(TUBE_DIFFUSE_ASPECT);
		shadow_coord = HSM_Get2DCurvedCoord(shadow_coord, curvature_values);

		shadow_coord.x = HSM_FLIP_VIEWPORT_HORIZONTAL * (shadow_coord.x - 0.5) + 0.5;
		shadow_coord.y = HSM_FLIP_VIEWPORT_VERTICAL * (shadow_coord.y - 0.5) + 0.5;
		

		vec4 tube_shadow_sample = HSM_GetMipmappedTexSample(TubeShadowImage, shadow_coord, TUBE_SCALE * vec2(HSM_TUBE_SHADOW_IMAGE_SCALE_X, HSM_TUBE_SHADOW_IMAGE_SCALE_Y), 0);
		tube_shadow_sample = HSM_GLOBAL_GRAPHICS_BRIGHTNESS * HSM_GetPreMultipliedColorLinear(tube_shadow_sample, SOURCE_MATTE_PREMULTIPLIED, DEFAULT_SRGB_GAMMA);

		tube_shadow = (1 - HSM_TUBE_SHADOW_IMAGE_OPACITY) + HSM_TUBE_SHADOW_IMAGE_OPACITY * tube_shadow_sample.rgb;
	}

	// If the crt blend mode is multiply then use the diffuse image
	float tube_diffuse_mode = HSM_CRT_BLEND_MODE == 2 ? 1 : HSM_TUBE_DIFFUSE_MODE;
	vec4 tube_diffuse = vec4(0, 0, 0, tube_mask);
	float adjusted_tube_diffuse_amount = HSM_TUBE_DIFFUSE_MODE == 0 ? 0 : HSM_TUBE_DIFFUSE_IMAGE_AMOUNT;

	if (tube_diffuse_mode == 2)
	{
		// Do something if the tube is supposed to be transparent here
	}
	if (tube_diffuse_mode == 1)
	{
		// TubeDiffuseImage
		if (adjusted_tube_diffuse_amount > 0 || HSM_CRT_BLEND_MODE == 2)
		{
			vec2 diffuse_coord = tube_diffuse_curved_coord;

			diffuse_coord = HSM_GetMirrorWrappedCoord(diffuse_coord);
			diffuse_coord.x = HSM_FLIP_VIEWPORT_HORIZONTAL * (diffuse_coord.x - 0.5) + 0.5;
			diffuse_coord.y = HSM_FLIP_VIEWPORT_VERTICAL * (diffuse_coord.y - 0.5) + 0.5;

			diffuse_coord = (diffuse_coord - 0.5) / (HSM_TUBE_DIFFUSE_IMAGE_SCALE * vec2(HSM_TUBE_DIFFUSE_IMAGE_SCALE_X, 1)) + 0.5;

			// HSM_RotateCoordinate(diffuse_coord, HSM_TUBE_DIFFUSE_IMAGE_ROTATION)
			tube_diffuse = HSM_GetMipmappedTexSample(TubeDiffuseImage, HSM_RotateCoordinate(diffuse_coord, HSM_TUBE_DIFFUSE_IMAGE_ROTATION), TUBE_SCALE * HSM_TUBE_DIFFUSE_IMAGE_SCALE, 0);
			tube_diffuse.rgb = HSM_GLOBAL_GRAPHICS_BRIGHTNESS * HSM_GetPreMultipliedColorLinear(vec4(tube_diffuse.rgb, 1), SOURCE_MATTE_PREMULTIPLIED, DEFAULT_SRGB_GAMMA).rgb;
			tube_diffuse.rgb = HSM_ApplyHSVAdjustment(tube_diffuse.rgb, HSM_TUBE_DIFFUSE_IMAGE_HUE, HSM_TUBE_DIFFUSE_IMAGE_SATURATION, HSM_TUBE_DIFFUSE_IMAGE_BRIGHTNESS, HSM_TUBE_DIFFUSE_IMAGE_COLORIZE_ON, HSM_TUBE_DIFFUSE_IMAGE_GAMMA);
			tube_diffuse.rgb *= HSM_TUBE_DIFFUSE_IMAGE_AMOUNT;

			tube_diffuse.rgb *= tube_shadow;
		}

		// If CRT Blend Mode is Multiply (2) then the tube must be fully opaque
		tube_diffuse.rgb = HSM_ApplyAmbientImage(tube_diffuse.rgb, ambient_lighting_image.rgb, HSM_TUBE_DIFFUSE_IMAGE_AMBIENT_LIGHTING);
		tube_diffuse.rgb = HSM_ApplyAmbientImage(tube_diffuse.rgb, ambient2_lighting_image.rgb, HSM_TUBE_DIFFUSE_IMAGE_AMBIENT2_LIGHTING);
	}

	tube_diffuse *= HSM_GetTubeOpacity();
	tube_diffuse.a *= tube_mask;

	out_color = tube_diffuse;
	out_color.rgb += crt_color.rgb;

	// Don't apply the CRT images
	if (HSM_CRT_BLEND_MODE == 0)
		out_color = tube_diffuse;

	// Apply the CRT image additively to the tube diffuse color
	if (HSM_CRT_BLEND_MODE == 1)
		// out_color = HSM_BlendModeLayerMix(tube_diffuse, out_color, BLEND_MODE_ADD, HSM_CRT_BLEND_AMOUNT);
		out_color.rgb += tube_diffuse.rgb;

	// Apply the CRT image with multiply blending
	if (HSM_CRT_BLEND_MODE == 2)
	{	
		out_color = clamp(out_color, 0, 1);
		out_color = HSM_BlendModeLayerMix(tube_diffuse, out_color, BLEND_MODE_MULTIPLY, HSM_CRT_BLEND_AMOUNT);
	}

	// Add a Colored Gel image on top of the screen, this is what colors games like space invaders or battlezone
	if (HSM_GetUseTubeColoredGelImage())
	{
		vec2 gel_coord = HSM_GetMirrorWrappedCoord(tube_diffuse_curved_coord);
		gel_coord.x = HSM_TUBE_COLORED_GEL_IMAGE_FLIP_HORIZONTAL * (gel_coord.x - 0.5) + 0.5;
		gel_coord.y = HSM_TUBE_COLORED_GEL_IMAGE_FLIP_VERTICAL * (gel_coord.y - 0.5) + 0.5;
		gel_coord = (gel_coord - 0.5) / HSM_TUBE_COLORED_GEL_IMAGE_SCALE + 0.5;
		vec4 gel_image = HSM_GetMipmappedTexSample(TubeColoredGelImage, gel_coord, TUBE_SCALE, 0);

		gel_image = HSM_GLOBAL_GRAPHICS_BRIGHTNESS * HSM_Linearize(gel_image, DEFAULT_SRGB_GAMMA);

		if (HSM_TUBE_COLORED_GEL_IMAGE_MULTIPLY_AMOUNT > 0)
			out_color.rgb = mix(out_color.rgb, out_color.rgb * gel_image.rgb, HSM_TUBE_COLORED_GEL_IMAGE_MULTIPLY_AMOUNT);

		if (HSM_TUBE_COLORED_GEL_IMAGE_ADDITIVE_AMOUNT > 0)
			out_color.rgb = mix(out_color.rgb, out_color.rgb + gel_image.rgb, HSM_TUBE_COLORED_GEL_IMAGE_ADDITIVE_AMOUNT);

		if (HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT > 0)
		{
			gel_image.a = clamp( gel_image.a - HSM_TUBE_COLORED_GEL_IMAGE_TRANSPARENCY_THRESHOLD, 0, 1 ) / (1 - HSM_TUBE_COLORED_GEL_IMAGE_TRANSPARENCY_THRESHOLD);
			gel_image.rgb *= HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_BRIGHTNESS;
			gel_image.rgb *= tube_shadow;

			if (HSM_TUBE_COLORED_GEL_IMAGE_FAKE_SCANLINE_AMOUNT > 0.001)
				gel_image.rgb = HSM_ApplyScanlineMask(gel_image, SCREEN_SCALE, SCREEN_COORD, screen_curved_coord, tube_curved_coord, HSM_TUBE_COLORED_GEL_IMAGE_FAKE_SCANLINE_AMOUNT).rgb;

			gel_image.rgb = HSM_ApplyAmbientImage(gel_image.rgb, ambient_lighting_image.rgb, HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT_LIGHTING);
			gel_image.rgb = HSM_ApplyAmbientImage(gel_image.rgb, ambient2_lighting_image.rgb, HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT2_LIGHTING);

			gel_image = HSM_BlendModeLayerMix(gel_image, vec4(vec3(vignette_factor), 1), BLEND_MODE_MULTIPLY, HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_VIGNETTE);

			if ( HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL == 0 )
				out_color = HSM_BlendModeLayerMix(out_color, gel_image, BLEND_MODE_NORMAL, HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT);
			
			if ( HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL == 1 )
			{
				gel_image.rgb *= tube_shadow;
				vec4 normal_blended = HSM_BlendModeLayerMix(vec4(0, 0, 0, 1), gel_image, BLEND_MODE_NORMAL, HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT);
				out_color = normal_blended + out_color;
			}
		}
	}

	#ifndef IS_NO_REFLECT_PRESET

	out_color.rgb *= black_edge_mask_invert;

	#else

	out_color.rgb *= tube_diffuse_mask;
	out_color *= tube_mask;

	#endif

	if (HSM_GetUseTubeStaticReflection())
	{
		vec3 tube_highlight_image = HSM_GetTubeHighlight(tube_curved_coord, false, TubeStaticReflectionImage).rgb;
		tube_highlight_image = HSM_ApplyAmbientImage(tube_highlight_image.rgb, ambient_lighting_image.rgb, HSM_TUBE_STATIC_AMBIENT_LIGHTING);
		tube_highlight_image = HSM_ApplyAmbientImage(tube_highlight_image.rgb, ambient2_lighting_image.rgb, HSM_TUBE_STATIC_AMBIENT2_LIGHTING);

		tube_highlight_image.rgb *= HSM_TUBE_STATIC_SHADOW_OPACITY * tube_shadow + (1 - HSM_TUBE_STATIC_SHADOW_OPACITY);

		tube_highlight_image = clamp(HSM_TUBE_STATIC_BLACK_LEVEL * (tube_highlight_image - 1) + 1, 0, 1);

		tube_highlight_image *= 1 - HSM_TUBE_STATIC_OPACITY_DIFFUSE_MULTIPLY + HSM_TUBE_STATIC_OPACITY_DIFFUSE_MULTIPLY * adjusted_tube_diffuse_amount;

		out_color.rgb += tube_highlight_image * HSM_TUBE_STATIC_REFLECTION_IMAGE_OPACITY;
	}
	
	if (HSM_SINDEN_BORDER_ON > 0.5)
	{
		out_color = HSM_Apply_Sinden_Lightgun_Border(out_color, tube_diffuse_curved_coord, tube_diffuse_mask, black_edge_corner_radius);
	}

	return out_color;
}

void main()
{
	if (HSM_AB_COMPARE_FREEZE_CRT_TUBE == 1 && HSM_GetIsInABCompareArea(vTexCoord))
	{
		FragColor = texture(PassFeedback, vTexCoord);
		return;
	}

	VIEWPORT_UNSCALED_COORD = HSM_GetViewportCoordWithFlip(vTexCoord);

	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec2 cache_bounds_coord = TUBE_COORD;

// If it's the potato preset render the whole frame
#ifndef IS_NO_REFLECT_PRESET
	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	vec2 bezel_outside_flat_coord;
	vec2 frame_outside_flat_coord;
	HSM_GetSimpleBezelCoords(TUBE_DIFFUSE_COORD, 
							TUBE_DIFFUSE_SCALE, 
							TUBE_SCALE, 
							SCREEN_ASPECT,
							bezel_outside_flat_coord, 
							frame_outside_flat_coord);
	cache_bounds_coord = (bezel_outside_flat_coord - 0.5) * 0.9 + 0.5;
#endif

	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
	{
			FragColor = vec4(0);
			return;
	}

	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
						TUBE_DIFFUSE_SCALE, 
						TUBE_SCALE, 
						SCREEN_ASPECT, 
						false,
						BEZEL_OUTSIDE_SCALE,
						BEZEL_OUTSIDE_COORD, 
						BEZEL_OUTSIDE_CURVED_COORD, 
						FRAME_OUTSIDE_CURVED_COORD);

	vec2 screen_curved_coord = HSM_GetCurvedCoord(SCREEN_COORD, 1, SCREEN_ASPECT);

	FragColor = texture(Source, UNFLIPPED_VIEWPORT_COORD);

	FragColor = HSM_GetPostCrtPreppedColor(FragColor, vTexCoord, screen_curved_coord, DerezedPass, TubeDiffuseImage, TubeColoredGelImage, TubeStaticReflectionImage, BackgroundImage, BackgroundVertImage, NightLightingImage, NightLighting2Image);
}
