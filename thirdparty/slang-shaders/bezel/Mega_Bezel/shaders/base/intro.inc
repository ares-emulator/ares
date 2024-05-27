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

// Import so all the mega bezel parameters are the first in the parameter list

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.00001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#ifndef IS_POTATO_PRESET
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 7) uniform sampler2D IntroImage;
#endif

layout(set = 0, binding = 8) uniform sampler2D InfoCachePass;

layout(set = 0, binding = 10) uniform sampler2D TextPass;

#define eps 1e-3

#ifndef IS_POTATO_PRESET
vec4 AntiFlicker(vec2 in_coord, vec4 current)
{
	// sample the textures
	vec4 prev1 = texture(OriginalHistory1, in_coord);
	vec4 prev2 = texture(OriginalHistory2, in_coord);
	vec4 prev3 = texture(OriginalHistory3, in_coord);
	
	// get luma for comparison
	float cur_lum = dot(current.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev1_lum = dot(prev1.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev2_lum = dot(prev2.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev3_lum = dot(prev3.rgb, vec3(0.2125, 0.7154, 0.0721));
	
	// Test whether the luma difference between the pixel in the current frame and that of
	// the previous frame exceeds the threshold while the difference between the current frame
	// and 2 frames previous is below the threshold.
	// Repeat the process for the previous frame's pixel to reduce false-positives
	bool flicker = (abs(cur_lum - prev1_lum) > HSM_ANTI_FLICKER_THRESHOLD && abs(cur_lum - prev2_lum) < HSM_ANTI_FLICKER_THRESHOLD) &&
		(abs(prev1_lum - prev2_lum) > HSM_ANTI_FLICKER_THRESHOLD && abs(prev1_lum - prev3_lum) < HSM_ANTI_FLICKER_THRESHOLD);
	
	// Average the current frame with the previous frame in linear color space to avoid over-darkening
	vec4 blended = (pow(current, vec4(2.2)) + pow(prev1, vec4(2.2))) / 2.0;
	
	// delinearize the averaged result
	blended = pow(blended, vec4(1.0 / 2.2));
	
	return (!flicker) ? current : blended;
}

// -----------------------------------------------------------------------------
// TV Startup Static Animation
// -----------------------------------------------------------------------------
vec4 GetColorWithIntro(vec4 in_color, vec2 in_coord, in sampler2D in_logo_image_sampler2d)
{
   vec4 out_color = in_color;

   // If we are out of the region of time the intro plays just give back the same color
   if (CURRENT_FRAME_FROM_CACHE_INFO > (HSM_INTRO_SOLID_COLOR_HOLD + 
                                       HSM_INTRO_SOLID_COLOR_FADE_OUT + 
                                       HSM_INTRO_LOGO_WAIT + 
                                       HSM_INTRO_LOGO_FADE_IN +
                                       HSM_INTRO_LOGO_HOLD +
                                       HSM_INTRO_LOGO_FADE_OUT +
                                       HSM_INTRO_NOISE_HOLD +
                                       HSM_INTRO_NOISE_FADE_OUT + 
                                       HSM_INTRO_SOLID_BLACK_HOLD +
                                       HSM_INTRO_SOLID_BLACK_FADE_OUT) / HSM_INTRO_SPEED)
      return out_color;

   in_coord.y = HSM_FLIP_VIEWPORT_VERTICAL * HSM_FLIP_CORE_VERTICAL * in_coord.y;
   in_coord.x = HSM_FLIP_VIEWPORT_HORIZONTAL * HSM_FLIP_CORE_HORIZONTAL * in_coord.x;

   if (abs(HSM_ROTATE_CORE_IMAGE) == 1)
   {
      in_coord.xy = in_coord.yx;

      if (HSM_ROTATE_CORE_IMAGE == 1)
         in_coord.y = (in_coord.y - 0.5) * -1 + 0.5;
      else if (HSM_ROTATE_CORE_IMAGE == -1)
         in_coord.x = (in_coord.x - 0.5) * -1 + 0.5;
   }

   float frame_count = HSM_INTRO_WHEN_TO_SHOW == 2 ? CURRENT_FRAME_FROM_CACHE_INFO : global.FrameCount;

   float mask_wait = 20 / HSM_INTRO_SPEED;
   
	float mask_fade_horizontal = 40 / HSM_INTRO_SPEED;
	float mask_fade_vertical = 60 / HSM_INTRO_SPEED;
	float mask_vert_wait = mask_fade_horizontal * 0.4;

   vec4 solid_color = vec4(HSM_HSVtoRGB(vec3(HSM_INTRO_SOLID_COLOR_HUE, HSM_INTRO_SOLID_COLOR_SAT, HSM_INTRO_SOLID_COLOR_VALUE)), 1);
   // vec4 solid_color = HSM_Linearize(vec4(HSM_HSVtoRGB(vec3(HSM_BZL_COLOR_HUE, HSM_BZL_COLOR_SATURATION, HSM_BZL_COLOR_VALUE)), 1), DEFAULT_SRGB_GAMMA).rgb;

	float solid_color_hold = HSM_INTRO_SOLID_COLOR_HOLD / HSM_INTRO_SPEED;
	float solid_color_fade_out = HSM_INTRO_SOLID_COLOR_FADE_OUT / HSM_INTRO_SPEED;

   float intro_logo_wait = HSM_INTRO_LOGO_WAIT / HSM_INTRO_SPEED;
   float intro_logo_fade_in = HSM_INTRO_LOGO_FADE_IN / HSM_INTRO_SPEED;
   float intro_logo_hold = HSM_INTRO_LOGO_HOLD / HSM_INTRO_SPEED;
   float intro_logo_fade_out = HSM_INTRO_LOGO_FADE_OUT / HSM_INTRO_SPEED;

	float noise_hold = HSM_INTRO_NOISE_HOLD / HSM_INTRO_SPEED;
	float noise_fade_out = HSM_INTRO_NOISE_FADE_OUT / HSM_INTRO_SPEED;

   // Black Hold starts from end of Static Hold
   float black_hold = HSM_INTRO_SOLID_BLACK_HOLD / HSM_INTRO_SPEED;
	float black_fade_out = HSM_INTRO_SOLID_BLACK_FADE_OUT / HSM_INTRO_SPEED;

   float power_on_seq_length = mask_wait + mask_vert_wait + mask_fade_vertical;
   float fade_out_seq_length = max(noise_fade_out, black_hold + black_fade_out);
   float logo_seq_length     = intro_logo_wait + intro_logo_fade_in + intro_logo_hold + intro_logo_fade_out;
   float full_seq_length     = max(power_on_seq_length + noise_hold + fade_out_seq_length, logo_seq_length);

   // Loop the animation so we can see the results
   if (HSM_INTRO_WHEN_TO_SHOW == 3)
      frame_count = mod(frame_count, full_seq_length + 15);

	if (frame_count <= full_seq_length)
	{
      vec2 noise_res = global.SourceSize.xy;
      noise_res = min(noise_res, vec2(640, 480));

      if (abs(HSM_ROTATE_CORE_IMAGE) > 0.5)
         noise_res.xy = noise_res.yx;

		vec2 quantized_coord = vec2(floor((in_coord.x + 0.01) * noise_res.x) / noise_res.x, floor((in_coord.y + 0.01) * noise_res.y) / noise_res.y);

		float width = (frame_count - mask_wait) / (mask_fade_horizontal/2);
		float height = (clamp((frame_count - mask_wait - mask_vert_wait) / mask_fade_vertical, 0, 1) + 0.005) * 2;
		float mask = abs(in_coord.x - 0.5) > width * 0.5 ? 0 : 1;
		mask *= HHLP_EasePowerOut(abs(in_coord.y - 0.5), 1.5) > height ? 0 : 1;

		float area = width * height;

      float black_opacity = HHLP_EasePowerIn(clamp(1 - (frame_count - (power_on_seq_length + noise_hold + black_hold)) / black_fade_out, 0, 1), 1.5);
      out_color = mix(out_color, vec4(0, 0, 0, 1), black_opacity);

		float solid_color_opacity = HHLP_EasePowerIn(clamp(1 - (frame_count - (mask_wait + mask_fade_horizontal/2.5 + solid_color_hold)) / solid_color_fade_out * 2, 0, 1), 1.5);
      // out_color = mix(out_color, vec4(0, 0, 0, 1), solid_color_opacity);

		float noise_opacity =  1 - clamp((frame_count - (power_on_seq_length + noise_hold)) / noise_fade_out, 0, 1);
		noise_opacity = HHLP_EasePowerInOut(noise_opacity, 4);

		// Calculate Static Noise
		float e = 2.7182818284590452353602874713527;
		float G = e + (mod(frame_count, 30)/30 + 1000);
		vec2 r = (G * sin(G * quantized_coord.xy));
		vec4 noise_color = vec4(fract(r.x * r.y * (1.0 + quantized_coord.x)));
		noise_color.a = 1;

		noise_color.rgb *= 0.60;

		out_color = HSM_BlendModeLayerMix(out_color, noise_color, HSM_INTRO_NOISE_BLEND_MODE, noise_opacity);

      vec2 intro_image_res = textureSize(in_logo_image_sampler2d, 0);
      float intro_image_aspect = intro_image_res.x/intro_image_res.y;

      float intro_logo_height = HSM_INTRO_LOGO_HEIGHT;
      if (intro_logo_height == 0)
         intro_logo_height = abs(HSM_ROTATE_CORE_IMAGE) > 0.5 ? intro_image_res.y / global.OriginalSize.y
                                                         : intro_image_res.y / global.OriginalSize.x;

      vec2 intro_image_scale = vec2(intro_logo_height * intro_image_aspect / SCREEN_ASPECT, intro_logo_height) * min(SCREEN_ASPECT, 1);
      intro_image_scale = floor(intro_image_scale * global.SourceSize.xy) / global.SourceSize.xy;

      if (HSM_DUALSCREEN_MODE > 0.5)
      {
         if (HSM_GetCoreImageSplitDirection() == 1)
         {
            in_coord.y = in_coord.y * 2 - MAX_NEGATIVE_CROP;
         }
         if (HSM_GetCoreImageSplitDirection() == 2)
         {
            in_coord.x = in_coord.x * 2 - MAX_NEGATIVE_CROP;
         }
      }

      in_coord.x -= HSM_INTRO_LOGO_POS_X;
      in_coord.y += HSM_INTRO_LOGO_POS_Y;

      // Center
      vec2 coord_in_intro_image = (in_coord - 0.5) / intro_image_scale + 0.5;
      // Top Left
      if (HSM_INTRO_LOGO_PLACEMENT == 1)
         coord_in_intro_image = in_coord / intro_image_scale;
      // Top Right
      if (HSM_INTRO_LOGO_PLACEMENT == 2)
         coord_in_intro_image = (in_coord - vec2(1 - intro_image_scale.x, 0)) / intro_image_scale;
      // Bottom Left
      if (HSM_INTRO_LOGO_PLACEMENT == 3)
         coord_in_intro_image = (in_coord - vec2(0, 1 - intro_image_scale.y)) / intro_image_scale;
      // Bottom Right
      if (HSM_INTRO_LOGO_PLACEMENT == 4)
         coord_in_intro_image = (in_coord - (1 - intro_image_scale)) / intro_image_scale;

      // Quilez gives a better rescaling weighted more to the center of the pixel like bicubic
      vec4 intro_image = HSM_TextureQuilez(IntroImage, intro_image_res, coord_in_intro_image);
      float logo_opacity = 1 - HHLP_EasePowerOut(clamp(1 - (frame_count - mask_wait - mask_fade_horizontal/4) / intro_logo_fade_in, 0, 1), 1.5);
      logo_opacity *=  clamp(clamp((frame_count - intro_logo_wait) / intro_logo_fade_in, 0, 1) - 
                              clamp((frame_count - (intro_logo_wait + intro_logo_fade_in + intro_logo_hold)) / intro_logo_fade_out, 0, 1), 
                              0, 1);

      // Logo UNDER solid color
      if (HSM_INTRO_LOGO_OVER_SOLID_COLOR == 0)
         out_color = HSM_BlendModeLayerMix(out_color, intro_image, HSM_INTRO_LOGO_BLEND_MODE, logo_opacity);

		out_color = HSM_BlendModeLayerMix(out_color, solid_color, HSM_INTRO_SOLID_COLOR_BLEND_MODE, solid_color_opacity);

      // Logo OVER solid color
      if (HSM_INTRO_LOGO_OVER_SOLID_COLOR == 1)
         out_color = HSM_BlendModeLayerMix(out_color, intro_image, HSM_INTRO_LOGO_BLEND_MODE, logo_opacity);

		out_color *= mask;

      HSM_Delinearize(out_color, DEFAULT_SRGB_GAMMA);
	}
   return out_color;
}
#endif

// vec4 GetCropOverlay(vec2 viewport_coord)
// {
//    vec4 out_color = vec4(0);
//    vec4 screen_color = vec4(1, 0, 0, 1);
   
//    for (int i=1; i < 3; i++)
//    {
//       vec2 cropped_rotated_size = vec2(0);
//       vec2 cropped_rotated_size_with_res_mult = vec2(0);
//       vec2 cropped_sample_area_start_pixel_coord = vec2(0);
//       HSM_GetCroppedRotatedSizeAndPixelSampleAreaStart(i, Source, cropped_rotated_size, cropped_rotated_size_with_res_mult, cropped_sample_area_start_pixel_coord);

//       vec2 crop_scale = cropped_rotated_size / ROTATED_CORE_PREPPED_SIZE;
//       vec2 mask_coord = viewport_coord - cropped_sample_area_start_pixel_coord / ROTATED_CORE_PREPPED_SIZE;
//       mask_coord = mask_coord / crop_scale;
      
//       if (i == 2)
//          screen_color = vec4(0, 1, 0, 1);

//       out_color += screen_color * HSM_GetCornerMask(mask_coord, cropped_rotated_size.x/cropped_rotated_size.y, 0, 0.90);
//    }

//    return clamp(out_color, 0, 1);
// }

void main()
{
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	// Flip the coordinate vertically if desired
   vec2 viewport_coord_adjusted = vTexCoord;
	viewport_coord_adjusted.x = HSM_FLIP_VIEWPORT_HORIZONTAL * HSM_FLIP_CORE_HORIZONTAL * (viewport_coord_adjusted.x - 0.5) + 0.5;
	viewport_coord_adjusted.y = HSM_FLIP_VIEWPORT_VERTICAL * HSM_FLIP_CORE_VERTICAL * (viewport_coord_adjusted.y - 0.5) + 0.5;

   FragColor = texture(Source, viewport_coord_adjusted);

#ifndef IS_POTATO_PRESET
   // if (HSM_ANTI_FLICKER_ON == 1)
   //    FragColor = AntiFlicker(viewport_coord_adjusted, FragColor);

   if (HSM_INTRO_WHEN_TO_SHOW > 0)
      FragColor = GetColorWithIntro(FragColor, viewport_coord_adjusted, IntroImage);
#endif

   bool cache_info_changed = CACHE_INFO_CHANGED;
   bool show_update_indicator = cache_info_changed && ( HSM_CACHE_UPDATE_INDICATOR_MODE == 1 || HSM_CACHE_GRAPHICS_ON < 0.5 && HSM_CACHE_UPDATE_INDICATOR_MODE == 2);

	if (show_update_indicator)
	{
		float final_aspect = SCREEN_ASPECT;
		// vec2 corner_offset = vec2(MAX_NEGATIVE_CROP * 2 * 0.8, MAX_NEGATIVE_CROP * 2 * 0.8);
		vec2 corner_offset = vec2(0.1, 0.1);
		vec2 center_coord = vec2(1 - corner_offset.x * final_aspect, 1 - corner_offset.y);
		vec2 test_coord = vec2(1 - (1 - viewport_coord_adjusted.x) * final_aspect, viewport_coord_adjusted.y);
		float distance =  length(test_coord - center_coord);

		if (distance < 0.04)
		{
			float modulation = clamp(0.4 + abs(mod(global.FrameCount, 90) / 90 - 0.5) * 1.2, 0, 1);
			modulation = HHLP_EasePowerInOut(modulation, 2);
			FragColor *= 1 - modulation;
			FragColor += modulation * vec4(1, 0, 0, 1);
		}
	}
   
   // vec4 overlay_color = GetCropOverlay(vTexCoord);
   // FragColor *= 1 - 0.7 * overlay_color.a;
   // FragColor += 1 * 0.7 * overlay_color;

   //////// Draw text to show resolutions ////////// 
   if (HSM_RESOLUTION_DEBUG_ON == 1)
   {
      vec2 ps = global.DerezedPassSize.zw;
		vec2 outline_offset_mult = ps;
		outline_offset_mult.x *= max(floor(global.DerezedPassSize.x / 240 + 0.01), 1);
		outline_offset_mult.y *= max(floor(global.DerezedPassSize.y / 200 + 0.01), 1);

      vec4 text_rgba = vec4(0);
      text_rgba = texture(TextPass, vTexCoord);
      text_rgba.rgb *= vec3(1, 1, 0);
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(1, 0)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(0, 1)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(1, 1)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(-1, 1)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(-1, 0)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(0, -1)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(-1, -1)).a;
      text_rgba.a += texture(TextPass, vTexCoord + outline_offset_mult * vec2(1, -1)).a;
      text_rgba = clamp(text_rgba, 0, 1);
      FragColor = HSM_PreMultAlphaBlend(FragColor, text_rgba);
   }
}