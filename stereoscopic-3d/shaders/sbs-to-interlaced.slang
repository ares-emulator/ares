#version 450

// side-by-side to interlaced 3D
// author: hunterk
// license: public domain

// convert side-by-side content to work with interlaced 3D displays

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float split_dist, interlace_zoom, interlace_eye_swap, aspect_mod, crop, vert;
} params;

#pragma parameter split_dist "Eye Separation" 0.25 -1.0 1.0 0.001
#pragma parameter interlace_zoom "Interlace Zoom" 0.8 0.0 2.0 0.01
#pragma parameter interlace_eye_swap "Interlace Swap Eyes" 0.0 0.0 1.0 1.0
#pragma parameter aspect_mod "Aspect Mod" 0.55 0.0 2.0 0.01
#pragma parameter crop "Crop Edges" 0.5 0.0 1.0 0.005
#pragma parameter vert "Swap Interlacing Direction (horz / vert)" 0.0 0.0 1.0 1.0

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

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = (((TexCoord - 0.5) * params.interlace_zoom) * vec2(params.aspect_mod, 1.0)) + 0.5;
   leftCoord = vTexCoord + vec2(params.split_dist, 0.);
   rightCoord = vTexCoord - vec2(params.split_dist, 0.);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 leftCoord;
layout(location = 2) in vec2 rightCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   // interlaced displays want every other physical pixel, so use gl_FragCoord
   float fragcoord = mix(gl_FragCoord.x, gl_FragCoord.y, params.vert);
   vec2 oscillator = vec2(mod(fragcoord.x, 2.0), mod(fragcoord.x + 1.0, 2.0));
   vec4 left_eye = texture(Source, leftCoord);
   left_eye = (leftCoord.x < 1.0 && leftCoord.x > params.crop)
      ? left_eye : vec4(0.0);
   vec4 right_eye = texture(Source, rightCoord);
   right_eye = (rightCoord.x > 0.0 && rightCoord.x < params.crop)
      ? right_eye : vec4(0.0);
   bool eye_picker = mix(oscillator.x, oscillator.y, params.interlace_eye_swap) > 1.0;
   FragColor = (eye_picker) ? left_eye : right_eye;
}
