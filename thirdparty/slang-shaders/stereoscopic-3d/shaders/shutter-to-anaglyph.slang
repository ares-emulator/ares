#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float flip_eye_toggle;
   float green;
   float blue;
} params;

#pragma parameter flip_eye_toggle "Flip Left/Right" 0.0 0.0 1.0 1.0
bool flip_eye = bool(params.flip_eye_toggle);
#pragma parameter green "Green" 1.0 0.0 1.0 1.0
#pragma parameter blue "Blue" 1.0 0.0 1.0 1.0

const vec3 rec_709_luma = vec3(0.212, 0.701, 0.087);

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float oscillator;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   oscillator = float(mod(params.FrameCount, 2.));
   oscillator = (flip_eye) ? oscillator : 1. - oscillator;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float oscillator;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;

void main()
{
// convert current and previous frames to luma to preserve details
   vec3 curr = texture(Source, vTexCoord).rgb;
   float curr_lum = dot(curr, rec_709_luma);
   vec3 last = texture(OriginalHistory1, vTexCoord).rgb;
   float last_lum = dot(last, rec_709_luma);
// image for left/right eyes alternate on each frame
   float one_eye = last_lum * oscillator + (curr_lum * (1. - oscillator));
   float other_eye = curr_lum * oscillator + (last_lum * (1. - oscillator));
// one eye gets the red channel, the other eye goes to green and/or blue, depending on glasses
   vec3 combined = vec3(one_eye, other_eye * params.green, other_eye * params.blue);
   FragColor = vec4(combined, 1.0);
}
