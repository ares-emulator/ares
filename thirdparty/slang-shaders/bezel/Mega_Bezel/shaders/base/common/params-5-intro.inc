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
//    INTRO IMAGE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_INTRO_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTRO_IMAGE_TITLE							"[ INTRO SEQUENCE ]:" 0 0 0.01 0.01

#pragma parameter HSM_INTRO_WHEN_TO_SHOW     					"          When to Show Intro - OFF|Game Load|Shader Load|Repeat"  1 0 3 1
#define HSM_INTRO_WHEN_TO_SHOW global.HSM_INTRO_WHEN_TO_SHOW

#pragma parameter HSM_INTRO_SPEED     							"          Speed"  100 1 500 5
#define HSM_INTRO_SPEED (global.HSM_INTRO_SPEED / 100)


#pragma parameter HSM_INTRO_LOGO_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTRO_LOGO_TITLE							"[ INTRO LOGO ]:" 0 0 0.01 0.01

#pragma parameter HSM_INTRO_LOGO_BLEND_MODE        				"          Logo Blend Mode - OFF | NORM | ADD | MULT"  1 0 3 1
#define HSM_INTRO_LOGO_BLEND_MODE global.HSM_INTRO_LOGO_BLEND_MODE

// #pragma parameter HSM_INTRO_LOGO_FLIP_VERTICAL        		"          Logo Flip Vertical"  0 0 1 1
// #define HSM_INTRO_LOGO_FLIP_VERTICAL global.HSM_INTRO_LOGO_FLIP_VERTICAL

#pragma parameter HSM_INTRO_LOGO_OVER_SOLID_COLOR       		"          Logo Over Solid Color - Off | ON"  1 0 1 1
#define HSM_INTRO_LOGO_OVER_SOLID_COLOR global.HSM_INTRO_LOGO_OVER_SOLID_COLOR

#pragma parameter HSM_INTRO_LOGO_HEIGHT        					"          Logo Height (0 for exact resolution)"  42.86 0 100 1
#define HSM_INTRO_LOGO_HEIGHT (global.HSM_INTRO_LOGO_HEIGHT / 100)

#pragma parameter HSM_INTRO_LOGO_PLACEMENT        				"          Logo Placement - Middle | TL | TR | BL | BR"  0 0 4 1
#define HSM_INTRO_LOGO_PLACEMENT global.HSM_INTRO_LOGO_PLACEMENT

#pragma parameter HSM_INTRO_LOGO_POS_X        					"          Logo Pos X"  0 -100 100 0.5
#define HSM_INTRO_LOGO_POS_X global.HSM_INTRO_LOGO_POS_X / 100

#pragma parameter HSM_INTRO_LOGO_POS_Y        					"          Logo Pos Y"  2 -100 100 0.5
#define HSM_INTRO_LOGO_POS_Y global.HSM_INTRO_LOGO_POS_Y / 100

// #pragma parameter HSM_INTRO_LOGO_WAIT     					"          Logo Wait Before Start Frames"  20 0 1200 5
// #define HSM_INTRO_LOGO_WAIT global.HSM_INTRO_LOGO_WAIT
#define HSM_INTRO_LOGO_WAIT 20

// #pragma parameter HSM_INTRO_LOGO_FADE_IN       				"          Logo Fade In Frames"  20 0 600 5
// #define HSM_INTRO_LOGO_FADE_IN global.HSM_INTRO_LOGO_FADE_IN
#define HSM_INTRO_LOGO_FADE_IN 20

// #pragma parameter HSM_INTRO_LOGO_HOLD     					"          Logo Hold Frames"  130 0 600 5
// #define HSM_INTRO_LOGO_HOLD global.HSM_INTRO_LOGO_HOLD
#define HSM_INTRO_LOGO_HOLD 130

// #pragma parameter HSM_INTRO_LOGO_FADE_OUT       				"          Logo Fade Out Frames"  80 0 600 5
// #define HSM_INTRO_LOGO_FADE_OUT global.HSM_INTRO_LOGO_FADE_OUT
#define HSM_INTRO_LOGO_FADE_OUT 80


#pragma parameter HSM_INTRO_SOLID_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTRO_LOGO_SOLID_TITLE					"[ INTRO SOLID COLOR ]:" 0 0 0.01 0.01

#pragma parameter HSM_INTRO_SOLID_COLOR_BLEND_MODE      		"          Solid Color Blend Mode - OFF | NORM | ADD | MULT"  1 0 3 1
#define HSM_INTRO_SOLID_COLOR_BLEND_MODE global.HSM_INTRO_SOLID_COLOR_BLEND_MODE

#pragma parameter HSM_INTRO_SOLID_COLOR_HUE     				"          Solid Color Hue"  0 0 360 1
#define HSM_INTRO_SOLID_COLOR_HUE (global.HSM_INTRO_SOLID_COLOR_HUE / 360)

#pragma parameter HSM_INTRO_SOLID_COLOR_SAT     				"          Solid Color Saturation"  0 0 100 5
#define HSM_INTRO_SOLID_COLOR_SAT (global.HSM_INTRO_SOLID_COLOR_SAT / 100)

#pragma parameter HSM_INTRO_SOLID_COLOR_VALUE     				"          Solid Color Value"  100 0 100 5
#define HSM_INTRO_SOLID_COLOR_VALUE (global.HSM_INTRO_SOLID_COLOR_VALUE / 100)

// #pragma parameter HSM_INTRO_SOLID_COLOR_HOLD     			"          Solid Color Hold Frames"  40 0 600 5
// #define HSM_INTRO_SOLID_COLOR_HOLD global.HSM_INTRO_SOLID_COLOR_HOLD
#define HSM_INTRO_SOLID_COLOR_HOLD 40

// #pragma parameter HSM_INTRO_SOLID_COLOR_FADE_OUT        		"          Solid Color Fade Out Frames"  40 0 600 5
// #define HSM_INTRO_SOLID_COLOR_FADE_OUT global.HSM_INTRO_SOLID_COLOR_FADE_OUT
#define HSM_INTRO_SOLID_COLOR_FADE_OUT 40


#pragma parameter HSM_INTRO_NOISE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTRO_NOISE_TITLE							"[ INTRO STATIC NOISE ]:" 0 0 0.01 0.01

#pragma parameter HSM_INTRO_NOISE_BLEND_MODE        			"          Static Noise Blend Mode - OFF | NORM | ADD | MULT"  1 0 3 1
#define HSM_INTRO_NOISE_BLEND_MODE global.HSM_INTRO_NOISE_BLEND_MODE

// #pragma parameter HSM_INTRO_NOISE_HOLD     					"          Static Noise Hold Frames"  80 0 600 5
// #define HSM_INTRO_NOISE_HOLD global.HSM_INTRO_NOISE_HOLD
#define HSM_INTRO_NOISE_HOLD 80

// #pragma parameter HSM_INTRO_NOISE_FADE_OUT       			"          Static Noise Fade Out Frames"  120 0 600 5
// #define HSM_INTRO_NOISE_FADE_OUT global.HSM_INTRO_NOISE_FADE_OUT
#define HSM_INTRO_NOISE_FADE_OUT 120

#pragma parameter HSM_INTRO_SOLID_BLACK_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTRO_SOLID_BLACK_TITLE					"[ INTRO SOLID BLACK ]:" 0 0 0.01 0.01

#pragma parameter HSM_INTRO_SOLID_BLACK_HOLD     				"          Solid Black Hold Frames"  0 0 600 5
#define HSM_INTRO_SOLID_BLACK_HOLD global.HSM_INTRO_SOLID_BLACK_HOLD

// #pragma parameter HSM_INTRO_SOLID_BLACK_FADE_OUT       			"          Solid Black Fade Out Frames"  0 0 600 5
// #define HSM_INTRO_SOLID_BLACK_FADE_OUT global.HSM_INTRO_SOLID_BLACK_FADE_OUT
#define HSM_INTRO_SOLID_BLACK_FADE_OUT 0