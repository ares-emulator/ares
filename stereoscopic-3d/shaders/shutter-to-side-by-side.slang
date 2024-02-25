#version 450

// Shutter 3D to side-by-side
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float ZOOM, vert_pos, horz_pos, separation, flicker, height_mod, swap_eye;
} params;

#pragma parameter ZOOM "Zoom" 1.0 0.0 2.0 0.01
#define ZOOM params.ZOOM
#pragma parameter vert_pos "Vertical Modifier" 0.0 -2.0 2.0 0.01
#define vert_pos params.vert_pos
#pragma parameter horz_pos "Horizontal Modifier" 0.0 -2.0 2.0 0.01
#define horz_pos params.horz_pos
#pragma parameter separation "Eye Separation" 0.0 -2.0 2.0 0.01
#define separation params.separation
#pragma parameter flicker "Hold Last Frame (reduce flicker)" 1.0 0.0 1.0 0.25
#define flicker params.flicker
#pragma parameter height_mod "Image Height" 1.0 0.0 2.0 0.01
#define height_mod params.height_mod
#pragma parameter swap_eye "Swap Eye Sequence" 0.0 0.0 1.0 1.0
#define swap_eye params.swap_eye

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 left_coord;
layout(location = 1) out vec2 right_coord;
layout(location = 2) out float timer;

void main()
{
   gl_Position = global.MVP * Position;

	vec2 temp_coord = TexCoord.xy - 0.5;
	temp_coord *= ZOOM;
	temp_coord *= vec2(2.,1. / height_mod);
	temp_coord += vec2(horz_pos, vert_pos);
	temp_coord += 0.5;
    left_coord  = temp_coord.xy - vec2(0.5 + separation,0.);
	right_coord = temp_coord.xy + vec2(0.5 + separation,0.);
	timer = abs(swap_eye - mod(float(params.FrameCount), 2.));
}

#pragma stage fragment
layout(location = 0) in vec2 left_coord;
layout(location = 1) in vec2 right_coord;
layout(location = 2) in float timer;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;

#define PrevTexture OriginalHistory1

void main()
{
   vec4 left_screen = texture(Source, left_coord);
   vec4 left_hold = texture(PrevTexture, left_coord);
   vec4 right_screen = texture(Source, right_coord);
   vec4 right_hold = texture(PrevTexture, right_coord);
   left_screen = left_screen * timer + (1. - timer) * left_hold * flicker;
   right_screen = right_screen * (1. - timer) + right_hold * timer * flicker;
   FragColor = left_screen + right_screen;
}
