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
//    BEZEL
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------
//    GENERATED BEZEL & FRAME
//---------------------------------------------------------------------------------------------------
// #pragma parameter HSM_BZL_TITLE "-----[ GENERATED BEZEL & FRAME ]-------------------------------" 0 0 0 1

#define HAS_BEZEL_PARAMS

#pragma parameter HSM_BZL_INNER_EDGE_THICKNESS		    "          Inner Edge Thickness" 110 0 700 10
float HSM_BZL_INNER_EDGE_THICKNESS = global.HSM_BZL_INNER_EDGE_THICKNESS  * 0.00007;

#pragma parameter HSM_BZL_INNER_EDGE_SHARPNESS		    "          Inner Edge Sharpness - Def 95" 95 0 90 5
float HSM_BZL_INNER_EDGE_SHARPNESS = global.HSM_BZL_INNER_EDGE_SHARPNESS  / 100;

#pragma parameter HSM_BZL_OUTER_POSITION_Y 			    "          Outer Edge Position Y" 0 -500 500 1
float HSM_BZL_OUTER_POSITION_Y = global.HSM_BZL_OUTER_POSITION_Y / 2000;

#pragma parameter HSM_BZL_OUTER_CURVATURE_SCALE 		"          Outer Curvature Scale" 0 0 500 5
float HSM_BZL_OUTER_CURVATURE_SCALE = global.HSM_BZL_OUTER_CURVATURE_SCALE / 100;

#pragma parameter HSM_BZL_OUTER_CORNER_RADIUS_SCALE 	"          Outer Corner Radius Scale - Def 60" 60 0 4000 1
float HSM_BZL_OUTER_CORNER_RADIUS_SCALE = 0.9 * global.HSM_BZL_OUTER_CORNER_RADIUS_SCALE / 100;

#pragma parameter HSM_BZL_NOISE 					"          Noise - Def 30" 30 0 100 1
float HSM_BZL_NOISE = global.HSM_BZL_NOISE / 100;

#pragma parameter HSM_BZL_INNER_EDGE_SHADOW             "          Opacity of Shadow from Bezel on Tube" 70 0 200 5
float HSM_BZL_INNER_EDGE_SHADOW = global.HSM_BZL_INNER_EDGE_SHADOW / 100;


//---------------------------------------------------------------------------------------------------
// Bezel Brightness
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_BEZEL_BRIGHTNESS_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_BEZEL_BRIGHTNESS_TITLE		    "[ BEZEL BRIGHTNESS ]:" 0 0 0.01 0.01

#pragma parameter HSM_BZL_BRIGHTNESS 				    "          Base Brightness - Def 30" 30 0 600 2
float HSM_BZL_BRIGHTNESS = global.HSM_BZL_BRIGHTNESS / 100;

#pragma parameter HSM_BZL_BRIGHTNESS_MULT_TOP 	        "          Top Multiplier - Def 50" 50 0 1000 2
float HSM_BZL_BRIGHTNESS_MULT_TOP = global.HSM_BZL_BRIGHTNESS_MULT_TOP / 100;

#pragma parameter HSM_BZL_BRIGHTNESS_MULT_BOTTOM 	    "          Bottom Multiplier - Def 200" 200 0 1000 2
float HSM_BZL_BRIGHTNESS_MULT_BOTTOM = global.HSM_BZL_BRIGHTNESS_MULT_BOTTOM / 100;

#pragma parameter HSM_BZL_BRIGHTNESS_MULT_SIDES 	    "          Sides Multiplier" 100 0 1000 2
float HSM_BZL_BRIGHTNESS_MULT_SIDES = global.HSM_BZL_BRIGHTNESS_MULT_SIDES / 100;

#pragma parameter HSM_BZL_BRIGHTNESS_MULT_SIDE_LEFT 	"          Left Side Multiplier" 100 0 1000 2
float HSM_BZL_BRIGHTNESS_MULT_SIDE_LEFT = global.HSM_BZL_BRIGHTNESS_MULT_SIDE_LEFT / 100;

#pragma parameter HSM_BZL_BRIGHTNESS_MULT_SIDE_RIGHT 	"          Right Side Multiplier" 100 0 1000 2
float HSM_BZL_BRIGHTNESS_MULT_SIDE_RIGHT = global.HSM_BZL_BRIGHTNESS_MULT_SIDE_RIGHT / 100;

#pragma parameter HSM_BZL_HIGHLIGHT 					"          Main Highlight" 1 0 100 0.2
float HSM_BZL_HIGHLIGHT = global.HSM_BZL_HIGHLIGHT / 100;

#pragma parameter HSM_BZL_INNER_EDGE_HIGHLIGHT 			"          Inner Edge Highlight" 100 0 100 5
float HSM_BZL_INNER_EDGE_HIGHLIGHT = global.HSM_BZL_INNER_EDGE_HIGHLIGHT / 100;


//---------------------------------------------------------------------------------------------------
// Bezel Color
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_BEZEL_COLOR_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_BEZEL_COLOR_TITLE		"[ BEZEL COLOR ]:" 0 0 0.01 0.01

#pragma parameter HSM_BZL_COLOR_HUE					"          Hue" 0 0 360 1
float HSM_BZL_COLOR_HUE = global.HSM_BZL_COLOR_HUE / 360;

#pragma parameter HSM_BZL_COLOR_SATURATION			"          Saturation" 0 0 100 1
float HSM_BZL_COLOR_SATURATION = global.HSM_BZL_COLOR_SATURATION / 100;

#pragma parameter HSM_BZL_COLOR_VALUE				"          Value/Brightness - Def 10" 10 0 100 0.2
float HSM_BZL_COLOR_VALUE = global.HSM_BZL_COLOR_VALUE / 100;

#pragma parameter HSM_BZL_AMBIENT_LIGHTING_MULTIPLIER         "          Bezel Ambient Lighting Multiplier"  100 0 100 1
float HSM_BZL_AMBIENT_LIGHTING_MULTIPLIER = global.HSM_BZL_AMBIENT_LIGHTING_MULTIPLIER / 100;

#pragma parameter HSM_BZL_AMBIENT2_LIGHTING_MULTIPLIER         "          Ambient 2nd Image Lighting Multiplier"  0 0 100 1
float HSM_BZL_AMBIENT2_LIGHTING_MULTIPLIER = global.HSM_BZL_AMBIENT2_LIGHTING_MULTIPLIER / 100;


//---------------------------------------------------------------------------------------------------
// Frame Color
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_FRM_COLOR_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_FRM_COLOR_TITLE		"[ FRAME COLOR ]:" 0 0 0.01 0.01

#pragma parameter HSM_FRM_USE_INDEPENDENT_COLOR				"          Use Independent Frame Color" 0 0 1 1
float HSM_FRM_USE_INDEPENDENT_COLOR = global.HSM_FRM_USE_INDEPENDENT_COLOR;

#pragma parameter HSM_FRM_COLOR_HUE					"          Hue" 0 0 360 1
float HSM_FRM_COLOR_HUE = global.HSM_FRM_COLOR_HUE / 360;

#pragma parameter HSM_FRM_COLOR_SATURATION			"          Saturation" 0 0 100 1
float HSM_FRM_COLOR_SATURATION = global.HSM_FRM_COLOR_SATURATION / 100;

#pragma parameter HSM_FRM_COLOR_VALUE				"          Value/Brightness - Def 10" 10 0 100 0.2
float HSM_FRM_COLOR_VALUE = global.HSM_FRM_COLOR_VALUE / 100;


//---------------------------------------------------------------------------------------------------
//    FRAME
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_FRAME_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_FRAME_TITLE			        "[ FRAME GENERAL ]:" 0 0 0.01 0.01

#pragma parameter HSM_FRM_OPACITY                   "          Opacity" 100 0 100 5
float HSM_FRM_OPACITY = global.HSM_FRM_OPACITY / 100;

// #pragma parameter HSM_FRM_BLEND_MODE                    "          Blend Mode - Off | Normal | Add | Mult"  1 0 3 1
// float HSM_FRM_BLEND_MODE = global.HSM_FRM_BLEND_MODE;

float HSM_FRM_BLEND_MODE = 1;

#pragma parameter HSM_FRM_TEXTURE_OPACITY               "          Texture Overlay Opacity (Highlight)" 1.5 0 100 0.1
float HSM_FRM_TEXTURE_OPACITY = global.HSM_FRM_TEXTURE_OPACITY / 100;

#pragma parameter HSM_FRM_TEXTURE_BLEND_MODE            "         Texture Overlay Blend Mode - Off | Normal | Add | Mult"  2 0 3 1
float HSM_FRM_TEXTURE_BLEND_MODE = global.HSM_FRM_TEXTURE_BLEND_MODE;

#pragma parameter HSM_FRM_NOISE 					    "          Noise - Def 30" 30 0 100 1
float HSM_FRM_NOISE = global.HSM_FRM_NOISE / 100;

#pragma parameter HSM_FRM_INNER_EDGE_THICKNESS 		    "          Inner Edge Thickness" 100 0 5000 5
float HSM_FRM_INNER_EDGE_THICKNESS = global.HSM_FRM_INNER_EDGE_THICKNESS * 0.00003;

#pragma parameter HSM_FRM_INNER_EDGE_HIGHLIGHT 		    "          Inner Edge Highlight" 14 0 100 1
float HSM_FRM_INNER_EDGE_HIGHLIGHT = global.HSM_FRM_INNER_EDGE_HIGHLIGHT * 0.001;

#pragma parameter HSM_FRM_THICKNESS 					"          Frame Thickness" 100 0 2000 5
float HSM_FRM_THICKNESS = global.HSM_FRM_THICKNESS * 0.0007;

#pragma parameter HSM_FRM_THICKNESS_SCALE_X 			"          Frame Thickness Scale X" 100 0 1000 2
float HSM_FRM_THICKNESS_SCALE_X = global.HSM_FRM_THICKNESS_SCALE_X / 100;

#pragma parameter HSM_FRM_OUTER_POS_Y 			        "          Frame Outer Pos Y" 0 -100 100 0.05
float HSM_FRM_OUTER_POS_Y = global.HSM_FRM_OUTER_POS_Y / 100;

#pragma parameter HSM_FRM_OUTER_CURVATURE_SCALE 		"          Frame Outer Curvature Scale" 0 0 500 5
float HSM_FRM_OUTER_CURVATURE_SCALE = global.HSM_FRM_OUTER_CURVATURE_SCALE / 100;

// TODO how this is being used is strange
#pragma parameter HSM_FRM_OUTER_CORNER_RADIUS 		    "          Outer Corner Radius" 5 0 70 0.05
float HSM_FRM_OUTER_CORNER_RADIUS = global.HSM_FRM_OUTER_CORNER_RADIUS;

#pragma parameter HSM_FRM_OUTER_EDGE_THICKNESS 		    "          Outer Edge Thickness" 100 0 1000 10
float HSM_FRM_OUTER_EDGE_THICKNESS = global.HSM_FRM_OUTER_EDGE_THICKNESS * 0.00006;

#pragma parameter HSM_FRM_OUTER_EDGE_SHADING 		    "          Outer Edge Shading" 50 0 100 5
float HSM_FRM_OUTER_EDGE_SHADING = global.HSM_FRM_OUTER_EDGE_SHADING / 100;

#pragma parameter HSM_FRM_SHADOW_OPACITY	            "          Shadow Opacity" 100 0 300 1
float HSM_FRM_SHADOW_OPACITY = global.HSM_FRM_SHADOW_OPACITY / 100;

#pragma parameter HSM_FRM_SHADOW_WIDTH                  "          Shadow Width" 100 0 500 1
float HSM_FRM_SHADOW_WIDTH = global.HSM_FRM_SHADOW_WIDTH / 1000;


#pragma parameter HSM_REFLECT_CORNER_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECT_CORNER_TITLE			            "[ CORNER CREASE - ALSO CONTROLS REFLECTION]:" 0 0 0.01 0.01

#pragma parameter HSM_REFLECT_CORNER_FADE 						"          Corner Fade" 10 1 100 0.5
float HSM_REFLECT_CORNER_FADE = global.HSM_REFLECT_CORNER_FADE / 100;

#pragma parameter HSM_REFLECT_CORNER_FADE_DISTANCE 				"          Corner Fade Distance" 100 1 100 1
float HSM_REFLECT_CORNER_FADE_DISTANCE = global.HSM_REFLECT_CORNER_FADE_DISTANCE / 100;

#pragma parameter HSM_REFLECT_CORNER_INNER_SPREAD 	            "          Corner Inner Spread" 500 0 1000 10
float HSM_REFLECT_CORNER_INNER_SPREAD = global.HSM_REFLECT_CORNER_INNER_SPREAD / 100;

#pragma parameter HSM_REFLECT_CORNER_OUTER_SPREAD 	            "          Corner Outer Spread" 160 0 1000 10
float HSM_REFLECT_CORNER_OUTER_SPREAD = global.HSM_REFLECT_CORNER_OUTER_SPREAD / 100;

#pragma parameter HSM_REFLECT_CORNER_ROTATION_OFFSET_TOP 	    "          Corner Rotation Offset Top" 0 -90 90 0.5
float HSM_REFLECT_CORNER_ROTATION_OFFSET_TOP = global.HSM_REFLECT_CORNER_ROTATION_OFFSET_TOP;

#pragma parameter HSM_REFLECT_CORNER_ROTATION_OFFSET_BOTTOM 	"          Corner Rotation Offset Bottom" 0 -90 90 0.5
float HSM_REFLECT_CORNER_ROTATION_OFFSET_BOTTOM = global.HSM_REFLECT_CORNER_ROTATION_OFFSET_BOTTOM;

#pragma parameter HSM_REFLECT_CORNER_SPREAD_FALLOFF             "          Corner Spread Falloff"  100 1 200 1
float HSM_REFLECT_CORNER_SPREAD_FALLOFF = global.HSM_REFLECT_CORNER_SPREAD_FALLOFF;
