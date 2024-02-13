#version 450

// side-by-side to shutter 3D
// author: hunterk
// license: public domain

// convert side-by-side content to work with shutter glasses

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float split_dist, shutter_zoom, shutter_eye_swap, aspect_mod, crop;
} params;

#pragma parameter split_dist "Eye Separation" 0.25 -1.0 1.0 0.001
#pragma parameter shutter_zoom "Shutter Zoom" 0.8 0.0 2.0 0.01
#pragma parameter shutter_eye_swap "Shutter Swap Eyes" 0.0 0.0 1.0 1.0
#pragma parameter aspect_mod "Aspect Mod" 0.55 0.0 2.0 0.01
#pragma parameter crop "Crop Edges" 0.5 0.0 1.0 0.005

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 leftCoord;
layout(location = 2) out vec2 rightCoord;
layout(location = 3) out vec2 oscillator;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = (((TexCoord - 0.5) * params.shutter_zoom) * vec2(params.aspect_mod, 1.0)) + 0.5;
   leftCoord = vTexCoord + vec2(params.split_dist, 0.);
   rightCoord = vTexCoord - vec2(params.split_dist, 0.);
   oscillator.x = mod(float(params.FrameCount), 2.0);
   oscillator.y = 1. - oscillator.x;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 leftCoord;
layout(location = 2) in vec2 rightCoord;
layout(location = 3) in vec2 oscillator;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec4 left_eye = texture(Source, leftCoord);
   left_eye = (leftCoord.x < 1.0 && leftCoord.x > params.crop)
      ? left_eye : vec4(0.0);
   vec4 right_eye = texture(Source, rightCoord);
   right_eye = (rightCoord.x > 0.0 && rightCoord.x < params.crop)
      ? right_eye : vec4(0.0);
   FragColor = mix(left_eye, right_eye, (bool(params.shutter_eye_swap)) ? oscillator.x : oscillator.y);
}
