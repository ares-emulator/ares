#version 450

// 120 Hz non-voltage-accumulating BFI
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	float deadline;
} params;

#pragma format R16G16B16A16_SFLOAT
#pragma alias calcPass

#pragma parameter deadline "Max Cadence Flip Interval (in min)" 1.0 0.25 10.0 0.25

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	vec4 FinalViewportSize;
	uint FrameCount;
	uint FrameDirection;
	uint CurrentSubFrame;
	uint TotalSubFrames;
	uint Rotation;
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
layout(set = 0, binding = 3) uniform sampler2D calcPassFeedback;

#define feedback calcPassFeedback

void main()
{
	if(global.TotalSubFrames < 2)
	{
		return;
	}
	else
	{
		vec4 stored = texture(feedback, vec2(0.5,0.5));
		// Our manually tracked framecount
		float counter = stored.x;
		// The current cadence, which we store from frame to frame.
		float cadence = stored.y;
		// When to roll over the counter
		float rollover = 3600.0 * params.deadline;
		// Sample the original image to detect dark/black screens
		vec3 img = texture(Source, vTexCoord, 9.0).rgb;
		float luma = dot(img, vec3(0.2126, 0.7152, 0.0722));
		// Go ahead and reset the counter if we're within some threshold of the target whenever we get a dark/black screen
		counter = ((luma < 0.05) && (counter > (0.5 * rollover))) ? 0.0 : counter;
		// How often to flip the cadence of the BFI. Default is 1 min, assuming 60 frames per second, but can be reduced/extended.
		counter = mod(counter, rollover);
		// Flip the cadence whenever the counter rolls over.
		cadence = (int(counter) < 1) ? float(!bool(cadence)) : cadence;
		// The BFI tick oscillates between 0 and 1. TODO/FIXME: make this work with arbitrary total subframe values (or, at least evenly numbered ones)
		float flickerTicker = clamp(float(global.CurrentSubFrame - 1), 0.0, 1.0);
		flickerTicker = (bool(cadence)) ? flickerTicker : float(!bool(flickerTicker));
		// Tick the counter
		counter += 1.0;
		// Store the values we want to mess with later, either in the next pass or the next frame
		FragColor.x = counter;
		FragColor.y = cadence;
		FragColor.z = flickerTicker;
		FragColor.w = luma;
	}
}