#version 450

/*
	simpletex_lcd - a simple, textured LCD shader intended for non-backlit systems
	
	- Makes use of the 'line weighting' equation from zfast_lcd_standard
	  [original zfast_lcd_standard code copyright (C) 2017 Greg Hogan (SoltanGris42)]
	
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
	                - 1.0: Normal full width
	                - 0.0: Minimum width
	                       (Note - this does not mean zero width! Instead, this
	                        is the minimum 'sane' width, below which the grid
	                        becomes pointless...)
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

#pragma parameter GRID_INTENSITY "Grid Intensity" 1.0 0.0 1.0 0.05
#pragma parameter GRID_WIDTH "Grid Width" 1.0 0.0 1.0 0.05
#pragma parameter GRID_BIAS "Grid Bias" 0.0 0.0 1.0 0.05
#pragma parameter DARKEN_GRID "Darken Grid" 0.0 0.0 1.0 0.05
#pragma parameter DARKEN_COLOUR "Darken Colours" 0.0 0.0 2.0 0.05
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

// Grid pattern
// > Line weighting equation:
//   y = a * (x^4 - b * x^6)
const float LINE_WEIGHT_A = 48.0;
const float LINE_WEIGHT_B = 8.0 / 3.0;

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
	
	// Darken colours (if required...)
	colour.rgb = pow(colour.rgb, vec3(1.0 + registers.DARKEN_COLOUR));
	
	// Generate grid pattern...
	vec2 distFromCenter = abs(imgCenterCoord.xy - imgPixelCoord.xy);
	
	float xSquared = max(distFromCenter.x, distFromCenter.y);
	xSquared = xSquared * xSquared;
	
	float xQuarted = xSquared * xSquared;
	
	// > Line weighting equation:
	//   y = 48 * (x^4 - (8/3) * x^6)
	float lineWeight = LINE_WEIGHT_A * (xQuarted - (LINE_WEIGHT_B * xQuarted * xSquared));
	
	// > Apply grid adjustments (phase 1)
	//   - GRID_WIDTH:
	//        1.0: Use raw lineWeight value
	//        0.0: Use lineWeight ^ 2 (makes lines thinner - realistically, this is
	//                                 the thinnest we can go before the grid effect
	//                                 becomes pointless, particularly with 'high resolution'
	//                                 systems like the GBA)
	//   - GRID_INTENSITY:
	//        1.0: Grid lines are white
	//        0.0: Grid lines are invisible
	lineWeight = lineWeight * (lineWeight + ((1.0 - lineWeight) * registers.GRID_WIDTH)) * registers.GRID_INTENSITY;
	
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
