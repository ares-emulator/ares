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

#pragma parameter HSM_CRT_SCREEN_BLEND_MODE                         "          CRT POTATO Blend Mode - OFF | NORMAL | ADD (Backdrop)" 1 0 2 1
float HSM_CRT_SCREEN_BLEND_MODE = global.HSM_CRT_SCREEN_BLEND_MODE;

//---------------------------------------------------------------------------------------------------
//    BASIC BACKGROUND IMAGE LAYER
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_BASIC_BG_IMG_TITLE	                    "[ POTATO BACKGROUND IMAGE LAYER ]:" 0 0 0.01 0.01

#pragma parameter HSM_POTATO_SHOW_BG_OVER_SCREEN 	"          Show BG: 0:UNDER SCREEN | 1:OVER SCREEN" 1 0 1 1
float HSM_POTATO_SHOW_BG_OVER_SCREEN = global.HSM_POTATO_SHOW_BG_OVER_SCREEN;

#pragma parameter HSM_BG_BLEND_MODE               "          Blend Mode - OFF | NORMAL | ADD | MULTIPLY"  1 -1 3 1
float HSM_BG_BLEND_MODE = global.HSM_BG_BLEND_MODE;

#pragma parameter HSM_POTATO_COLORIZE_CRT_WITH_BG                  "          Colorize CRT with BG" 0 0 100 1
float HSM_POTATO_COLORIZE_CRT_WITH_BG = global.HSM_POTATO_COLORIZE_CRT_WITH_BG / 100;

#pragma parameter HSM_POTATO_COLORIZE_BRIGHTNESS                  "          Colorize Brightness" 100 0 1000 10
float HSM_POTATO_COLORIZE_BRIGHTNESS = global.HSM_POTATO_COLORIZE_BRIGHTNESS / 100;

#pragma parameter HSM_BG_OPACITY                  "          Opacity" 100 0 100 0.1
float HSM_BG_OPACITY = global.HSM_BG_OPACITY / 100;

#pragma parameter HSM_BG_BRIGHTNESS                "          Brightness" 100 0 200 1
float HSM_BG_BRIGHTNESS = global.HSM_BG_BRIGHTNESS / 100;
