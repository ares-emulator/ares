#version 450

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/
// Shader that replicates the LCD Colorspace from a PSP 1000 Model --

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

#define target_gamma 2.2
#define display_gamma 2.2

/*
We'll define our color weights in this pattern:
	r,   rg,  rb,  0.0,  //red channel
	gr,  g,   gb,  0.0,  //green channel
	br,  bg,  b,   0.0,  //blue channel
	blr, blg, blb, lum   //alpha channel; we'll hide lum at the end, too
*/

const mat4 PSP_sRGB = mat4(
	0.525, 0.0525, 0.09, 0.0,  //red channel
	0.13, 0.545, 0.15, 0.0,  //green channel
	-0.07, 0.1975, 0.76, 0.0,  //blue channel
	0.0,  0.0,  0.0,  1.0   //alpha channel
); 

const mat4 PSP_sRGB_white = mat4(
	0.90, 0.065, 0.09, 0.0,  //red channel
	0.225, 0.685, 0.15, 0.0,  //green channel
	-0.125, 0.25, 0.76, 0.0,  //blue channel
	0.0,  0.0,  0.0,  0.89   //alpha channel
); 

const mat4 PSP_DCI = mat4(
	0.73, 0.067, 0.03, 0.0,  //red channel
	0.30, 0.771, 0.0825, 0.0,  //green channel
	-0.105, 0.162, 0.855, 0.0,  //blue channel
	0.0,   0.0,   0.0,   0.97  //alpha channel
); 

const mat4 PSP_DCI_white = mat4(
	0.79, 0.067, 0.033, 0.0,  //red channel
	0.32, 0.771, 0.085, 0.0,  //green channel
	-0.11, 0.162, 0.882, 0.0,  //blue channel
	0.0,   0.0,   0.0,   0.9   //alpha channel
); 

const mat4 PSP_Rec2020 = mat4(
	0.57, 0.098, 0.03, 0.0,  //red channel
	0.38, 0.742, 0.0975, 0.0,  //green channel
	0.0, 0.16, 0.855,  0.0,  //blue channel
	0.0,   0.0,  0.0,   1.0  //alpha channel
); 

const mat4 PSP_Rec2020_white = mat4(
	0.60, 0.098, 0.03, 0.0,  //red channel
	0.40, 0.742, 0.099, 0.0,  //green channel
	0.0, 0.16, 0.871,  0.0,  //blue channel
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

	if (color_mode == 1) profile = (!white) ? PSP_sRGB : PSP_sRGB_white;
	else if (color_mode == 2) profile = (!white) ? PSP_DCI : PSP_DCI_white;
	else if (color_mode == 3) profile = (!white) ? PSP_Rec2020 : PSP_Rec2020_white;
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