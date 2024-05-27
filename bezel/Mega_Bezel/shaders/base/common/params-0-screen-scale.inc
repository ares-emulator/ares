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

// Screen Scale Required Parameters
#pragma parameter HSM_MEGA_BEZEL_TITLE								"[ --- HSM MEGA BEZEL V1.17.2_2024-05-18 --- ]" 0 0 0.001 0.001

#pragma parameter HSM_RESOLUTION_DEBUG_ON							"          Show Resolution Info" 0 0 1 1
float HSM_RESOLUTION_DEBUG_ON = global.HSM_RESOLUTION_DEBUG_ON;


//---------------------------------------------------------------------------------------------------
//    CRT BRIGHTNESS & GAMMA
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_CRT_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_CRT_TITLE                             "[ CRT BRIGHTNESS & GAMMA ]:" 0 0 0.001 0.001

#pragma parameter GAMMA_INPUT   							"          Gamma In (Game Embedded Gamma) Def 2.4" 2.4 0.1 5 0.05
#define GAMMA_INPUT global.GAMMA_INPUT

#pragma parameter gamma_out   								"          Gamma Out (Electron Gun Gamma) Def 2.4" 2.4 0.1 5 0.05
#define HSM_GAMMA_OUT_CRT global.gamma_out

#pragma parameter post_br                                   "          Post CRT Brightness" 1 0.01 5 0.01
#define HSM_POST_CRT_BRIGHTNESS global.post_br

#pragma parameter post_br_affect_black_level                "          Post CRT Brightness Affects Grade Black Level" 100 0 100 10
#define HSM_POST_CRT_BRIGHTNESS_AFFECT_BLACK_LEVEL global.post_br_affect_black_level / 100


//---------------------------------------------------------------------------------------------------
//    Global Graphics Brightness
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_GLOBAL_GRAPHICS_BRIGHTNESS_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_GLOBAL_GRAPHICS_BRIGHTNESS_TITLE              "[ GRAPHICS GLOBAL BRIGHTNESS ]:" 0 0 0.001 0.001

#pragma parameter HSM_GLOBAL_GRAPHICS_BRIGHTNESS                    "          Graphics Brightness"  100 0 500 1
float HSM_GLOBAL_GRAPHICS_BRIGHTNESS = global.HSM_GLOBAL_GRAPHICS_BRIGHTNESS / 100;

#pragma parameter HSM_STATIC_LAYERS_GAMMA		                    "          Graphics Gamma Adjust" 1 0.01 5 0.01
float HSM_STATIC_LAYERS_GAMMA = global.HSM_STATIC_LAYERS_GAMMA;


//---------------------------------------------------------------------------------------------------
//    AMBIENT LIGHTING AKA NIGHTLIGHTING
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_AMBIENT_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_AMBIENT_TITLE                       "[ AMBIENT LIGHTING ]:" 0 0 0.001 0.001

#pragma parameter HSM_AMBIENT_LIGHTING_OPACITY                      "          Ambient Global Opacity Multiplier"  0 0 100 1
float HSM_AMBIENT_LIGHTING_OPACITY = global.HSM_AMBIENT_LIGHTING_OPACITY / 100;

#pragma parameter HSM_AMBIENT1_OPACITY                     "          Ambient 1st Image Opacity"  100 0 100 1
float HSM_AMBIENT1_OPACITY = global.HSM_AMBIENT1_OPACITY / 100 * global.HSM_AMBIENT_LIGHTING_OPACITY / 100;

#pragma parameter HSM_AMBIENT2_OPACITY                     "          Ambient 2nd Image Opacity (Used for BG by Default)"  100 0 100 1
float HSM_AMBIENT2_OPACITY = global.HSM_AMBIENT2_OPACITY / 100 * global.HSM_AMBIENT_LIGHTING_OPACITY / 100;

#pragma parameter HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE     "          Which Images to Use: BOTH | 1 ONLY | 2 ONLY | SWAP"  0 0 3 1
float HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE = global.HSM_AMBIENT_LIGHTING_SWAP_IMAGE_MODE;


//---------------------------------------------------------------------------------------------------
//    Sinden Lightgun Border
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SINDEN_EMPTY_LINE					" " 0 0 0.001 0.001
#pragma parameter HSM_SINDEN_TITLE                      "[ SINDEN LIGHTGUN BORDER ]:" 0 0 0.001 0.001

#pragma parameter HSM_SINDEN_BORDER_ON					"         Show Border" 0 0 1 1
float HSM_SINDEN_BORDER_ON = global.HSM_SINDEN_BORDER_ON;

#pragma parameter HSM_SINDEN_BORDER_BRIGHTNESS				"         Brightness" 60 0 200 5
float HSM_SINDEN_BORDER_BRIGHTNESS = global.HSM_SINDEN_BORDER_BRIGHTNESS / 100;

#pragma parameter HSM_SINDEN_BORDER_THICKNESS		    "         Thickness" 2 0 30 0.5
float HSM_SINDEN_BORDER_THICKNESS = global.HSM_SINDEN_BORDER_THICKNESS / 100;

#pragma parameter HSM_SINDEN_BORDER_EMPTY_TUBE_COMPENSATION	"         Compensate Empty Tube" 0 0 1 1
float HSM_SINDEN_BORDER_EMPTY_TUBE_COMPENSATION = global.HSM_SINDEN_BORDER_EMPTY_TUBE_COMPENSATION;


//---------------------------------------------------------------------------------------------------
//    Zoom & Pan
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_ZOOM_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_VIEWPORT_ZOOM_TITLE							"[ VIEWPORT ZOOM ]:" 0 0 0.001 0.001

#pragma parameter HSM_VIEWPORT_ZOOM									"          Viewport Zoom"  100 10 500 0.2
float HSM_VIEWPORT_ZOOM = global.HSM_VIEWPORT_ZOOM / 100;

#pragma parameter HSM_VIEWPORT_ZOOM_MASK							"          Zoom CRT Mask"  0 0 1 1
float HSM_VIEWPORT_ZOOM_MASK = global.HSM_VIEWPORT_ZOOM_MASK;

#pragma parameter HSM_VIEWPORT_POSITION_X							"          Viewport Position X"  0 -500 500 1
float HSM_VIEWPORT_POSITION_X = -1 * global.HSM_VIEWPORT_POSITION_X / 1000;

#pragma parameter HSM_VIEWPORT_POSITION_Y							"          Viewport Position Y"  0 -500 500 1
float HSM_VIEWPORT_POSITION_Y = -1 * global.HSM_VIEWPORT_POSITION_Y / 1000;


//---------------------------------------------------------------------------------------------------
//    FLIP & ROTATE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_FLIP_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_FLIP_ROTATE_TITLE								"[ FLIP & ROTATE ]:" 0 0 0.001 0.001

#pragma parameter HSM_FLIP_VIEWPORT_VERTICAL						"          Flip Viewport Vertical"  0 0 1 1
float HSM_FLIP_VIEWPORT_VERTICAL = -(global.HSM_FLIP_VIEWPORT_VERTICAL * 2 - 1);

#pragma parameter HSM_FLIP_VIEWPORT_HORIZONTAL						"          Flip Viewport Horizontal"  0 0 1 1
float HSM_FLIP_VIEWPORT_HORIZONTAL = -(global.HSM_FLIP_VIEWPORT_HORIZONTAL * 2 - 1);

#pragma parameter HSM_FLIP_CORE_VERTICAL							"          Flip Core Image Vertical"  0 0 1 1
// float HSM_FLIP_CORE_VERTICAL = global.HSM_FLIP_CORE_VERTICAL;
float HSM_FLIP_CORE_VERTICAL = -(global.HSM_FLIP_CORE_VERTICAL * 2 - 1);

#pragma parameter HSM_FLIP_CORE_HORIZONTAL							"          Flip Core Image Horizontal"  0 0 1 1
// float HSM_FLIP_CORE_HORIZONTAL = global.HSM_FLIP_CORE_HORIZONTAL;
float HSM_FLIP_CORE_HORIZONTAL = -(global.HSM_FLIP_CORE_HORIZONTAL * 2 - 1);

#pragma parameter HSM_ROTATE_CORE_IMAGE								"          Rotate CRT Tube --  -1:270 | 0 | 1:90"  0 -1 1 1
float HSM_ROTATE_CORE_IMAGE = global.HSM_ROTATE_CORE_IMAGE;


//---------------------------------------------------------------------------------------------------
//    ASPECT RATIO
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_ASPECT_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_ASPECT_RATIO_TITLE							"[ ASPECT RATIO ]:" 0 0 0.001 0.001

#pragma parameter HSM_ASPECT_RATIO_ORIENTATION 						"          Orientation - Auto | Horizontal | Vertical"  0 0 2 1
float HSM_ASPECT_RATIO_ORIENTATION = global.HSM_ASPECT_RATIO_ORIENTATION;

#pragma parameter HSM_ASPECT_RATIO_MODE 							"         Type - Auto | Explicit | 4:3 | 3:2 | 16:9 | PAR | Full"  0 0 6 1
float HSM_ASPECT_RATIO_MODE = global.HSM_ASPECT_RATIO_MODE;

#pragma parameter HSM_ASPECT_RATIO_EXPLICIT							"          Explicit Aspect Ratio (Also Auto Ratio Fallback)"  1.3333 0 8 0.002
float HSM_ASPECT_RATIO_EXPLICIT = global.HSM_ASPECT_RATIO_EXPLICIT;


//---------------------------------------------------------------------------------------------------
//    CRT SCREEN SCALING GENERAL
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SCALING_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SCALING_TITLE									"[ CRT SCREEN SCALING GENERAL ]:" 0 0 0.001 0.001

#pragma parameter HSM_INT_SCALE_MODE 								"          Integer Scale Mode - OFF | ShortAxis | BothAxes" 0 0 2 1
#define HSM_INT_SCALE_MODE global.HSM_INT_SCALE_MODE

#pragma parameter HSM_VERTICAL_PRESET 								"       Preset is for Monitor Portrait Mode (Smaller CRT Screen)" 0 0 1 1
float HSM_VERTICAL_PRESET = global.HSM_VERTICAL_PRESET;


//---------------------------------------------------------------------------------------------------
//    Integer Scale
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_INTEGER_SCALE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTEGER_SCALE_TITLE							"[ INTEGER SCALE ]:" 0 0 0.001 0.001

#pragma parameter HSM_INT_SCALE_MAX_HEIGHT 					        "          Base Integer Scale Max Height %" 89 60 140 0.5
float HSM_INT_SCALE_MAX_HEIGHT = global.HSM_INT_SCALE_MAX_HEIGHT / 100;

#pragma parameter HSM_INT_SCALE_MULTIPLE_OFFSET 					"          Integer Scale Multiple Offset" 0 -20 20 1
#define HSM_INT_SCALE_MULTIPLE_OFFSET global.HSM_INT_SCALE_MULTIPLE_OFFSET

#pragma parameter HSM_INT_SCALE_MULTIPLE_OFFSET_LONG 				"          Integer Scale Multiple Offset on Long Axis" 0 -20 20 1
#define HSM_INT_SCALE_MULTIPLE_OFFSET_LONG global.HSM_INT_SCALE_MULTIPLE_OFFSET_LONG


//---------------------------------------------------------------------------------------------------
//    NON-INTEGER SCALE
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_NON_INTEGER_SCALE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_NON_INTEGER_SCALE_PERCENT_TITLE							"[ NON-INTEGER SCALE PERCENT ]:" 0 0 0.001 0.001

// Non integer scale is set to 82.97 so that cores which output 224px will have integer scale by default
// This is 8x integer scale at 3840x2160 and 6x integer scale at 1920x1080
#pragma parameter HSM_NON_INTEGER_SCALE 							"          Non-Integer Scale %" 82.97 1 200 0.2
float HSM_NON_INTEGER_SCALE = global.HSM_NON_INTEGER_SCALE / 100;


//---------------------------------------------------------------------------------------------------
//    PHYSICAL SIZE
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_PHYSICAL_SIZE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_PHYSICAL_SIZE_TITLE							"[ NON-INTEGER - PHYSICAL SIZES ]:" 0 0 0.001 0.001

#pragma parameter HSM_USE_PHYSICAL_SIZE_FOR_NON_INTEGER 			"          Use Physical Monitor and Tube Sizes for Non-Integer" 0 0 1 1
float HSM_USE_PHYSICAL_SIZE_FOR_NON_INTEGER = global.HSM_USE_PHYSICAL_SIZE_FOR_NON_INTEGER;

#pragma parameter HSM_PHYSICAL_MONITOR_ASPECT_RATIO					"          Your Monitor's Aspect Ratio" 1.77 0.25 5 0.01
float HSM_PHYSICAL_MONITOR_ASPECT_RATIO = global.HSM_PHYSICAL_MONITOR_ASPECT_RATIO;

#pragma parameter HSM_PHYSICAL_MONITOR_DIAGONAL_SIZE		        "          Your Monitor's Size (Diagonal)" 27 1 100 0.1
float HSM_PHYSICAL_MONITOR_DIAGONAL_SIZE = global.HSM_PHYSICAL_MONITOR_DIAGONAL_SIZE;

#pragma parameter HSM_PHYSICAL_SIM_TUBE_DIAGONAL_SIZE				"          Simulated Tube Size (Diagonal)" 18.1 1 100 0.1
float HSM_PHYSICAL_SIM_TUBE_DIAGONAL_SIZE = global.HSM_PHYSICAL_SIM_TUBE_DIAGONAL_SIZE;


//---------------------------------------------------------------------------------------------------
//    AUTOMATIC SCALE & POSITION
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_AUTOMATIC_SCREEN_SCALE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_AUTOMATIC_SCREEN_SCALE_TITLE					"[ NON-INTEGER - AUTOMATIC SCREEN SCALE & PLACEMENT ]:" 0 0 0.001 0.001

#pragma parameter HSM_USE_IMAGE_FOR_PLACEMENT 						"          Use Image For Automatic Placement (Scale and Y Pos)" 0 0 1 1
float HSM_USE_IMAGE_FOR_PLACEMENT = global.HSM_USE_IMAGE_FOR_PLACEMENT;

#pragma parameter HSM_PLACEMENT_IMAGE_USE_HORIZONTAL 				"          Auto Place Horizontal (X Pos)" 0 0 1 1
float HSM_PLACEMENT_IMAGE_USE_HORIZONTAL = global.HSM_PLACEMENT_IMAGE_USE_HORIZONTAL;

#pragma parameter HSM_PLACEMENT_IMAGE_MODE 						    "          Placement Image Mode: TRANSPARENCY : WHITE ON BLACK" 0 0 1 1
float HSM_PLACEMENT_IMAGE_MODE = global.HSM_PLACEMENT_IMAGE_MODE;


//---------------------------------------------------------------------------------------------------
//    NON-INTEGER SCALE OFFSET
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_NONINTEGER_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_NONINTEGER_SCALE_OFFSET_TITLE					"[ NON-INTEGER SCALE OFFSET ]:" 0 0 0.001 0.001

#pragma parameter HSM_NON_INTEGER_SCALE_OFFSET 						"          Non-Integer Scale Offset" 100 5 400 0.1
float HSM_NON_INTEGER_SCALE_OFFSET = global.HSM_NON_INTEGER_SCALE_OFFSET / 100;


//---------------------------------------------------------------------------------------------------
//    SNAP TO INTEGER SCALE
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_SNAP_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SNAP_NONINTEGER_TITLE							"[ SNAP NON-INTEGER TO CLOSEST INTEGER SCALE ]:" 0 0 0.001 0.001

#pragma parameter HSM_USE_SNAP_TO_CLOSEST_INT_SCALE 				"          Snap to Closest Integer Scale: OFF | ON" 0 0 1 1
float HSM_USE_SNAP_TO_CLOSEST_INT_SCALE = global.HSM_USE_SNAP_TO_CLOSEST_INT_SCALE;

#pragma parameter HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE 			"          Snap To Closest Integer Scale Tolerance" 20 0 300 1
float HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE = HSM_USE_SNAP_TO_CLOSEST_INT_SCALE > 0.5 ? global.HSM_SNAP_TO_CLOSEST_INT_SCALE_TOLERANCE / 100 : 0;



//---------------------------------------------------------------------------------------------------
//    POSITION OFFSET
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SCREEN_POS_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SCREEN_POSITION_TITLE							"[ POSITION OFFSET ]:" 0 0 0.001 0.001

#pragma parameter HSM_SCREEN_POSITION_X 							"          Position X" 0 -1000 1000 1
float HSM_SCREEN_POSITION_X = global.HSM_SCREEN_POSITION_X / 1000;

#pragma parameter HSM_SCREEN_POSITION_Y 							"          Position Y" 0 -1000 1000 1
float HSM_SCREEN_POSITION_Y = -1 * global.HSM_SCREEN_POSITION_Y / 1000;


//---------------------------------------------------------------------------------------------------
//    CROPPING
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_CROPPING_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_CROPPING_TITLE								"[ CROPPING CORE IMAGE ]:" 0 0 0.001 0.001
#pragma parameter HSM_CROP_FYI 									    "    FYI !!! Negative Crop is currently Disabled" 0 0 0.001 0.001

#pragma parameter HSM_CROP_MODE 									"          Crop Mode  -  OFF | CROP BLACK ONLY | CROP ANY" 2 0 2 1
float HSM_CROP_MODE = global.HSM_CROP_MODE;

#pragma parameter HSM_CROP_PERCENT_ZOOM								"          Crop Zoom %" 0 -50 100 0.1
float HSM_CROP_PERCENT_ZOOM = global.HSM_CROP_PERCENT_ZOOM / 100;

#pragma parameter HSM_CROP_PERCENT_TOP 								"          Crop Top %" 0 -50 100 0.1
float HSM_CROP_PERCENT_TOP = global.HSM_CROP_PERCENT_TOP / 100;

#pragma parameter HSM_CROP_PERCENT_BOTTOM 							"          Crop Bottom %" 0 -50 100 0.1
float HSM_CROP_PERCENT_BOTTOM = global.HSM_CROP_PERCENT_BOTTOM / 100;

#pragma parameter HSM_CROP_PERCENT_LEFT 							"          Crop Left %" 0 -50 100 0.1
float HSM_CROP_PERCENT_LEFT = global.HSM_CROP_PERCENT_LEFT / 100;

#pragma parameter HSM_CROP_PERCENT_RIGHT 							"          Crop Right %" 0 -50 100 0.1
float HSM_CROP_PERCENT_RIGHT = global.HSM_CROP_PERCENT_RIGHT / 100;

#pragma parameter HSM_CROP_BLACK_THRESHOLD 							"          Black Threshold for 'CROP BLACK ONLY'" 0 0 100 1
float HSM_CROP_BLACK_THRESHOLD = global.HSM_CROP_BLACK_THRESHOLD / 100;


//---------------------------------------------------------------------------------------------------
//    DREZ Filter
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_GSHARP_DREZ_EMPTY_LINE		" " 0 0 0.001 0.001
#pragma parameter GSHARP_DREZ_TITLE 	    "[ --- DREZ DOWNSAMPLE FILTER - GUEST.R G-SHARP RESAMPLER --- ]:"  0 0 0.01 0.01

#pragma parameter HSM_DREZ_GSHARP_ON 	    "          G-SHARP ON" 	  1 0 1 1
#pragma parameter HSM_DREZ_THRESHOLD_RATIO 	"         Downsample Threshold (Original Res Vs Downsampled Res)" 	  1.5 1 4 0.1
// Default was 1.2, now set to 2.3 to match smoothing in Hyllian b-spline
#pragma parameter HSM_DREZ_HSHARP0 		    "          Filter Range"  2.3 1.0 6.0 0.1
#pragma parameter HSM_DREZ_SIGMA_HV 		"          Gaussian Blur Sigma" 0.75 0.1 7.0 0.05
#pragma parameter HSM_DREZ_SHAR 			"          Sharpness Definition" 0.5 0.0 2.0 0.05
#pragma parameter HSM_DREZ_HAR 			    "          Anti-Ringing" 0.5 0.0 1.0 0.10


//---------------------------------------------------------------------------------------------------
//    CORE RES SAMPLING
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SAMPLING_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SAMPLING_TITLE								"[ CORE RES SAMPLING ]:" 0 0 0.001 0.001

#pragma parameter HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR			"          Scanline Dir Multiplier"  100 25 1600 25
float HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR = global.HSM_CORE_RES_SAMPLING_MULT_SCANLINE_DIR / 100;

#pragma parameter HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR 					"          Scanline Dir Downsample Blur" 0 0 200 1
float HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR = global.HSM_DOWNSAMPLE_BLUR_SCANLINE_DIR / 100;

#pragma parameter HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR			"          Opposite Dir Multiplier"  100 25 1600 25
float HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR = global.HSM_CORE_RES_SAMPLING_MULT_OPPOSITE_DIR / 100;

#pragma parameter HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR 					"          Opposite Dir Downsample Blur" 0 0 200 1
float HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR = global.HSM_DOWNSAMPLE_BLUR_OPPOSITE_DIR / 100;

// #pragma parameter HSM_CORE_RES_SAMPLING_SHIFT_OPPOSITE_DIR 			"          Shift Sampling Relative to Scanlines" 0 -100 100 5
// float HSM_CORE_RES_SAMPLING_SHIFT_OPPOSITE_DIR = global.HSM_CORE_RES_SAMPLING_SHIFT_OPPOSITE_DIR / 100;

//---------------------------------------------------------------------------------------------------
//    FAST SHARPEN - GUEST.R
//---------------------------------------------------------------------------------------------------
#pragma parameter SHARPEN_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter SHARPEN_TITLE  	"[ FAST SHARPEN - GUEST.R ]:"         			0.00 0.0 2.00 0.05
#pragma parameter SHARPEN_ON  		"          FSharpen - ON"       		1 0 1 1
#pragma parameter CSHARPEN          "          FSharpen - Sharpen strength"         0.00 0.0 5.00 0.10
#pragma parameter CCONTR            "          FSharpen - Sharpen Contrast/Ringing" 0.05 0.0 0.25 0.01
#pragma parameter CDETAILS          "          FSharpen - Details sharpened"        1.00 0.0 1.00 0.05
#pragma parameter DEBLUR            "          FSharpen - Deblur Strength (For NTSC Only)"          1.00 1.0 7.00 0.25


//---------------------------------------------------------------------------------------------------
//    INTERLACING from Guest.r's Guest Advanced Shader
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_INTERLACE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_INTERLACE_TITLE								"[ SCANLINES & INTERLACING( By Guest.r) ]:" 0 0 0.001 0.001
#pragma parameter HSM_SCANLINE_DIRECTION 							"          Scanline Direction - Auto | Horiz | Vert"  0 0 2 1
float HSM_SCANLINE_DIRECTION = global.HSM_SCANLINE_DIRECTION;

#pragma parameter hiscan "          High Resolution Scanlines (ScaleFx or XBR Only)" 0.0 0.0 1.0 1.0
#define hiscan        global.hiscan

// Uses naming from Guest Advanced
#pragma parameter no_scanlines                                      "          No-Scanline Mode (Guest Advanced Only)" 0.0 0.0 1.5 0.05

#pragma parameter HSM_INTERLACE_SUPPORT1							"[  Interlace Mode: What Happens if Vertical Res > Trigger Res ]" 0 0 0.001 0.001
#pragma parameter HSM_INTERLACE_SUPPORT2 							"[ -1:NO SCANLN 0:NO LACE 1-3:NORM LACE 4-5:INTERPOLATE FIELDS ]" 0 0 0.0001 1

#pragma parameter HSM_INTERLACE_MODE 								"          Interlace Mode:" 4 -1.0 5.0 1.0
#define HSM_INTERLACE_MODE global.HSM_INTERLACE_MODE

#pragma parameter HSM_INTERLACE_TRIGGER_RES 						"          Trigger Res for Interlace, Fake Scanlines & VGA" 375 100 2000 25
#define HSM_INTERLACE_TRIGGER_RES global.HSM_INTERLACE_TRIGGER_RES

#pragma parameter HSM_INTERLACE_EFFECT_SMOOTHNESS_INTERS 			"          Interlace Effect Smoothness" 0 0 100 10
#define HSM_INTERLACE_EFFECT_SMOOTHNESS_INTERS global.HSM_INTERLACE_EFFECT_SMOOTHNESS_INTERS / 200

#pragma parameter HSM_INTERLACE_SCANLINE_EFFECT 					"          Interlace Scanline Effect ('Laced brightness)" 0.20 0.0 1.0 0.05
#define HSM_INTERLACE_SCANLINE_EFFECT global.HSM_INTERLACE_SCANLINE_EFFECT / 100

#pragma parameter iscans 				                            "          Interlace (Scanline) Saturation" 0.25 0 1 0.05
#define iscans global.iscans

#pragma parameter vga_mode                                          "          VGA Single/Double Scan mode" 0.0 0.0 1.0 1.0
#define vga_mode  global.vga_mode     // interlace saturation


//---------------------------------------------------------------------------------------------------
//    FAKE SCANLINES
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_FAKE_SCANLINES_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_FAKE_SCANLINE_TITLE							"[ FAKE SCANLINES ]" 0 0 0.001 0.001

#pragma parameter HSM_FAKE_SCANLINE_MODE 							"          Show Fake Scanlines - OFF | ON | USE TRIGGER RES" 2 0 2 1
#define HSM_FAKE_SCANLINE_MODE global.HSM_FAKE_SCANLINE_MODE

#pragma parameter HSM_FAKE_SCANLINE_OPACITY 						"          Opacity - Def 70" 0 0 100 5
#define HSM_FAKE_SCANLINE_OPACITY global.HSM_FAKE_SCANLINE_OPACITY / 100

#pragma parameter HSM_FAKE_SCANLINE_RES_MODE						"          Scan Resolution Mode: AUTO (CORE RES) : EXPLICIT" 1 0 1 1
#define HSM_FAKE_SCANLINE_RES_MODE global.HSM_FAKE_SCANLINE_RES_MODE

#pragma parameter HSM_FAKE_SCANLINE_RES 							"          Explicit Scan Resolution" 240 0 480 8
#define HSM_FAKE_SCANLINE_RES global.HSM_FAKE_SCANLINE_RES

#pragma parameter HSM_FAKE_SCANLINE_INT_SCALE 						"          Int Scale Scanlines" 1 0 1 1
#define HSM_FAKE_SCANLINE_INT_SCALE global.HSM_FAKE_SCANLINE_INT_SCALE

#pragma parameter HSM_FAKE_SCANLINE_ROLL 							"          Rolling Scanline" 0 -100 100 1
#define HSM_FAKE_SCANLINE_ROLL global.HSM_FAKE_SCANLINE_ROLL / 100

#pragma parameter HSM_FAKE_SCANLINE_CURVATURE 						"          Scanline Curvature" 0 0 100 1
#define HSM_FAKE_SCANLINE_CURVATURE global.HSM_FAKE_SCANLINE_CURVATURE / 100

// #pragma parameter HSM_FAKE_SCANLINE_BRIGHTNESS_CUTOFF 	"[ SCREEN FX ]  Fake 240p Scanline Brightness Cutoff - Def 3" 3 0 10 0.05
#ifndef HSM_FAKE_SCANLINE_BRIGHTNESS_CUTOFF
	#define HSM_FAKE_SCANLINE_BRIGHTNESS_CUTOFF 3
#endif

//---------------------------------------------------------------------------------------------------
//    CURVATURE
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_OVERSCAN_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_OVERSCAN_TITLE "[ OVERSCAN & RASTER BLOOM ]: " 0.0 0.0 1.0 1.0

#pragma parameter HSM_OVERSCAN_AMOUNT "          Overscan %" 0 -50 100 0.5
#define HSM_OVERSCAN_AMOUNT        (global.HSM_OVERSCAN_AMOUNT / 100)

#pragma parameter HSM_OVERSCAN_X "          Overscan X %" 0 -50 100 0.5
#define HSM_OVERSCAN_X        (HSM_OVERSCAN_AMOUNT + global.HSM_OVERSCAN_X / 100)

#pragma parameter HSM_OVERSCAN_Y "          Overscan Y %" 0 -50 100 0.5
#define HSM_OVERSCAN_Y        (HSM_OVERSCAN_AMOUNT + global.HSM_OVERSCAN_Y / 100)

#pragma parameter HSM_OVERSCAN_RASTER_BLOOM_ON "          Raster Bloom ON -- (Scaling based on average luma)" 0 0 1 1
#define HSM_OVERSCAN_RASTER_BLOOM_ON        global.HSM_OVERSCAN_RASTER_BLOOM_ON

#pragma parameter HSM_OVERSCAN_RASTER_BLOOM_MODE "          Raster Bloom Overscan Mode" 2.0 0.0 2.0 1.0
#define HSM_OVERSCAN_RASTER_BLOOM_MODE           global.HSM_OVERSCAN_RASTER_BLOOM_MODE

#pragma parameter HSM_OVERSCAN_RASTER_BLOOM_AMOUNT "          Raster Bloom %" 5.0 0.0 20.0 1.0
#define HSM_OVERSCAN_RASTER_BLOOM_AMOUNT        global.HSM_OVERSCAN_RASTER_BLOOM_AMOUNT / 100     // Bloom overscan percentage

#pragma parameter HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE "          Raster Bloom Overscan Neutral Area" 0.3 0.0 1.0 0.05
#define HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE           global.HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE

#pragma parameter HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE_CENTER "          Raster Bloom Overscan Neutral Area Center" 0.5 0.0 1.0 0.05
#define HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE_CENTER           global.HSM_OVERSCAN_RASTER_BLOOM_NEUTRAL_RANGE_CENTER


//---------------------------------------------------------------------------------------------------
//    CURVATURE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_CURVATURE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_CURVATURE_TITLE								"[ CURVATURE ]:" 0 0 0.001 0.001

// #pragma parameter HSM_USE_GEOM 									"          Use Geom Curvature" 0 -1 3 1
// float HSM_USE_GEOM = global.HSM_USE_GEOM;
float HSM_USE_GEOM = 0;

#pragma parameter HSM_CURVATURE_MODE 								"       Curvature Mode  OFF | 2D | 2D-CYL | 3D-1 | 3D-2 | 3D CYL" 1 0 5 1
float HSM_CURVATURE_MODE = global.HSM_CURVATURE_MODE;

#pragma parameter HSM_CURVATURE_2D_SCALE_LONG_AXIS 					"          2D Curvature on Long Axis" 100 5 400 5
#define HSM_CURVATURE_2D_SCALE_LONG_AXIS global.HSM_CURVATURE_2D_SCALE_LONG_AXIS / 100

#pragma parameter HSM_CURVATURE_2D_SCALE_SHORT_AXIS 				"          2D Curvature on Short Axis" 100 5 400 5
#define HSM_CURVATURE_2D_SCALE_SHORT_AXIS global.HSM_CURVATURE_2D_SCALE_SHORT_AXIS / 100

#pragma parameter HSM_CURVATURE_3D_RADIUS 							"          3D Radius" 200 16 102400 1
float HSM_CURVATURE_3D_RADIUS = global.HSM_CURVATURE_3D_RADIUS / 100;

#pragma parameter HSM_CURVATURE_3D_VIEW_DIST 						"          3D View Distance - Def 150" 150 50 102400 5
float HSM_CURVATURE_3D_VIEW_DIST = global.HSM_CURVATURE_3D_VIEW_DIST / 100;

#pragma parameter HSM_CURVATURE_3D_TILT_ANGLE_X 					"          3D Tilt Angle X" 0 -120 120 1
float HSM_CURVATURE_3D_TILT_ANGLE_X = global.HSM_CURVATURE_3D_TILT_ANGLE_X / 100;

#pragma parameter HSM_CURVATURE_3D_TILT_ANGLE_Y 					"          3D Tilt Angle Y" 0 -120 120 1
float HSM_CURVATURE_3D_TILT_ANGLE_Y = global.HSM_CURVATURE_3D_TILT_ANGLE_Y / 100;

#pragma parameter HSM_CRT_CURVATURE_SCALE 				            "       CRT Curvature Scale Multiplier: 0 = Less Moire Artifacts" 100 0 100 1
float HSM_CRT_CURVATURE_SCALE = global.HSM_CRT_CURVATURE_SCALE / 100;


//---------------------------------------------------------------------------------------------------
//    SIGNAL NOISE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SIGNAL_NOISE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SIGNAL_NOISE_TITLE							"[ Signal-Noise ]:" 0 0 0.001 0.001

#pragma parameter HSM_SIGNAL_NOISE_ON 								"          Signal Noise ON" 0 0 1 1
#define HSM_SIGNAL_NOISE_ON global.HSM_SIGNAL_NOISE_ON

#pragma parameter HSM_SIGNAL_NOISE_AMOUNT 						    "          Amount" 0.2 -1.0 1.0 0.02
#define HSM_SIGNAL_NOISE_AMOUNT global.HSM_SIGNAL_NOISE_AMOUNT

#pragma parameter HSM_SIGNAL_NOISE_BLACK_LEVEL 						"          Contrast"  100 0 300 5
#define HSM_SIGNAL_NOISE_BLACK_LEVEL global.HSM_SIGNAL_NOISE_BLACK_LEVEL / 100

#pragma parameter HSM_SIGNAL_NOISE_SIZE_MODE 						"          Noise Resolution Mode:  SCANLINE | CORE | SMOOTHED" 0 0 2 1
#define HSM_SIGNAL_NOISE_SIZE_MODE global.HSM_SIGNAL_NOISE_SIZE_MODE

#pragma parameter HSM_SIGNAL_NOISE_SIZE_MULT 						"          Size Mult" 0.5 0.5 4 0.5
#define HSM_SIGNAL_NOISE_SIZE_MULT global.HSM_SIGNAL_NOISE_SIZE_MULT

#pragma parameter HSM_SIGNAL_NOISE_TYPE 						    "          Noise Type:  LUMA ADD | COLORED" 0 0 1 1
#define HSM_SIGNAL_NOISE_TYPE global.HSM_SIGNAL_NOISE_TYPE


//---------------------------------------------------------------------------------------------------
//    ANTI-FLICKER
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_ANTI_FLICKER_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_ANTI_FLICKER_TITLE							"[ ANTI-FLICKER ]:" 0 0 0.001 0.001

#pragma parameter HSM_ANTI_FLICKER_ON 								"          Anti-Flicker ON" 0 0 1 1
#define HSM_ANTI_FLICKER_ON global.HSM_ANTI_FLICKER_ON

#pragma parameter HSM_ANTI_FLICKER_THRESHOLD 						"          Luma Difference Threshold" 25 0 100 5
#define HSM_ANTI_FLICKER_THRESHOLD global.HSM_ANTI_FLICKER_THRESHOLD / 100


//---------------------------------------------------------------------------------------------------
//    Global Graphics Caching
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_GLOBAL_GRAPHICS_CACHE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_GLOBAL_GRAPHICS_CACHE_TITLE                   "[ GRAPHICS CACHE ]:" 0 0 0.001 0.001

#pragma parameter HSM_CACHE_GRAPHICS_ON							    "         Cache Graphics: OFF | ON" 1 0 1 1
float HSM_CACHE_GRAPHICS_ON = global.HSM_CACHE_GRAPHICS_ON;

#pragma parameter HSM_CACHE_UPDATE_INDICATOR_MODE					"        Cache Update Indicator: OFF | ON | ONLY IF CACHE IS OFF" 2 0 2 1
float HSM_CACHE_UPDATE_INDICATOR_MODE = global.HSM_CACHE_UPDATE_INDICATOR_MODE;


//---------------------------------------------------------------------------------------------------
//    A/B SPLITSCREEN COMPARE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_CRT_AB_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_CRT_AB_COMPARE_TITLE							"[ A/B SPLITSCREEN COMPARE ]:" 0 0 0.001 0.001

#pragma parameter HSM_AB_COMPARE_SHOW_MODE   				        "          Show:  CRT SHADER | ORIGINAL" 0 0 1 1
float HSM_AB_COMPARE_SHOW_MODE = global.HSM_AB_COMPARE_SHOW_MODE;

#pragma parameter HSM_AB_COMPARE_AREA   					        "          Compare Area:  LEFT | RIGHT | TOP | BOTTOM" 0 0 3 1
float HSM_AB_COMPARE_AREA = global.HSM_AB_COMPARE_AREA;

#pragma parameter HSM_AB_COMPARE_SPLIT_POSITION   		            "          Splitscreen Position" 50 0 100 0.2
float HSM_AB_COMPARE_SPLIT_POSITION = global.HSM_AB_COMPARE_SPLIT_POSITION / 100;

#pragma parameter HSM_AB_COMPARE_FREEZE_CRT_TUBE   				    "          Freeze CRT Tube (Freeze Left, New changes on Right)" 0 0 1 1
float HSM_AB_COMPARE_FREEZE_CRT_TUBE = global.HSM_AB_COMPARE_FREEZE_CRT_TUBE;

#pragma parameter HSM_AB_COMPARE_FREEZE_GRAPHICS   				    "          Freeze Graphics (Freeze Left, New changes on Right)" 0 0 1 1
float HSM_AB_COMPARE_FREEZE_GRAPHICS = global.HSM_AB_COMPARE_FREEZE_GRAPHICS;


//---------------------------------------------------------------------------------------------------
//    SHOW PASS
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_PASS_VIEWER_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_PASS_VIEWER_TITLE       "[ SHOW PASS (For Debugging) ]:" 0 0 0.001 0.001

#pragma parameter HSM_SHOW_PASS_INDEX_INFO1   "     0:END | 1:REFLECTION | 2:TUBE | 3:CRT" 0 0 0.001 0.001
#pragma parameter HSM_SHOW_PASS_INDEX_INFO2   "     4:INTERLACE | 5:COLOR CORRECT & UPSCALE" 0 0 0.001 0.001
#pragma parameter HSM_SHOW_PASS_INDEX_INFO3   "     6:DEDITHER | 7:DREZ | 8:CORE" 0 0 0.001 0.001
#pragma parameter HSM_SHOW_PASS_INDEX_INFO4   "     9:LAYERS TOP | 10:LAYERS BOTTOM" 0 0 0.001 0.001

#pragma parameter HSM_SHOW_PASS_INDEX         "          Show Pass Index" 0 0 10 1
float HSM_SHOW_PASS_INDEX = global.HSM_SHOW_PASS_INDEX;

#pragma parameter HSM_SHOW_PASS_APPLY_SCREEN_COORD  "          Apply Screen Scale & Curvature to Unscaled Passes" 1 0 1 1
float HSM_SHOW_PASS_APPLY_SCREEN_COORD = global.HSM_SHOW_PASS_APPLY_SCREEN_COORD;

#pragma parameter HSM_SHOW_PASS_ALPHA         "          Show Alpha Channel" 0 0 1 1
float HSM_SHOW_PASS_ALPHA = global.HSM_SHOW_PASS_ALPHA;


//---------------------------------------------------------------------------------------------------
//    SCREEN VIGNETTE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SCREEN_VIGNETTE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SCREEN_VIGNETTE_TITLE							"[ SCREEN VIGNETTE ]:" 0 0 0.001 0.001

#pragma parameter HSM_SCREEN_VIGNETTE_ON                			"          Use Vignette" 1 0 1 1
#define HSM_SCREEN_VIGNETTE_ON global.HSM_SCREEN_VIGNETTE_ON

#pragma parameter HSM_SCREEN_VIGNETTE_DUALSCREEN_VIS_MODE           "          Dual Screen Visibility - BOTH | SCREEN 1 | SCREEN 2" 0 0 2 1
float HSM_SCREEN_VIGNETTE_DUALSCREEN_VIS_MODE = global.HSM_SCREEN_VIGNETTE_DUALSCREEN_VIS_MODE;

#pragma parameter HSM_SCREEN_VIGNETTE_STRENGTH                		"          Amount (Strength)" 40 0 98 2
#define HSM_SCREEN_VIGNETTE_STRENGTH (1 - global.HSM_SCREEN_VIGNETTE_STRENGTH / 100) * 50

#pragma parameter HSM_SCREEN_VIGNETTE_POWER                			"          Corner Amount (Power)" 26 0 200 2
#define HSM_SCREEN_VIGNETTE_POWER global.HSM_SCREEN_VIGNETTE_POWER / 100

#pragma parameter HSM_SCREEN_VIGNETTE_IN_REFLECTION                	"          Amount of Vignette in Reflection" 100 0 100 1
#define HSM_SCREEN_VIGNETTE_IN_REFLECTION global.HSM_SCREEN_VIGNETTE_IN_REFLECTION / 100


//---------------------------------------------------------------------------------------------------
//    MONOCHROME
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_MONOCHROME_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_MONOCHROME_TITLE 								"[ MONOCHROME ]:" 0 0 0.001 0.001

#pragma parameter HSM_MONOCHROME_MODE  								"          Monochrome Color: OFF | BW | AMBER | GREEN" 0.0 0.0 3.0 1.0
#define HSM_MONOCHROME_MODE global.HSM_MONOCHROME_MODE

#pragma parameter HSM_MONOCHROME_BRIGHTNESS							"          Monochrome Brightness" 100 1 500 1
#define HSM_MONOCHROME_BRIGHTNESS global.HSM_MONOCHROME_BRIGHTNESS / 100

#pragma parameter HSM_MONOCHROME_GAMMA 								"          Monochrome Gamma" 1.0 0.1 5 0.1
#define HSM_MONOCHROME_GAMMA global.HSM_MONOCHROME_GAMMA

#pragma parameter HSM_MONOCHROME_HUE_OFFSET 						"          Monochrome Hue Offset" 0 -180 180 1
#define HSM_MONOCHROME_HUE_OFFSET (global.HSM_MONOCHROME_HUE_OFFSET / 360)

#pragma parameter HSM_MONOCHROME_SATURATION 						"          Monochrome Saturation" 100 -100 200 1
#define HSM_MONOCHROME_SATURATION (global.HSM_MONOCHROME_SATURATION / 100)

#pragma parameter HSM_MONOCHROME_DUALSCREEN_VIS_MODE                "          Dual Screen Visibility - BOTH | SCREEN 1 | SCREEN 2" 0 0 2 1
float HSM_MONOCHROME_DUALSCREEN_VIS_MODE = global.HSM_MONOCHROME_DUALSCREEN_VIS_MODE;


//---------------------------------------------------------------------------------------------------
//    TUBE EMPTY
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_ASPECT_AND_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_ASPECT_AND_EMPTY_TITLE						"[ TUBE ASPECT & EMPTY SPACE ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_DIFFUSE_FORCE_ASPECT 			        "          Tube Aspect - EVEN | SCREEN | Explicit"  1 0 2 1
#define HSM_TUBE_DIFFUSE_FORCE_ASPECT global.HSM_TUBE_DIFFUSE_FORCE_ASPECT

#pragma parameter HSM_TUBE_EXPLICIT_ASPECT 			                "          Explicit Tube Aspect "  1.3333 0.5 3 0.01
#define HSM_TUBE_EXPLICIT_ASPECT global.HSM_TUBE_EXPLICIT_ASPECT

#pragma parameter HSM_TUBE_EMPTY_THICKNESS 				            "          Empty Tube Thickness" 0 0 6000 10
#define HSM_TUBE_EMPTY_THICKNESS (0.7 * global.HSM_TUBE_EMPTY_THICKNESS / 100)

#pragma parameter HSM_TUBE_EMPTY_THICKNESS_X_SCALE 			        "          Empty Tube Thicknes X Scale" 100 0 3000 1
#define HSM_TUBE_EMPTY_THICKNESS_X_SCALE (global.HSM_TUBE_EMPTY_THICKNESS_X_SCALE / 100)

#pragma parameter HSM_SCREEN_CORNER_RADIUS_SCALE 			        "          Screen (Game Image) Corner Radius Scale" 100 5 3000 1
#define HSM_SCREEN_CORNER_RADIUS_SCALE (global.HSM_SCREEN_CORNER_RADIUS_SCALE / 100)


//---------------------------------------------------------------------------------------------------
//    TUBE DIFFUSE IMAGE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_DIFFUSE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_TITLE						"[ TUBE DIFFUSE IMAGE ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_DIFFUSE_MODE  					        "          Tube Diffuse Color - BLACK | IMAGE | TRANSPARENT" 0 0 2 1
float HSM_TUBE_DIFFUSE_MODE = global.HSM_TUBE_DIFFUSE_MODE;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_AMOUNT                     "          Amount" 100 0 100 1
float HSM_TUBE_DIFFUSE_IMAGE_AMOUNT = global.HSM_TUBE_DIFFUSE_IMAGE_AMOUNT / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_DUALSCREEN_VIS_MODE        "          Dual Screen Visibility - BOTH | SCREEN 1 | SCREEN 2" 0 0 2 1
float HSM_TUBE_DIFFUSE_IMAGE_DUALSCREEN_VIS_MODE = global.HSM_TUBE_DIFFUSE_IMAGE_DUALSCREEN_VIS_MODE;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_COLORIZE_ON                "          Colorize On" 0 0 1 1
float HSM_TUBE_DIFFUSE_IMAGE_COLORIZE_ON = global.HSM_TUBE_DIFFUSE_IMAGE_COLORIZE_ON;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_HUE                        "          Hue Offset" 0 0 360 1
float HSM_TUBE_DIFFUSE_IMAGE_HUE = global.HSM_TUBE_DIFFUSE_IMAGE_HUE / 360;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_SATURATION                 "          Saturation" 100 0 200 1
float HSM_TUBE_DIFFUSE_IMAGE_SATURATION = global.HSM_TUBE_DIFFUSE_IMAGE_SATURATION / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_BRIGHTNESS                 "          Brightness" 15 0 500 1
float HSM_TUBE_DIFFUSE_IMAGE_BRIGHTNESS = global.HSM_TUBE_DIFFUSE_IMAGE_BRIGHTNESS / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_GAMMA                      "          Gamma Adjust" 1 0 5 0.01
float HSM_TUBE_DIFFUSE_IMAGE_GAMMA = global.HSM_TUBE_DIFFUSE_IMAGE_GAMMA;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_AMBIENT_LIGHTING           "          Ambient Lighting Multiplier" 100 0 100 1
float HSM_TUBE_DIFFUSE_IMAGE_AMBIENT_LIGHTING = global.HSM_TUBE_DIFFUSE_IMAGE_AMBIENT_LIGHTING / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_AMBIENT2_LIGHTING          "          Ambient 2nd Image Lighting Multiplier" 0 0 100 1
float HSM_TUBE_DIFFUSE_IMAGE_AMBIENT2_LIGHTING = global.HSM_TUBE_DIFFUSE_IMAGE_AMBIENT2_LIGHTING / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_SCALE  			        "          Image Scale" 140 0 1000 1
float HSM_TUBE_DIFFUSE_IMAGE_SCALE = global.HSM_TUBE_DIFFUSE_IMAGE_SCALE / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_SCALE_X  			        "          Image Scale X" 100 0 1000 1
float HSM_TUBE_DIFFUSE_IMAGE_SCALE_X = global.HSM_TUBE_DIFFUSE_IMAGE_SCALE_X / 100;

#pragma parameter HSM_TUBE_DIFFUSE_IMAGE_ROTATION  			        "          Image Rotation: -1: 270 | 0 | 1: 90" 0 -1 1 1
float HSM_TUBE_DIFFUSE_IMAGE_ROTATION = global.HSM_TUBE_DIFFUSE_IMAGE_ROTATION;


//---------------------------------------------------------------------------------------------------
//    TUBE SHADOW IMAGE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_SHADOW_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_SHADOW_IMAGE_TITLE					"[ TUBE SHADOW IMAGE ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_SHADOW_IMAGE_ON  					"          Tube Shadow Image - OFF | ON" 1 0 1 1
float HSM_TUBE_SHADOW_IMAGE_ON = global.HSM_TUBE_SHADOW_IMAGE_ON;

#pragma parameter HSM_TUBE_SHADOW_IMAGE_OPACITY                 "          Opacity" 60 0 100 2
float HSM_TUBE_SHADOW_IMAGE_OPACITY = global.HSM_TUBE_SHADOW_IMAGE_OPACITY / 100;

#pragma parameter HSM_TUBE_SHADOW_IMAGE_POS_X			        "          Position X" 0 -500 500 0.1
float HSM_TUBE_SHADOW_IMAGE_POS_X = global.HSM_TUBE_SHADOW_IMAGE_POS_X / 100;

#pragma parameter HSM_TUBE_SHADOW_IMAGE_POS_Y			        "          Position Y" 9 -500 500 0.1
float HSM_TUBE_SHADOW_IMAGE_POS_Y = global.HSM_TUBE_SHADOW_IMAGE_POS_Y / 100;

#pragma parameter HSM_TUBE_SHADOW_IMAGE_SCALE_X  			    "          Scale X" 122 0 1000 0.1
float HSM_TUBE_SHADOW_IMAGE_SCALE_X = global.HSM_TUBE_SHADOW_IMAGE_SCALE_X / 100;

#pragma parameter HSM_TUBE_SHADOW_IMAGE_SCALE_Y  			    "          Scale Y" 130 0 1000 0.1
float HSM_TUBE_SHADOW_IMAGE_SCALE_Y = global.HSM_TUBE_SHADOW_IMAGE_SCALE_Y / 100;

#pragma parameter HSM_TUBE_SHADOW_CURVATURE_SCALE			    "          Curvature Scale" 300 0 2000 1
float HSM_TUBE_SHADOW_CURVATURE_SCALE = global.HSM_TUBE_SHADOW_CURVATURE_SCALE / 100;


//---------------------------------------------------------------------------------------------------
//    TUBE GLASS STATIC REFLECTION IMAGE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_STATIC_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_STATIC_TITLE			"[ TUBE GLASS STATIC REFLECTION IMAGE ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_STATIC_REFLECTION_IMAGE_ON  				"          Use Tube Static Reflection Image - OFF | ON" 0 0 1 1
float HSM_TUBE_STATIC_REFLECTION_IMAGE_ON = global.HSM_TUBE_STATIC_REFLECTION_IMAGE_ON;

#pragma parameter HSM_TUBE_STATIC_REFLECTION_IMAGE_DUALSCREEN_VIS_MODE    "          Dual Screen Visibility - BOTH | SCREEN 1 | SCREEN 2" 0 0 2 1
float HSM_TUBE_STATIC_REFLECTION_IMAGE_DUALSCREEN_VIS_MODE = global.HSM_TUBE_STATIC_REFLECTION_IMAGE_DUALSCREEN_VIS_MODE;

#pragma parameter HSM_TUBE_STATIC_REFLECTION_IMAGE_OPACITY  		"          Opacity" 14 0 100 0.1
float HSM_TUBE_STATIC_REFLECTION_IMAGE_OPACITY = global.HSM_TUBE_STATIC_REFLECTION_IMAGE_OPACITY / 100;

#pragma parameter HSM_TUBE_STATIC_OPACITY_DIFFUSE_MULTIPLY  "          Multiply Opacity by Diffuse Amount " 25 0 100 5
float HSM_TUBE_STATIC_OPACITY_DIFFUSE_MULTIPLY = global.HSM_TUBE_STATIC_OPACITY_DIFFUSE_MULTIPLY / 100;

#pragma parameter HSM_TUBE_STATIC_BLACK_LEVEL      "          Black Level"  25 -100 100 0.5
float HSM_TUBE_STATIC_BLACK_LEVEL = global.HSM_TUBE_STATIC_BLACK_LEVEL / 1000 + 1;

#pragma parameter HSM_TUBE_STATIC_AMBIENT_LIGHTING "          Ambient Lighting Multiplier" 60 0 100 1
float HSM_TUBE_STATIC_AMBIENT_LIGHTING = global.HSM_TUBE_STATIC_AMBIENT_LIGHTING / 100;

#pragma parameter HSM_TUBE_STATIC_AMBIENT2_LIGHTING "          Ambient 2nd Image Lighting Multiplier" 0 0 100 1
float HSM_TUBE_STATIC_AMBIENT2_LIGHTING = global.HSM_TUBE_STATIC_AMBIENT2_LIGHTING / 100;

#pragma parameter HSM_TUBE_STATIC_SCALE  			"          Scale" 120 0 1000 1
float HSM_TUBE_STATIC_SCALE = global.HSM_TUBE_STATIC_SCALE / 100.0;

#pragma parameter HSM_TUBE_STATIC_SCALE_X 			"          Scale X" 100 0 1000 1
float HSM_TUBE_STATIC_SCALE_X = global.HSM_TUBE_STATIC_SCALE_X / 100.0;

#pragma parameter HSM_TUBE_STATIC_POS_X  			"          Pos X"   10 -500 500 1
float HSM_TUBE_STATIC_POS_X = global.HSM_TUBE_STATIC_POS_X / -1000;

#pragma parameter HSM_TUBE_STATIC_POS_Y  			"          Pos Y"   10 -500 500 1
float HSM_TUBE_STATIC_POS_Y = global.HSM_TUBE_STATIC_POS_Y / 1000;

#pragma parameter HSM_TUBE_STATIC_SHADOW_OPACITY  	"          Shadow Opacity" 0 0 100 1
float HSM_TUBE_STATIC_SHADOW_OPACITY = global.HSM_TUBE_STATIC_SHADOW_OPACITY / 100;

#pragma parameter HSM_TUBE_STATIC_DITHER_SAMPLES  "          Noise Samples (Reduces Banding)" 0 0 10 1
float HSM_TUBE_STATIC_DITHER_SAMPLES = global.HSM_TUBE_STATIC_DITHER_SAMPLES;

#pragma parameter HSM_TUBE_STATIC_DITHER_DISTANCE  "          Noise Distance" 5 0 10 0.5
float HSM_TUBE_STATIC_DITHER_DISTANCE = global.HSM_TUBE_STATIC_DITHER_DISTANCE;

#pragma parameter HSM_TUBE_STATIC_DITHER_AMOUNT "          Noise Amount" 0.6 0 10 0.1
float HSM_TUBE_STATIC_DITHER_AMOUNT = global.HSM_TUBE_STATIC_DITHER_AMOUNT;


//---------------------------------------------------------------------------------------------------
//    CRT Blend Over Diffuse
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_CRT_BLEND_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_CRT_BLEND_TITLE						"[ CRT & TUBE BLENDING ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_OPACITY                          "          Tube Opacity" 100 0 100 0.1
float HSM_TUBE_OPACITY = global.HSM_TUBE_OPACITY / 100;

#pragma parameter HSM_CRT_BLEND_MODE                        "          CRT Blend Mode: OFF | ADD | MULTIPLY" 1 0 2 1
#define HSM_CRT_BLEND_MODE global.HSM_CRT_BLEND_MODE

#pragma parameter HSM_CRT_BLEND_AMOUNT                      "          CRT Blend Amount" 100 0 100 1
#define HSM_CRT_BLEND_AMOUNT global.HSM_CRT_BLEND_AMOUNT / 100


//---------------------------------------------------------------------------------------------------
//    TUBE COLORED GEL
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_COLORED_GEL_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_COLORED_GEL_TITLE						"[ TUBE COLORED GEL IMAGE GENERAL ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_ON  					"          Use Tube Colored Gel - OFF | ON" 0 0 1 1
float HSM_TUBE_COLORED_GEL_IMAGE_ON = global.HSM_TUBE_COLORED_GEL_IMAGE_ON;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_SCALE  			    "          Scale" 100 0 1000 1
float HSM_TUBE_COLORED_GEL_IMAGE_SCALE = global.HSM_TUBE_COLORED_GEL_IMAGE_SCALE / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_FLIP_HORIZONTAL  		"          Flip Horizontal" 0 0 1 1
float HSM_TUBE_COLORED_GEL_IMAGE_FLIP_HORIZONTAL = -(global.HSM_TUBE_COLORED_GEL_IMAGE_FLIP_HORIZONTAL * 2 - 1);

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_FLIP_VERTICAL  		"          Flip Vertical" 0 0 1 1
float HSM_TUBE_COLORED_GEL_IMAGE_FLIP_VERTICAL = -(global.HSM_TUBE_COLORED_GEL_IMAGE_FLIP_VERTICAL * 2 - 1);

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_DUALSCREEN_VIS_MODE    "          Dual Screen Visibility - BOTH | SCREEN 1 | SCREEN 2" 0 0 2 1
float HSM_TUBE_COLORED_GEL_IMAGE_DUALSCREEN_VIS_MODE = global.HSM_TUBE_COLORED_GEL_IMAGE_DUALSCREEN_VIS_MODE;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_MULTIPLY_AMOUNT  		"          Multiply Blend Amount" 90 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_MULTIPLY_AMOUNT = global.HSM_TUBE_COLORED_GEL_IMAGE_MULTIPLY_AMOUNT / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_ADDITIVE_AMOUNT  		"          Additive Blend Amount" 0 0 100 0.05
float HSM_TUBE_COLORED_GEL_IMAGE_ADDITIVE_AMOUNT = global.HSM_TUBE_COLORED_GEL_IMAGE_ADDITIVE_AMOUNT / 100;


//---------------------------------------------------------------------------------------------------
//    TUBE COLORED GEL
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_TUBE_COLORED_GEL_NORMAL_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_TUBE_COLORED_GEL_NORMAL_TITLE					"[ TUBE COLORED GEL NORMAL BLEND ]:" 0 0 0.001 0.001

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT  		"          Blend Amount" 15 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT = global.HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_AMOUNT / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_TRANSPARENCY_THRESHOLD "          Transparency Threshold" 23 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_TRANSPARENCY_THRESHOLD = global.HSM_TUBE_COLORED_GEL_IMAGE_TRANSPARENCY_THRESHOLD / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_BRIGHTNESS  	"          Brightness" 100 0 200 1
float HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_BRIGHTNESS = global.HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_BRIGHTNESS / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_VIGNETTE  		"          Vignette Amount" 0 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_VIGNETTE = global.HSM_TUBE_COLORED_GEL_IMAGE_NORMAL_VIGNETTE / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_FAKE_SCANLINE_AMOUNT   "          Fake Scanline Amount" 0 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_FAKE_SCANLINE_AMOUNT = global.HSM_TUBE_COLORED_GEL_IMAGE_FAKE_SCANLINE_AMOUNT / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT_LIGHTING       "          Ambient Lighting Multiplier" 100 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT_LIGHTING = global.HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT_LIGHTING / 100;

#pragma parameter HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT2_LIGHTING      "          Ambient 2nd Image Lighting Multiplier" 0 0 100 1
float HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT2_LIGHTING = global.HSM_TUBE_COLORED_GEL_IMAGE_AMBIENT2_LIGHTING / 100;

// #pragma parameter HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL				"          Show CRT on Top of Colored Gel Normal" 0 0 1 1
// float HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL = global.HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL;
float HSM_SHOW_CRT_ON_TOP_OF_COLORED_GEL = 0;


//---------------------------------------------------------------------------------------------------
//    SCREEN BLACK EDGE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_SCREEN_EDGE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_SCREEN_EDGE_TITLE								"[ SCREEN BLACK EDGE ]:" 0 0 0.001 0.001

#pragma parameter HSM_GLOBAL_CORNER_RADIUS 							"          Global Corner Radius - Def 10" 10 4 500 2
#define HSM_GLOBAL_CORNER_RADIUS global.HSM_GLOBAL_CORNER_RADIUS

#pragma parameter HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE 			"          Black Edge Corner Radius Scale" 100 10 8000 10
#define HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE global.HSM_TUBE_BLACK_EDGE_CORNER_RADIUS_SCALE / 100

#pragma parameter HSM_TUBE_BLACK_EDGE_SHARPNESS 					"          Black Edge Sharpness - Def 70" 70 0 90 1
#define HSM_TUBE_BLACK_EDGE_SHARPNESS global.HSM_TUBE_BLACK_EDGE_SHARPNESS / 100

#pragma parameter HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE 				"          Black Edge Curvature Scale Multiplier" 100 20 8000 10
#define HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE global.HSM_TUBE_BLACK_EDGE_CURVATURE_SCALE / 100

#pragma parameter HSM_TUBE_BLACK_EDGE_THICKNESS 					"          Black Edge Thickness" 100 -800 6000 5
#define HSM_TUBE_BLACK_EDGE_THICKNESS (0.7 * global.HSM_TUBE_BLACK_EDGE_THICKNESS / 100)

#pragma parameter HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE 			"          Black Edge Thickness X Scale" 100 0 3000 10
#define HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE (global.HSM_TUBE_BLACK_EDGE_THICKNESS_X_SCALE / 100)


//---------------------------------------------------------------------------------------------------
//    DUAL SCREEN
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_DUALSCREEN_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_DUALSCREEN_TITLE								"[ DUAL SCREEN ]:" 0 0 0.001 0.001

#pragma parameter HSM_DUALSCREEN_MODE								"          Dual Screen Mode - OFF | VERTICAL | HORIZONTAL" 0 0 2 1
float HSM_DUALSCREEN_MODE = global.HSM_DUALSCREEN_MODE;

#pragma parameter HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE				"          Core Image Split Mode  - AUTO | VERTICAL | HORIZONTAL" 1 0 2 1
float HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE = global.HSM_DUALSCREEN_CORE_IMAGE_SPLIT_MODE;

#pragma parameter HSM_DUALSCREEN_CORE_IMAGE_SWAP_SCREENS			"          Core Image Swap Screen Content" 0 0 1 1
float HSM_DUALSCREEN_CORE_IMAGE_SWAP_SCREENS = global.HSM_DUALSCREEN_CORE_IMAGE_SWAP_SCREENS;

#pragma parameter HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET			"          Core Image Split Offset" 0 -100 100 1
float HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET = global.HSM_DUALSCREEN_CORE_IMAGE_SPLIT_OFFSET / 1000;

// #pragma parameter HSM_2ND_SCREEN_SIDES_CROP				"          Crop % from Edges of 2nd Screen" 0 0 100 0.1
// float HSM_2ND_SCREEN_SIDES_CROP = global.HSM_2ND_SCREEN_SIDES_CROP / 100;

#pragma parameter HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION			"          Viewport Split Offset" 0 -500 500 1
float HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION = global.HSM_DUALSCREEN_VIEWPORT_SPLIT_LOCATION / 1000;

#pragma parameter HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE			"          Scale Screens from Center of Split" 1 0 1 1
float HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE = global.HSM_DUALSCREEN_SHIFT_POSITION_WITH_SCALE;

#pragma parameter HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS	"          Position Offset Between Screens" 0 -250 250 0.2
float HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS = global.HSM_DUALSCREEN_POSITION_OFFSET_BETWEEN_SCREENS / 1000;

#pragma parameter HSM_2ND_SCREEN_ASPECT_RATIO_MODE		            "        2nd Screen Aspect Ratio Mode - Same as 1st Screen | PAR" 0 0 1 1
float HSM_2ND_SCREEN_ASPECT_RATIO_MODE = global.HSM_2ND_SCREEN_ASPECT_RATIO_MODE;

#pragma parameter HSM_2ND_SCREEN_INDEPENDENT_SCALE		            "          2nd Screen Use Independent Scale" 0 0 1 1
float HSM_2ND_SCREEN_INDEPENDENT_SCALE = global.HSM_2ND_SCREEN_INDEPENDENT_SCALE;

#pragma parameter HSM_2ND_SCREEN_SCALE_OFFSET			            "          2nd Screen Scale Offset" 100 1 500 0.5
float HSM_2ND_SCREEN_SCALE_OFFSET = global.HSM_2ND_SCREEN_SCALE_OFFSET / 100;

#pragma parameter HSM_2ND_SCREEN_POS_X					            "          2nd Screen Pos X" 0 -500 500 1
float HSM_2ND_SCREEN_POS_X = global.HSM_2ND_SCREEN_POS_X / 1000;

#pragma parameter HSM_2ND_SCREEN_POS_Y					            "          2nd Screen Pos Y" 0 -500 500 1
float HSM_2ND_SCREEN_POS_Y = global.HSM_2ND_SCREEN_POS_Y / 1000;

#pragma parameter HSM_2ND_SCREEN_CROP_PERCENT_ZOOM					"          2nd Screen Crop Zoom %" 0 -10 100 0.1
float HSM_2ND_SCREEN_CROP_PERCENT_ZOOM = global.HSM_2ND_SCREEN_CROP_PERCENT_ZOOM / 100;

#pragma parameter HSM_2ND_SCREEN_CROP_PERCENT_TOP 					"          2nd Screen Crop Top %" 0 -10 100 0.1
float HSM_2ND_SCREEN_CROP_PERCENT_TOP = global.HSM_2ND_SCREEN_CROP_PERCENT_TOP / 100;

#pragma parameter HSM_2ND_SCREEN_CROP_PERCENT_BOTTOM 				"          2nd Screen Crop Bottom %" 0 -10 100 0.1
float HSM_2ND_SCREEN_CROP_PERCENT_BOTTOM = global.HSM_2ND_SCREEN_CROP_PERCENT_BOTTOM / 100;

#pragma parameter HSM_2ND_SCREEN_CROP_PERCENT_LEFT 					"          2nd Screen Crop Left %" 0 -10 100 0.1
float HSM_2ND_SCREEN_CROP_PERCENT_LEFT = global.HSM_2ND_SCREEN_CROP_PERCENT_LEFT / 100;

#pragma parameter HSM_2ND_SCREEN_CROP_PERCENT_RIGHT 				"          2nd Screen Crop Right %" 0 -10 100 0.1
float HSM_2ND_SCREEN_CROP_PERCENT_RIGHT = global.HSM_2ND_SCREEN_CROP_PERCENT_RIGHT / 100;


//---------------------------------------------------------------------------------------------------
//    Reflection Transform
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_REFLECTION_APPEARANCE_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_REFLECTION_APPEARANCE_TITLE					"[ REFLECTION POSITION & SCALE ]:" 0 0 0.001 0.001

// #pragma parameter HSM_SCREEN_REFLECTION_FOLLOW_DIFFUSE_THICKNESS    "          Follow Tube Diffuse Extra Thickness" 0 1 200 1
// float HSM_SCREEN_REFLECTION_FOLLOW_DIFFUSE_THICKNESS = global.HSM_SCREEN_REFLECTION_FOLLOW_DIFFUSE_THICKNESS / 100;

#pragma parameter HSM_SCREEN_REFLECTION_SCALE     					"          Screen Reflection Scale" 100 1 300 1
float HSM_SCREEN_REFLECTION_SCALE = global.HSM_SCREEN_REFLECTION_SCALE / 100;

#pragma parameter HSM_SCREEN_REFLECTION_POS_X     					"          Screen Reflection Pos X" 0 -100 100 1
float HSM_SCREEN_REFLECTION_POS_X = global.HSM_SCREEN_REFLECTION_POS_X / 600;

#pragma parameter HSM_SCREEN_REFLECTION_POS_Y     					"          Screen Reflection Pos Y" 0 -100 100 1
float HSM_SCREEN_REFLECTION_POS_Y = global.HSM_SCREEN_REFLECTION_POS_Y / 600;



#pragma parameter HSM_AMBIENT1_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_AMBIENT1_TITLE                       "[ AMBIENT LIGHTING IMAGE 1 ]:" 0 0 0.001 0.001

#pragma parameter HSM_AMBIENT1_HUE                 	        "          Hue"  0 -180 180 1
float HSM_AMBIENT1_HUE = global.HSM_AMBIENT1_HUE / 360;

#pragma parameter HSM_AMBIENT1_SATURATION          	        "          Saturation"  100 0 300 1
float HSM_AMBIENT1_SATURATION = global.HSM_AMBIENT1_SATURATION / 100;

#pragma parameter HSM_AMBIENT1_VALUE               	        "          Value"  120 0 400 1
float HSM_AMBIENT1_VALUE = global.HSM_AMBIENT1_VALUE / 100;

#pragma parameter HSM_AMBIENT1_CONTRAST               	    "          Contrast"  100 0 200 0.5
float HSM_AMBIENT1_CONTRAST = global.HSM_AMBIENT1_CONTRAST / 100;

#pragma parameter HSM_AMBIENT1_SCALE_KEEP_ASPECT           "        Scale Aspect - MATCH VIEWPORT | USE TEXURE ASPECT"  1 0 1 1
float HSM_AMBIENT1_SCALE_KEEP_ASPECT = global.HSM_AMBIENT1_SCALE_KEEP_ASPECT;

#pragma parameter HSM_AMBIENT1_SCALE_INHERIT_MODE         	"          Scale Inheritance - OFF | ZOOM | TUBE"  2 0 2 1
float HSM_AMBIENT1_SCALE_INHERIT_MODE = global.HSM_AMBIENT1_SCALE_INHERIT_MODE;

#pragma parameter HSM_AMBIENT1_SCALE             	        "          Scale Offset"  250 10 1000 1
float HSM_AMBIENT1_SCALE = global.HSM_AMBIENT1_SCALE / 100;

#pragma parameter HSM_AMBIENT1_SCALE_X             	        "          Scale Offset X"  100 10 1000 0.5
float HSM_AMBIENT1_SCALE_X = global.HSM_AMBIENT1_SCALE_X / 100;

#pragma parameter HSM_AMBIENT1_ROTATE         	            "          Rotate"  0 0 1 1
float HSM_AMBIENT1_ROTATE = global.HSM_AMBIENT1_ROTATE;

#pragma parameter HSM_AMBIENT1_MIRROR_HORZ         	        "          Mirror Horizontal"  0 0 1 1
float HSM_AMBIENT1_MIRROR_HORZ = global.HSM_AMBIENT1_MIRROR_HORZ;

#pragma parameter HSM_AMBIENT1_POS_INHERIT_MODE         	"          Position Inheritance - OFF | ON"  1 0 1 1
float HSM_AMBIENT1_POS_INHERIT_MODE = global.HSM_AMBIENT1_POS_INHERIT_MODE;

#pragma parameter HSM_AMBIENT1_POSITION_X 					"          Position X" 0 -1500 1500 1
float HSM_AMBIENT1_POSITION_X = global.HSM_AMBIENT1_POSITION_X / -1000;

#pragma parameter HSM_AMBIENT1_POSITION_Y 					"          Position Y" 0 -1500 1500 1
float HSM_AMBIENT1_POSITION_Y = -1 * global.HSM_AMBIENT1_POSITION_Y / -1000;

// Commented out because I don't think anyone is using this
// #pragma parameter HSM_AMBIENT1_DITHERING_SAMPLES   "          Dithering Noise Samples (Reduces Banding)" 0 0 10 1
// float HSM_AMBIENT1_DITHERING_SAMPLES = global.HSM_AMBIENT1_DITHERING_SAMPLES;
float HSM_AMBIENT1_DITHERING_SAMPLES = 0;


//---------------------------------------------------------------------------------------------------
//    Ambient Lighting Image 2
//---------------------------------------------------------------------------------------------------

#pragma parameter HSM_AMBIENT2_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_AMBIENT2_TITLE                            "[ AMBIENT LIGHTING IMAGE 2 ]:" 0 0 0.001 0.001

#pragma parameter HSM_AMBIENT2_HUE                 	            "          Hue"  0 -180 180 1
float HSM_AMBIENT2_HUE = global.HSM_AMBIENT2_HUE / 360;

#pragma parameter HSM_AMBIENT2_SATURATION          	            "          Saturation"  100 0 300 1
float HSM_AMBIENT2_SATURATION = global.HSM_AMBIENT2_SATURATION / 100;

#pragma parameter HSM_AMBIENT2_VALUE               	            "          Value"  100 0 400 1
float HSM_AMBIENT2_VALUE = global.HSM_AMBIENT2_VALUE / 100;

#pragma parameter HSM_AMBIENT2_CONTRAST               	        "          Contrast"  100 0 200 0.5
float HSM_AMBIENT2_CONTRAST = global.HSM_AMBIENT2_CONTRAST / 100;

#pragma parameter HSM_AMBIENT2_SCALE_KEEP_ASPECT                "        Scale Aspect - MATCH VIEWPORT | USE TEXURE ASPECT"  0 0 1 1
float HSM_AMBIENT2_SCALE_KEEP_ASPECT = global.HSM_AMBIENT2_SCALE_KEEP_ASPECT;

#pragma parameter HSM_AMBIENT2_SCALE_INHERIT_MODE         	    "          Scale Inheritance - OFF | ZOOM | TUBE"  1 0 2 1
float HSM_AMBIENT2_SCALE_INHERIT_MODE = global.HSM_AMBIENT2_SCALE_INHERIT_MODE;

#pragma parameter HSM_AMBIENT2_SCALE             	            "          Scale Offset"  100 10 1000 0.5
float HSM_AMBIENT2_SCALE = global.HSM_AMBIENT2_SCALE / 100;

#pragma parameter HSM_AMBIENT2_SCALE_X             	            "          Scale Offset X"  100 10 1000 0.5
float HSM_AMBIENT2_SCALE_X = global.HSM_AMBIENT2_SCALE_X / 100;

#pragma parameter HSM_AMBIENT2_ROTATE         	                "          Rotate"  0 0 1 1
float HSM_AMBIENT2_ROTATE = global.HSM_AMBIENT2_ROTATE;

#pragma parameter HSM_AMBIENT2_MIRROR_HORZ         	            "          Mirror Horizontal"  0 0 1 1
float HSM_AMBIENT2_MIRROR_HORZ = global.HSM_AMBIENT2_MIRROR_HORZ;

#pragma parameter HSM_AMBIENT2_POS_INHERIT_MODE         	    "          Position Inheritance - OFF | ON"  1 0 1 1
float HSM_AMBIENT2_POS_INHERIT_MODE = global.HSM_AMBIENT2_POS_INHERIT_MODE;

#pragma parameter HSM_AMBIENT2_POSITION_X 						"          Position X" 0 -1000 1000 1
float HSM_AMBIENT2_POSITION_X = global.HSM_AMBIENT2_POSITION_X / -1000;

#pragma parameter HSM_AMBIENT2_POSITION_Y 						"          Position Y" 0 -1000 1000 1
float HSM_AMBIENT2_POSITION_Y = -1 * global.HSM_AMBIENT2_POSITION_Y / -1000;


//---------------------------------------------------------------------------------------------------
//    BEZEL INDEPENDENT SCALE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_BEZEL_INDEPENDENT_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_BEZEL_INDEPENDENT_TITLE						"[ BEZEL INDEPENDENT SCALE & CURVATURE ]:" 0 0 0.001 0.001

#pragma parameter HSM_BZL_USE_INDEPENDENT_SCALE 					"          Use Independent Scale" 0 0 1 1
#define HSM_BZL_USE_INDEPENDENT_SCALE global.HSM_BZL_USE_INDEPENDENT_SCALE

#pragma parameter HSM_BZL_INDEPENDENT_SCALE 						"          Independent Scale (When scale from image is not used)" 82.97 1 200 0.1
#define HSM_BZL_INDEPENDENT_SCALE global.HSM_BZL_INDEPENDENT_SCALE / 100

#pragma parameter HSM_BZL_USE_INDEPENDENT_CURVATURE 				"          Use Independent Curvature" 0 0 1 1
#define HSM_BZL_USE_INDEPENDENT_CURVATURE global.HSM_BZL_USE_INDEPENDENT_CURVATURE

#pragma parameter HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS 	"          Independent Curvature X" 100 0 400 5
#define HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS global.HSM_BZL_INDEPENDENT_CURVATURE_SCALE_LONG_AXIS / 100

#pragma parameter HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS 	"          Independent Curvature Y" 100 0 400 5
#define HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS global.HSM_BZL_INDEPENDENT_CURVATURE_SCALE_SHORT_AXIS / 100


//---------------------------------------------------------------------------------------------------
//    BEZEL GENERAL
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_BEZEL_GENERAL_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_BEZEL_GENERAL_TITLE							"[ BEZEL GENERAL ]:" 0 0 0.001 0.001

#pragma parameter HSM_BZL_OPACITY                                   "          Opacity" 100 0 100 5
float HSM_BZL_OPACITY = global.HSM_BZL_OPACITY / 100;

#pragma parameter HSM_BZL_BLEND_MODE                                "          Blend Mode - Off | Normal | Add | Mult"  1 0 3 1
float HSM_BZL_BLEND_MODE = global.HSM_BZL_BLEND_MODE;

#pragma parameter HSM_BZL_WIDTH 						            "          Width - Def 125" 125 -250 5000 1
float HSM_BZL_WIDTH = global.HSM_BZL_WIDTH * 0.0008624;

#pragma parameter HSM_BZL_HEIGHT 					                "          Height" 100 -250 5000 1
float HSM_BZL_HEIGHT = global.HSM_BZL_HEIGHT * 0.0008732;

#pragma parameter HSM_BZL_SCALE_OFFSET 								"          Scale Offset" 100 1 200 0.1
#define HSM_BZL_SCALE_OFFSET global.HSM_BZL_SCALE_OFFSET / 100

#pragma parameter HSM_BZL_INNER_CURVATURE_SCALE 					"          Inner Curvature Scale Multiplier" 100 0 8000 10
#define HSM_BZL_INNER_CURVATURE_SCALE global.HSM_BZL_INNER_CURVATURE_SCALE / 100

#pragma parameter HSM_BZL_INNER_CORNER_RADIUS_SCALE	    			"          Inner Corner Radius Scale - Def 80" 80 0 8000 1
float HSM_BZL_INNER_CORNER_RADIUS_SCALE = global.HSM_BZL_INNER_CORNER_RADIUS_SCALE / 100;
