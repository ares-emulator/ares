#version 450

/*
	simpletex_lcd_720p+gbc-color - a simple, textured LCD shader intended for non-backlit systems.
	Designed for use at 720p, without integer scaling. Includes GBC colour correction.
	
	- Makes use of grid effect from lcd3x
	  [original lcd3x code written by Gigaherz and released into the public domain]
	
	- Colour correction code taken from 'gbc-color', written by hunterk and realeased
	  into the public domain, with further tweaks by Pokefan531
	
	Other code by jdgleaver
	
	Usage notes:
	
	- Background texture size is hard-coded (I can't find a way to get this
	  automatically...). User must ensure that 'BG_TEXTURE_SIZE' define is
	  set appropriately.
	
	- Adjustable parameters:
	
	  > GRID_INTENSITY: Sets overall visibility of grid effect
	                    - 1.0: Grid is shown
	                    - 0.0: Grid is invisible (same colour as pixels)
	  > GRID_WIDTH: Sets effective with of grid lines
	                - 1.0: Maximum width
	                - 0.0: Minimum width
	                       (Note - this is kind of a hack: changing the width
	                        also changes the grid intensity, but we have to do
	                        it like this otherwise the grid is uneven without
	                        integer scaling enabled...)
	  > GRID_BIAS: Dynamically adjusts the grid intensity based on pixel luminosity
	               - 0.0: Grid intensity is uniform
	               - 1.0: Grid intensity scales linearly with pixel luminosity
	                      > i.e. the darker the pixel, the less the grid effect
	                        is apparent - black pixels exhibit no grid effect at all
	  > DARKEN_GRID: Darkens grid (duh...)
	                 - 0.0: Grid is white
	                 - 1.0: Grid is black
	  > DARKEN_COLOUR: Simply darkens pixel colours (effectively lowers gamma level of pixels)
	                   - 0.0: Colours are normal
	                   - 2.0: Colours are too dark...
	  > BACKGROUND_INTENSITY: Adjusts how prominent the paper background texture is
	                          - 0.0: No background texture
	                          - 1.0: Background texture is clearly visible
	
	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.
*/

// Background texture size
// > 2048 x 2048 textures are suitable for screen resolutions up to
//   1200p (or 1440p if running 'square' aspect ratio systems)
#define BG_TEXTURE_SIZE 2048.0
// > 4096 x 4096 textures are suitable for screen resolutions up to 4k
//#define BG_TEXTURE_SIZE 4096.0

#pragma parameter GRID_INTENSITY "Grid Intensity" 0.9 0.0 1.0 0.01
#pragma parameter GRID_WIDTH "Grid Width" 0.9 0.0 1.0 0.01
#pragma parameter GRID_BIAS "Grid Bias" 0.5 0.0 1.0 0.01
#pragma parameter DARKEN_GRID "Darken Grid" 0.0 0.0 1.0 0.01
#pragma parameter DARKEN_COLOUR "Darken Colours" 0.0 0.0 2.0 0.01
#pragma parameter BACKGROUND_INTENSITY "Background Intensity" 1.0 0.0 1.0 0.05

layout(push_constant) uniform Push
{
	float GRID_INTENSITY;
	float GRID_WIDTH;
	float GRID_BIAS;
	float DARKEN_GRID;
	float DARKEN_COLOUR;
	float BACKGROUND_INTENSITY;
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/*
    VERTEX_SHADER
*/
void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord   = TexCoord;
}

#pragma stage fragment

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D BACKGROUND;

// ### Magic Numbers...

// Grid parameters
#define PI 3.141592654
#define WIDTH_FACTOR_MAX 31.0

// RGB -> Luminosity conversion
// > Photometric/digital ITU BT.709
#define LUMA_R 0.2126
#define LUMA_G 0.7152
#define LUMA_B 0.0722
// > Digital ITU BT.601
//#define LUMA_R 0.299
//#define LUMA_G 0.587
//#define LUMA_B 0.114

// Background texture size
const float INV_BG_TEXTURE_SIZE = 1.0 / BG_TEXTURE_SIZE;

// Colour correction
#define TARGET_GAMMA 2.2
const float INV_DISPLAY_GAMMA = 1.0 / 2.2;
#define CC_LUM 0.94
#define CC_R 0.82
#define CC_G 0.665
#define CC_B 0.73
#define CC_RG 0.125
#define CC_RB 0.195
#define CC_GR 0.24
#define CC_GB 0.075
#define CC_BR -0.06
#define CC_BG 0.21

/*
    FRAGMENT SHADER
*/
void main()
{
	// Get current texture coordinate
	vec2 imgPixelCoord = vTexCoord.xy * registers.SourceSize.xy;
	vec2 imgCenterCoord = floor(imgPixelCoord.xy) + vec2(0.5, 0.5);
	
	// Get colour of current pixel
	vec3 colour = texture(Source, registers.SourceSize.zw * imgCenterCoord.xy).rgb;
	
	// Darken colours (if required...) and apply colour correction
	colour.rgb = pow(colour.rgb, vec3(TARGET_GAMMA - 0.5 + registers.DARKEN_COLOUR));
	colour.rgb = mat3(CC_R,  CC_RG, CC_RB,
							CC_GR, CC_G,  CC_GB,
							CC_BR, CC_BG, CC_B) * (colour.rgb * CC_LUM);
	colour.rgb = clamp(pow(colour.rgb, vec3(INV_DISPLAY_GAMMA)), 0.0, 1.0);
	
	// Generate grid pattern...
	// > Note the 0.25 pixel offset -> required to ensure that
	//   grid lines occur *between* pixels
	vec2 angle = 2.0 * PI * (imgPixelCoord - 0.25);
	
	float wfactor = 1.0 + (WIDTH_FACTOR_MAX - (registers.GRID_WIDTH * WIDTH_FACTOR_MAX));
	float yfactor = (wfactor + sin(angle.y)) / (wfactor + 1.0);
	float xfactor = (wfactor + sin(angle.x)) / (wfactor + 1.0);
	
	float lineWeight = 1.0 - (yfactor * xfactor);
	
	// > Apply grid adjustments (phase 1)
	//   - GRID_INTENSITY:
	//        1.0: Grid lines are white
	//        0.0: Grid lines are invisible
	lineWeight = lineWeight * registers.GRID_INTENSITY;
	
	// > Apply grid adjustments (phase 2)
	//   - GRID_BIAS:
	//        0.0: Use 'unbiased' lineWeight value calculated above
	//        1.0: Scale lineWeight by current pixel luminosity
	//             > i.e. the darker the pixel, the lower the intensity of the grid
	float luma = (LUMA_R * colour.r) + (LUMA_G * colour.g) + (LUMA_B * colour.b);
	lineWeight = lineWeight * (luma + ((1.0 - luma) * (1.0 - registers.GRID_BIAS)));
	
	// Apply grid pattern
	// (lineWeight == 1 -> set colour to value specified by DARKEN_GRID)
	colour.rgb = mix(colour.rgb, vec3(1.0 - registers.DARKEN_GRID), lineWeight);
	
	// Get background sample point
	// > NB: external texture coordinates are referenced in a completely different fashion
	//   here than they are in GLSL shaders...
	vec2 bgPixelCoord = floor(vTexCoord.xy * registers.OutputSize.xy) + vec2(0.5, 0.5);
	
	// Sample background texture and 'colourise' according to current pixel colour
	// (NB: the 'colourisation' here is lame, but the proper method is slow...)
	vec3 bgTexture = texture(BACKGROUND, bgPixelCoord.xy * INV_BG_TEXTURE_SIZE).rgb * colour.rgb;
	
	// Blend current pixel with background according to luminosity
	// (lighter colour == more transparent, more visible background)
	// Note: Have to calculate luminosity a second time... tiresome, but
	// it's not a particulary expensive operation...
	luma = (LUMA_R * colour.r) + (LUMA_G * colour.g) + (LUMA_B * colour.b);
	colour.rgb = mix(colour.rgb, bgTexture.rgb, luma * registers.BACKGROUND_INTENSITY);
	
	FragColor = vec4(colour.rgb, 1.0);
}
