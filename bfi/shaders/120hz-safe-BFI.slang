#version 450

// 120 Hz non-voltage-accumulating BFI
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	float deadline, debug_toggle;
} params;

#pragma parameter deadline "Max Cadence Flip Interval (in min)" 1.0 0.25 10.0 0.25
#pragma parameter debug_toggle "Debug (show only cadence swaps)" 0.0 0.0 1.0 1.0
bool debug = bool(params.debug_toggle);

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
layout(set = 0, binding = 3) uniform sampler2D PassFeedback0;

#define feedback PassFeedback0

void main()
{
	// The current cadence, which we store from frame to frame.
	float cadence = texture(feedback, vec2(0.5,0.5)).a;
	// How often to flip the cadence of the BFI. Default is 1 min, assuming 60 frames per second, but can be reduced/extended.
	float timer = mod(global.FrameCount, 3600.0 * params.deadline);
	// Flip the cadence whenever the timer rolls over.
	cadence = (int(timer) == 0) ? float(!bool(cadence)) : cadence;
	// The BFI tick oscillates between 0 and 1. TODO/FIXME: make this work with arbitrary total subframe values (or, at least evenly numbered ones)
	float bfiTick = clamp(float(global.CurrentSubFrame - 1), 0.0, 1.0);
	bfiTick = (bool(cadence)) ? bfiTick : float(!bool(bfiTick));
	// sample the image
	vec3 img = texture(Source, vTexCoord).rgb;
	FragColor.rgb = img * bfiTick;
	// Store the cadence in the alpha channel where it won't cause any trouble
	FragColor.a = cadence;
	if(debug) FragColor.rgb = FragColor.aaa;
}