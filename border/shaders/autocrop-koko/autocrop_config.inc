/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////
//
//  autocrop-koko
//  Copyright (C) 2024 Antonio Orefice <kokoko3k@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 3 of the License, or any later version.
//
////////////////////////////////////////////////////////////////////////////////

layout(push_constant) uniform Push {
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;


layout(std140, set = 0, binding = 0) uniform UBO {
	mat4 MVP;
	float AUTOCROP_MAX;
	float AUTOCROP_MIN;
	float AUTOCROP_SAMPLES;
	float AUTOCROP_SAMPLE_SIZE;
	float AUTOCROP_TRANSITION_SPEED;
	float AUTOCROP_STEADINESS;
} global;


#pragma parameter AUTOCROP_MAX	            "★ Autocrop: maximum amount"                                     25.0   0.0    0.5  0.01
#pragma parameter AUTOCROP_MIN	            "        Number of mandatory lines to crop"                       1.0   0.0   10.0  1.0
#pragma parameter AUTOCROP_SAMPLES          "        Samples per frame (faster response, higher gpu use)"    20.0   0.0  300.0  1.0
#pragma parameter AUTOCROP_SAMPLE_SIZE      "        Sample size (big speedup, less accurate)"                2.0   0.0    5.0  1.0  
#pragma parameter AUTOCROP_STEADINESS       "        Scene change treshold (0 = continuous cropping)"         0.2   0.0    0.5  0.01
#pragma parameter AUTOCROP_TRANSITION_SPEED "        Transition speed"    		                              0.1   0.05   1.0  0.05


#define AUTOCROP_MAX              global.AUTOCROP_MAX
#define AUTOCROP_MIN              global.AUTOCROP_MIN
#define AUTOCROP_SAMPLES          global.AUTOCROP_SAMPLES
#define AUTOCROP_SAMPLE_SIZE      global.AUTOCROP_SAMPLE_SIZE
#define AUTOCROP_TRANSITION_SPEED global.AUTOCROP_TRANSITION_SPEED
#define AUTOCROP_STEADINESS       global.AUTOCROP_STEADINESS

//Coords holding the autocrop zoom value
#define AUTOCROP_SAMPLING_POINT vec2(0.125)

//How much colors can differ to to be considered part of a solid area
#define AUTOCROP_TOLERANCE (1.0/255.0)

#define eps 1e-5