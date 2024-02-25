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

//---------------------------------------------------------------------------------------------------
//    REFLECTION MASK
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_REFLECT_MASK_EMPTY_LINE					    " " 0 0 0.001 0.001
#pragma parameter HSM_REFLECT_MASK_TITLE			                "[ REFLECTION MASK IMAGE - Only in Image Layer Presets ]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_MASK_IMAGE_AMOUNT 		            "          Reflection Image Mask Amount" 0 0 100 1
#ifndef HSM_REFLECT_MASK_IMAGE_AMOUNT 
#define HSM_REFLECT_MASK_IMAGE_AMOUNT global.HSM_REFLECT_MASK_IMAGE_AMOUNT / 100
#endif

#pragma parameter HSM_REFLECT_MASK_FOLLOW_LAYER                    "    Follow Layer - FULL | TUBE | BEZEL |BG|DEVICE|DECAL|CAB|TOP" 4 0 7 1
#ifndef HSM_REFLECT_MASK_FOLLOW_LAYER
#define HSM_REFLECT_MASK_FOLLOW_LAYER global.HSM_REFLECT_MASK_FOLLOW_LAYER
#endif

#pragma parameter HSM_REFLECT_MASK_FOLLOW_MODE                      "          Follow Mode - FOLLOW SCALE & POS | FOLLOW EXACTLY" 1 0 1 1
#ifndef HSM_REFLECT_MASK_FOLLOW_MODE
#define HSM_REFLECT_MASK_FOLLOW_MODE global.HSM_REFLECT_MASK_FOLLOW_MODE
#endif

#pragma parameter HSM_REFLECT_MASK_BRIGHTNESS                      "          Mask Brightness" 100 0 200 1
#ifndef HSM_REFLECT_MASK_BRIGHTNESS
#define HSM_REFLECT_MASK_BRIGHTNESS global.HSM_REFLECT_MASK_BRIGHTNESS / 100
#endif

#pragma parameter HSM_REFLECT_MASK_BLACK_LEVEL                      "          Mask Black Level" 0 -100 100 0.5
#ifndef HSM_REFLECT_MASK_BLACK_LEVEL
#define HSM_REFLECT_MASK_BLACK_LEVEL (global.HSM_REFLECT_MASK_BLACK_LEVEL / 1000 + 1)
#endif

#pragma parameter HSM_REFLECT_MASK_MIPMAPPING_BLEND_BIAS            "          Mipmapping Blend Bias (Affects Sharpness)" 0 -5 5 0.1
#ifndef HSM_REFLECT_MASK_MIPMAPPING_BLEND_BIAS
#define HSM_REFLECT_MASK_MIPMAPPING_BLEND_BIAS global.HSM_REFLECT_MASK_MIPMAPPING_BLEND_BIAS
#endif