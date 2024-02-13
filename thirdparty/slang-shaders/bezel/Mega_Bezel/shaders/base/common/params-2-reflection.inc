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
//    REFLECTION
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_REFLECTION_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECTION_TITLE			                "[ REFLECTION ]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_GLOBAL_AMOUNT 				    "          Global Amount" 40 0 1000 1
float HSM_REFLECT_GLOBAL_AMOUNT = global.HSM_REFLECT_GLOBAL_AMOUNT / 100;

#pragma parameter HSM_REFLECT_GLOBAL_GAMMA_ADJUST 			    "          Global Gamma Adjust" 1.2 0.25 3 0.05
float HSM_REFLECT_GLOBAL_GAMMA_ADJUST = global.HSM_REFLECT_GLOBAL_GAMMA_ADJUST;

#pragma parameter HSM_REFLECT_DIRECT_AMOUNT 				    "          Direct Reflection" 150 0 300 2
float HSM_REFLECT_DIRECT_AMOUNT = global.HSM_REFLECT_DIRECT_AMOUNT / 100;

#pragma parameter HSM_REFLECT_DIFFUSED_AMOUNT 				    "          Diffused Reflection" 50 0 300 2
float HSM_REFLECT_DIFFUSED_AMOUNT = global.HSM_REFLECT_DIFFUSED_AMOUNT / 100;

#pragma parameter HSM_REFLECT_FULLSCREEN_GLOW 				    "          FullScreen Glow" 75 0 600 0.5
float HSM_REFLECT_FULLSCREEN_GLOW = global.HSM_REFLECT_FULLSCREEN_GLOW / 100;

#pragma parameter HSM_REFLECT_FULLSCREEN_GLOW_GAMMA 		    "          FullScreen Glow Gamma" 3 0 5 0.02
float HSM_REFLECT_FULLSCREEN_GLOW_GAMMA = global.HSM_REFLECT_FULLSCREEN_GLOW_GAMMA;

#pragma parameter HSM_REFLECT_BEZEL_INNER_EDGE_AMOUNT 			"          Bezel Inner Edge Amount" 130 0 300 2
float HSM_REFLECT_BEZEL_INNER_EDGE_AMOUNT = global.HSM_REFLECT_BEZEL_INNER_EDGE_AMOUNT / 100;

#pragma parameter HSM_REFLECT_BEZEL_INNER_EDGE_FULLSCREEN_GLOW 	"          Bezel Inner Edge Fullscreen Glow" 40 0 1000 1
float HSM_REFLECT_BEZEL_INNER_EDGE_FULLSCREEN_GLOW = global.HSM_REFLECT_BEZEL_INNER_EDGE_FULLSCREEN_GLOW / 100;

#pragma parameter HSM_REFLECT_FRAME_INNER_EDGE_AMOUNT 		    "          Frame Inner Edge Amount" 50 0 300 2
float HSM_REFLECT_FRAME_INNER_EDGE_AMOUNT = global.HSM_REFLECT_FRAME_INNER_EDGE_AMOUNT / 100;

#pragma parameter HSM_REFLECT_FRAME_INNER_EDGE_SHARPNESS 	    "          Frame Inner Edge Sharpness" 90 0 90 2
float HSM_REFLECT_FRAME_INNER_EDGE_SHARPNESS = global.HSM_REFLECT_FRAME_INNER_EDGE_SHARPNESS / 100;

#pragma parameter HSM_REFLECT_SHOW_TUBE_FX_AMOUNT 	            "          Show Tube Effects in Reflection Amount" 100 0 100 2
float HSM_REFLECT_SHOW_TUBE_FX_AMOUNT = global.HSM_REFLECT_SHOW_TUBE_FX_AMOUNT / 100;

//---------------------------------------------------------------------------------------------------
// Reflection Fading
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_REFLECTION_FADE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECTION_FADE_TITLE			            "[ REFLECTION FADE ]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_FADE_AMOUNT 					    "          Fade Amount" 100 0 100 5
float HSM_REFLECT_FADE_AMOUNT = global.HSM_REFLECT_FADE_AMOUNT / 100;

#pragma parameter HSM_REFLECT_RADIAL_FADE_WIDTH 			    "          Radial Fade Width" 100 0 1000 1
float HSM_REFLECT_RADIAL_FADE_WIDTH = global.HSM_REFLECT_RADIAL_FADE_WIDTH / 350;

#pragma parameter HSM_REFLECT_RADIAL_FADE_HEIGHT 				"          Radial Fade Height" 100 0 1000 1
float HSM_REFLECT_RADIAL_FADE_HEIGHT = global.HSM_REFLECT_RADIAL_FADE_HEIGHT / 350;

#pragma parameter HSM_REFLECT_LATERAL_OUTER_FADE_POSITION 		"          Lateral Outer Fade Position" -18 -500 500 0.5
float HSM_REFLECT_LATERAL_OUTER_FADE_POSITION = global.HSM_REFLECT_LATERAL_OUTER_FADE_POSITION / -1000;

#pragma parameter HSM_REFLECT_LATERAL_OUTER_FADE_DISTANCE 		"          Lateral Outer Fade Distance" 20 1 400 0.5
float HSM_REFLECT_LATERAL_OUTER_FADE_DISTANCE = global.HSM_REFLECT_LATERAL_OUTER_FADE_DISTANCE / 100;


//---------------------------------------------------------------------------------------------------
// Reflection Blur
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_REFLECT_BLUR_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECT_BLUR_TITLE			                "[ REFLECTION BLUR ]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_BLUR_NUM_SAMPLES 					    "          Blur Samples - 0 for OFF" 12 0 40 1
float HSM_REFLECT_BLUR_NUM_SAMPLES = global.HSM_REFLECT_BLUR_NUM_SAMPLES;

#pragma parameter HSM_REFLECT_BLUR_MIN 							    "          Min Blur" 	0 0 100 1
float HSM_REFLECT_BLUR_MIN = global.HSM_REFLECT_BLUR_MIN / 100;

#pragma parameter HSM_REFLECT_BLUR_MAX 							    "          Max Blur" 	95 0 100 1
float HSM_REFLECT_BLUR_MAX = global.HSM_REFLECT_BLUR_MAX / 100;

#pragma parameter HSM_REFLECT_BLUR_FALLOFF_DISTANCE 				"          Falloff Distance" 	100 0 1000 1
float HSM_REFLECT_BLUR_FALLOFF_DISTANCE = global.HSM_REFLECT_BLUR_FALLOFF_DISTANCE / 100;

// Reflection Noise
#pragma parameter HSM_REFLECT_NOISE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECT_NOISE_TITLE			                "[ REFLECTION NOISE ]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_NOISE_AMOUNT 						    "          Noise Amount" 50 0 100 1
float HSM_REFLECT_NOISE_AMOUNT = global.HSM_REFLECT_NOISE_AMOUNT / 100;

#pragma parameter HSM_REFLECT_NOISE_SAMPLES 					    "          Noise Samples - 0 | 1 | 2 | 12" 1 0 3 1
float HSM_REFLECT_NOISE_SAMPLES = global.HSM_REFLECT_NOISE_SAMPLES;

#pragma parameter HSM_REFLECT_NOISE_SAMPLE_DISTANCE 			    "          Sample Distance" 100 0 500 5
float HSM_REFLECT_NOISE_SAMPLE_DISTANCE = global.HSM_REFLECT_NOISE_SAMPLE_DISTANCE / 500;

// #pragma parameter HSM_REFLECT_BRIGHTNESS_NOISE_BLACK_LEVEL                      "          Mask Black Level" 0 -100 100 0.5
// float HSM_REFLECT_BRIGHTNESS_NOISE_BLACK_LEVEL = (global.HSM_REFLECT_BRIGHTNESS_NOISE_BLACK_LEVEL / 1000 + 1);

// #pragma parameter HSM_REFLECT_BRIGHTNESS_NOISE_BRIGHTNESS                      "          Mask Brightness" 100 0 200 1
// float HSM_REFLECT_BRIGHTNESS_NOISE_BRIGHTNESS = global.HSM_REFLECT_BRIGHTNESS_NOISE_BRIGHTNESS / 100;

//---------------------------------------------------------------------------------------------------
//    REFLECTION MASK
//---------------------------------------------------------------------------------------------------
#include "params-2-reflection-mask.inc"


// Glass Preset Stuff
#pragma parameter HSM_REFLECT_GLASS_EMPTY_LINE						" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECT_GLASS_TITLE			                "[ REFLECTION GLASS ]:" 0 0 0.01 0.01

#pragma parameter HSM_GLASS_BORDER_ON                               "          Glass Border ON (For Glass Preset)" 0 0 1 1
float HSM_GLASS_BORDER_ON = global.HSM_GLASS_BORDER_ON;

#pragma parameter HSM_REFLECT_VIGNETTE_AMOUNT 		                "          Glass Reflection Vignette" 0 0 100 0.5
float HSM_REFLECT_VIGNETTE_AMOUNT = global.HSM_REFLECT_VIGNETTE_AMOUNT / 100;

#pragma parameter HSM_REFLECT_VIGNETTE_SIZE 		                "          Glass Reflection Vignette Size" 100 50 200 2
float HSM_REFLECT_VIGNETTE_SIZE = global.HSM_REFLECT_VIGNETTE_SIZE / 100;


/////////////// CONSTANT TUNING SETTINGS ///////////////

// #ifndef HSM_REFLECT_CORNER_SPREAD_FALLOFF
//     #define HSM_REFLECT_CORNER_SPREAD_FALLOFF 100
// #endif