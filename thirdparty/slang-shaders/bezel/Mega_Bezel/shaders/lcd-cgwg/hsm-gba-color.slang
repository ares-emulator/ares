#version 450

layout(push_constant) uniform Push
{
   float darken_screen;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
} global;

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/
// Shader that replicates the LCD dynamics from a GameBoy Advance

// #pragma parameter darken_screen "Darken Screen" 0.5 -0.25 1.5 0.05

#define darken_screen 0
#define target_gamma 2.2
#define display_gamma 2.2
#define sat 1.0
#define lum 1.0
#define contrast 1.0
#define blr 0.0
#define blg 0.0
#define blb 0.0
#define r 0.86
#define g 0.66
#define b 0.81
#define rg 0.11
#define rb 0.1325
#define gr 0.19
#define gb 0.0575
#define br -0.05
#define bg 0.23
#define overscan_percent_x 0.0
#define overscan_percent_y 0.0

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
   vec4 screen = pow(texture(Source, vTexCoord), vec4(target_gamma + darken_screen)).rgba;
   vec4 avglum = vec4(0.5);
   screen = mix(screen, avglum, (1.0 - contrast));
   
 //				r   g    b   black
mat4 color = mat4(r,  rg,  rb, 0.0,  //red channel
			   gr,  g,   gb, 0.0,  //green channel
			   br,  bg,  b,  0.0,  //blue channel
			  blr, blg, blb,    0.0); //alpha channel; these numbers do nothing for our purposes.
			  
mat4 adjust = mat4((1.0 - sat) * 0.3086 + sat, (1.0 - sat) * 0.3086, (1.0 - sat) * 0.3086, 1.0,
(1.0 - sat) * 0.6094, (1.0 - sat) * 0.6094 + sat, (1.0 - sat) * 0.6094, 1.0,
(1.0 - sat) * 0.0820, (1.0 - sat) * 0.0820, (1.0 - sat) * 0.0820 + sat, 1.0,
0.0, 0.0, 0.0, 1.0);
	color *= adjust;
	screen = clamp(screen * lum, 0.0, 1.0);
	screen = color * screen;
	FragColor = pow(screen, vec4(1.0 / (display_gamma)));
}