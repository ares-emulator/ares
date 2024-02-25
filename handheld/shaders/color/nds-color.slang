#version 450

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/
// Shader that replicates the LCD Colorspace from a Nintendo DS Phat --

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

#pragma parameter white_toggle "Toggle White Balance" 1.0 0.0 1.0 1.0
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

const mat4 NDS_sRGB = mat4(
	0.705, 0.09, 0.1075, 0.0,  //red channel
	0.235, 0.585, 0.1725, 0.0,  //green channel
	-0.075, 0.24, 0.72, 0.0,  //blue channel
	0.0,  0.0,  0.0,  1.0   //alpha channel
); 

const mat4 NDS_sRGB_white = mat4(
	0.815, 0.10, 0.1075, 0.0,  //red channel
	0.275, 0.64, 0.1725, 0.0,  //green channel
	-0.09, 0.26, 0.72, 0.0,  //blue channel
	0.0,  0.0,  0.0,  0.915   //alpha channel
); 

const mat4 NDS_DCI = mat4(
	0.605, 0.115, 0.115, 0.0,  //red channel
	0.295, 0.575, 0.205, 0.0,  //green channel
	-0.03, 0.23, 0.68, 0.0,  //blue channel
	0.0,   0.0,   0.0,   1.0  //alpha channel
); 

const mat4 NDS_DCI_white = mat4(
	0.69, 0.125, 0.12, 0.0,  //red channel
	0.335, 0.625, 0.20, 0.0,  //green channel
	-0.025, 0.25, 0.68, 0.0,  //blue channel
	0.0,   0.0,   0.0,   0.975  //alpha channel
); 

const mat4 NDS_Rec2020 = mat4(
	0.495, 0.135, 0.1175, 0.0,  //red channel
	0.345, 0.555, 0.2075, 0.0,  //green channel
	0.055, 0.225, 0.675, 0.0,  //blue channel
	0.0,   0.0,   0.0,   1.0  //alpha channel
); 

const mat4 NDS_Rec2020_white = mat4(
	0.55, 0.1475, 0.1175, 0.0,  //red channel
	0.39, 0.61, 0.2075, 0.0,  //green channel
	0.06, 0.2425, 0.675, 0.0,  //blue channel
	0.0,   0.0,   0.0,   1.0  //alpha channel
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

	if (color_mode == 1) profile = (!white) ? NDS_sRGB : NDS_sRGB_white;
	else if (color_mode == 2) profile = (!white) ? NDS_DCI : NDS_DCI_white;
	else if (color_mode == 3) profile = (!white) ? NDS_Rec2020 : NDS_Rec2020_white;
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