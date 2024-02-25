#version 450

/*
	Anti-Flicker shader
	by hunterk
	License: public domain
	
	This shader detects large variations in luminance from frame to frame
	and then blends frames to smooth the transition. In flicker-based
	shadow effects, this should result in true transparency.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float THRESH;
} params;

#pragma parameter THRESH "Luma Diff Threshold" 0.25 0.0 1.0 0.05

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;

void main()
{
	// sample the textures
	vec4 current = texture(Source, vTexCoord);
	vec4 prev1 = texture(OriginalHistory1, vTexCoord);
	vec4 prev2 = texture(OriginalHistory2, vTexCoord);
	vec4 prev3 = texture(OriginalHistory3, vTexCoord);
	
	// get luma for comparison
	float cur_lum = dot(current.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev1_lum = dot(prev1.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev2_lum = dot(prev2.rgb, vec3(0.2125, 0.7154, 0.0721));
	float prev3_lum = dot(prev3.rgb, vec3(0.2125, 0.7154, 0.0721));
	
	// Test whether the luma difference between the pixel in the current frame and that of
	// the previous frame exceeds the threshold while the difference between the current frame
	// and 2 frames previous is below the threshold.
	// Repeat the process for the previous frame's pixel to reduce false-positives
	bool flicker = (abs(cur_lum - prev1_lum) > params.THRESH && abs(cur_lum - prev2_lum) < params.THRESH) &&
		(abs(prev1_lum - prev2_lum) > params.THRESH && abs(prev1_lum - prev3_lum) < params.THRESH);
	
	// Average the current frame with the previous frame in linear color space to avoid over-darkening
	vec4 blended = (pow(current, vec4(2.2)) + pow(prev1, vec4(2.2))) / 2.0;
	
	// delinearize the averaged result
	blended = pow(blended, vec4(1.0 / 2.2));
	
	FragColor = (!flicker) ? current : blended;
}