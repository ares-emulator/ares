#version 450

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/
// Shader that replicates the Vivid mode of Nintendo Switch OLED Model --

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	float mode, white_toggle;
} global;

#pragma parameter mode "Color Profile (1=sRGB, 2=DCI, 3=Rec2020)" 1.0 1.0 3.0 1.0
int color_mode = int(global.mode);

#pragma parameter white_toggle "Full White Scale" 1.0 0.0 1.0 1.0
bool white = bool(global.white_toggle);

#define target_gamma 2.0
#define display_gamma 2.0

/*
We'll define our color weights in this pattern:
	r,   rg,  rb,  0.0,  //red channel
	gr,  g,   gb,  0.0,  //green channel
	br,  bg,  b,   0.0,  //blue channel
	blr, blg, blb, lum   //alpha channel; we'll hide lum at the end, too
*/

const mat4 SWITCH_sRGB = mat4(
	1.54, -0.045, -0.0225, 0.0,  //red channel
	-0.48, 1.0625, -0.075, 0.0,  //green channel
	-0.06, -0.0175, 1.1025, 0.0,  //blue channel
	0.0,  0.0,  0.0,  0.65   //alpha channel
); 

const mat4 SWITCH_sRGB_white = mat4(
	1.54, -0.045, -0.0225, 0.0,  //red channel
	-0.48, 1.0625, -0.075, 0.0,  //green channel
	-0.06, -0.0175, 1.1025, 0.0,  //blue channel
	0.0,  0.0,  0.0,  1.0   //alpha channel
); 

const mat4 SWITCH_DCI = mat4(
	1.2125, 0.0025, 0.0, 0.0,  //red channel
	-0.1625, 1.02, 0.0, 0.0,  //green channel
	-0.05, -0.0225, 1.0, 0.0,  //blue channel
	0.0,   0.0,   0.0,   0.825  //alpha channel
); 

const mat4 SWITCH_DCI_white = mat4(
	1.2125, 0.0025, 0.0, 0.0,  //red channel
	-0.1625, 1.02, 0.0, 0.0,  //green channel
	-0.05, -0.0225, 1.0, 0.0,  //blue channel
	0.0,   0.0,   0.0,   1.0  //alpha channel
); 

const mat4 SWITCH_Rec2020 = mat4(
	0.865, 0.0475, 0.0, 0.0,  //red channel
	0.12, 0.9575, 0.02, 0.0,  //green channel
	0.015, -0.005, 0.98, 0.0,  //blue channel
	0.0,   0.0,  0.0,   0.995  //alpha channel
); 

const mat4 SWITCH_Rec2020_white = mat4(
	0.865, 0.0475, 0.0, 0.0,  //red channel
	0.12, 0.9575, 0.02, 0.0,  //green channel
	0.015, -0.005, 0.98, 0.0,  //blue channel
	0.0,   0.0,  0.0,   1.0  //alpha channel
); 

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out mat4 profile;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;

	if (color_mode == 1) profile = (!white) ? SWITCH_sRGB : SWITCH_sRGB_white;
	else if (color_mode == 2) profile = (!white) ? SWITCH_DCI : SWITCH_DCI_white;
	else if (color_mode == 3) profile = (!white) ? SWITCH_Rec2020 : SWITCH_Rec2020_white;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in mat4 profile;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	// bring out our stored luminance value
	float lum = profile[3].w;

	// our adjustments need to happen in linear gamma
	vec4 screen = pow(texture(Source, vTexCoord), vec4(target_gamma)).rgba;

	screen = clamp(screen * lum, 0.0, 1.0);
	screen = profile * screen;
	FragColor = pow(screen, vec4(1.0 / display_gamma));
}