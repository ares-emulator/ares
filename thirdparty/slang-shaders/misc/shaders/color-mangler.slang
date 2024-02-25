#version 450

layout(push_constant) uniform Push
{
	float gamma_boost_r;
	float gamma_boost_g;
	float gamma_boost_b;
	float sat;
	float lum;
	float cntrst;
	float r;
	float g;
	float b;
	float rg;
	float rb;
	float gr;
	float gb;
	float br;
	float bg;
	float blr;
	float blg;
	float blb;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
} global;

/*
   Color Mangler
   Author: hunterk
   License: Public domain
*/
#pragma parameter gamma_boost_r "Gamma Mod Red Channel" 0.0 -5.0 5.0 0.1
#pragma parameter gamma_boost_g "Gamma Mod Green Channel" 0.0 -5.0 5.0 0.1
#pragma parameter gamma_boost_b "Gamma Mod Blue Channel" 0.0 -5.0 5.0 0.1
#pragma parameter sat "Saturation" 1.0 0.0 3.0 0.01
#pragma parameter lum "Luminance" 1.0 0.0 5.0 0.01
#pragma parameter cntrst "Contrast" 1.0 0.0 2.0 0.01
#pragma parameter r "Red" 1.0 0.0 2.0 0.01
#pragma parameter g "Green" 1.0 0.0 2.0 0.01
#pragma parameter b "Blue" 1.0 0.0 2.0 0.01
#pragma parameter rg "Red-Green Tint" 0.0 0.0 1.0 0.005
#pragma parameter rb "Red-Blue Tint" 0.0 0.0 1.0 0.005
#pragma parameter gr "Green-Red Tint" 0.0 0.0 1.0 0.005
#pragma parameter gb "Green-Blue Tint" 0.0 0.0 1.0 0.005
#pragma parameter br "Blue-Red Tint" 0.0 0.0 1.0 0.005
#pragma parameter bg "Blue-Green Tint" 0.0 0.0 1.0 0.005
#pragma parameter blr "Black-Red Tint" 0.0 0.0 1.0 0.005
#pragma parameter blg "Black-Green Tint" 0.0 0.0 1.0 0.005
#pragma parameter blb "Black-Blue Tint" 0.0 0.0 1.0 0.005

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

void main()
{
   vec4 screen = pow(texture(Source, vTexCoord), vec4(2.2)).rgba;
   vec4 avglum = vec4(0.5);
   screen = mix(screen, avglum, (1.0 - params.cntrst));
   
 //				params.r   params.g    params.b   params.black
mat4 color = mat4(params.r,  params.rg,  params.rb, 0.0,  //red channel
			   params.gr,  params.g,   params.gb, 0.0,  //green channel
			   params.br,  params.bg,  params.b,  0.0,  //blue channel
			  params.blr, params.blg, params.blb,    0.0); //alpha channel; these numbers do nothing for our purposes.
			  
mat4 adjust = mat4((1.0 - params.sat) * 0.2126 + params.sat, (1.0 - params.sat) * 0.2126, (1.0 - params.sat) * 0.2126, 1.0,
(1.0 - params.sat) * 0.7152, (1.0 - params.sat) * 0.7152 + params.sat, (1.0 - params.sat) * 0.7152, 1.0,
(1.0 - params.sat) * 0.0722, (1.0 - params.sat) * 0.0722, (1.0 - params.sat) * 0.0722 + params.sat, 1.0,
0.0, 0.0, 0.0, 1.0);
	color *= adjust;
	screen = clamp(screen * params.lum, 0.0, 1.0);
	screen = color * screen;
	vec3 out_gamma = vec3(1.) / (vec3(2.2) - vec3(params.gamma_boost_r, params.gamma_boost_g, params.gamma_boost_b));
	FragColor = pow(screen, vec4(out_gamma, 1.0));
}