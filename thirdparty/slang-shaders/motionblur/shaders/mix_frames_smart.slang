#version 450

/*
	mix_frames_smart - performs 50:50 blending between the current and 
	previous frames, but only if pixels repeatedly switch state on alternate
	frames (i.e. prevents flicker on games that use LCD ghosting for transparency,
	without blurring the entire screen). This is not 100% effective, but 'good
	enough' in many cases (e.g. it fixes map rendering issues in F-Zero GP on the GBA).
	Works best when flickering objects are in a fixed location.
	
	Author: jdgleaver
	
	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.
*/

// User-specified fudge factor. Increasing this value loosens up the
// detection of repeated 'flicker' frames. This is required for
// games like Boktai on the GBA, where the character shadow flickers
// on and off between frames, but is sometimes overlaid with a screen
// shading effect (so checking for pixel RGB equality fails - need to
// check whether pixels are 'almost' equal)
#pragma parameter DEFLICKER_EMPHASIS "Deflicker Emphasis" 0.0 0.0 1.0 0.01

layout(push_constant) uniform Push
{
	float DEFLICKER_EMPHASIS;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/*
    VERTEX_SHADER
*/
void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord   = TexCoord;
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 6) uniform sampler2D OriginalHistory4;
layout(set = 0, binding = 7) uniform sampler2D OriginalHistory5;

#define EPSILON 0.000001

float is_equal(vec3 x, vec3 y)
{
	vec3 result = 1.0 - abs(sign(x - y));
	return min(min(result.r, result.g), result.b);
}

float is_approx_equal(vec3 x, vec3 y)
{
	vec3 result = 1.0 - step(EPSILON + registers.DEFLICKER_EMPHASIS, abs(x - y));
	return min(min(result.r, result.g), result.b);
}

/*
    FRAGMENT SHADER
*/
void main()
{
	// Get pixel colours of current + last 5 frames
	// NB: Using fewer frames results in too many false positives
	vec3 colour0 = texture(Source, vTexCoord.xy).rgb;
	vec3 colour1 = texture(OriginalHistory1, vTexCoord.xy).rgb;
	vec3 colour2 = texture(OriginalHistory2, vTexCoord.xy).rgb;
	vec3 colour3 = texture(OriginalHistory3, vTexCoord.xy).rgb;
	vec3 colour4 = texture(OriginalHistory4, vTexCoord.xy).rgb;
	vec3 colour5 = texture(OriginalHistory5, vTexCoord.xy).rgb;
	
	// Determine whether mixing should occur
	// i.e. whether alternate frames have the same pixel colour, but
	// adjacent frames do not (don't need to check colour0 != colour1,
	// since if this is true the mixing will do nothing)
	float doMix =   (1.0 - is_equal(colour0, colour3))
					  * (1.0 - is_equal(colour0, colour5))
					  * (1.0 - is_equal(colour1, colour2))
					  * (1.0 - is_equal(colour1, colour4))
					  * (1.0 - is_equal(colour2, colour3))
					  * (1.0 - is_equal(colour2, colour5))
					  * min(
					       (is_approx_equal(colour0, colour2) * is_approx_equal(colour2, colour4)) +
					       (is_approx_equal(colour1, colour3) * is_approx_equal(colour3, colour5)),
					       1.0
					    );
	
	// Mix colours
	colour0.rgb = mix(colour0.rgb, colour1.rgb, doMix * 0.5);
	
	FragColor = vec4(colour0.rgb, 1.0);
}
