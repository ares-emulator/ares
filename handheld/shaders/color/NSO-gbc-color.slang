#version 450

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/
// Shader that replicates the Nintendo Switch Online's GBC color filter --
// (Experimental WIP shader. Due to how complex the NSO GBC filter used (using different gamma ramps and greyscale temperatures), it couldn't be replicated to a shader as of now completely, so a side effect of using correct color metrics for RGB, is warm greyscales. This is not an issue with NSO GBA Filter which was easy to replicate. If you want an actual NSO GBC replication, use Reshade NSO-GBC shader pack instead that uses Reshade LUT shader)

layout(push_constant) uniform Push
{
   float lighten_screen;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	float mode;
} global;

#pragma parameter mode "Color Profile (1=sRGB, 2=DCI, 3=Rec2020)" 1.0 1.0 3.0 1.0
int color_mode = int(global.mode);

#pragma parameter lighten_screen "Lighten Screen" 1.0 0.0 1.0 0.05

#define target_gamma 1.0
#define display_gamma 1.0

/*
We'll define our color weights in this pattern:
	r,   rg,  rb,  0.0,  //red channel
	gr,  g,   gb,  0.0,  //green channel
	br,  bg,  b,   0.0,  //blue channel
	blr, blg, blb, lum   //alpha channel; we'll hide lum at the end, too
*/

const mat4 GBC_sRGB = mat4(
	0.84, 0.105, 0.15, 0.0,  //red channel
	0.265, 0.67, 0.30, 0.0,  //green channel
	0.0, 0.24, 0.525, 0.0,  //blue channel
	0.175,  0.18,  0.18,  0.85   //alpha channel
); 

const mat4 GBC_DCI = mat4(
	0.69, 0.17, 0.195, 0.0,  //red channel
	0.31, 0.61, 0.165, 0.0,  //green channel
	0.0, 0.22, 0.64, 0.0,  //blue channel
	0.0,   0.0,   0.0,   1.0  //alpha channel
); 

const mat4 GBC_Rec2020 = mat4(
	0.57, 0.195, 0.195, 0.0,  //red channel
	0.37, 0.605, 0.175, 0.0,  //green channel
	0.06, 0.22, 0.63,  0.0,  //blue channel
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

	if (color_mode == 1) profile = GBC_sRGB;
	else if (color_mode == 2) profile = GBC_DCI;
	else if (color_mode == 3) profile = GBC_Rec2020;
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
	vec4 screen = pow(texture(Source, vTexCoord), vec4(target_gamma + (params.lighten_screen * -1.0))).rgba;

	screen = clamp(screen * lum, 0.0, 1.0);
	screen = profile * screen;
	FragColor = pow(screen, vec4(1.0 / display_gamma));
}