#version 450

/*
    zfast_lcd_standard - A very simple LCD shader meant to be used at 1080p
		on the raspberry pi 3.
		
    Copyright (C) 2017 Greg Hogan (SoltanGris42)
    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.
Notes:  This shader just does nearest neighbor scaling of the game and then
		darkens the border pixels to imitate an LCD screen. You can change the
		amount of darkening and the thickness of the borders.  You can also 
		do basic gamma adjustment.
		
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BORDERMULT, GBAGAMMA;
} params;

#pragma parameter BORDERMULT "Border Multiplier" 14.0 -40.0 40.0 1.0
#pragma parameter GBAGAMMA "GBA Gamma Hack" 1.0 0.0 1.0 1.0

//For testing compilation 
//#define FRAGMENT
//#define VERTEX
//#define GBAGAMMA 

//Some drivers don't return black with texture coordinates out of bounds
//SNES Classic is too slow to black these areas out when using fullscreen
//overlays.  But you can uncomment the below to black them out if necessary

#define BLACK_OUT_BORDER

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

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
	vec2 texcoordInPixels = vTexCoord.xy * params.SourceSize.xy;
	vec2 centerCoord = floor(texcoordInPixels.xy)+vec2(0.5,0.5);
	vec2 distFromCenter = abs(centerCoord - texcoordInPixels);

	float Y = max(distFromCenter.x,(distFromCenter.y));

	Y=Y*Y;
	float YY = Y*Y;
	float YYY = YY*Y;

	float LineWeight = YY - 2.7*YYY;
	LineWeight = 1.0 - params.BORDERMULT*LineWeight;

	vec3 colour = texture(Source, params.SourceSize.zw*centerCoord).rgb*LineWeight;

//#if defined(GBAGAMMA)
//	//colour.rgb = pow(colour.rgb, vec3(1.35));
//	colour.rgb*=0.6+0.4*(colour.rgb); //fake gamma because the pi is too slow!
//#endif
	if (params.GBAGAMMA > 0.5)
		colour.rgb*=0.6+0.4*(colour.rgb); //fake gamma because the pi is too slow!
		
	FragColor = vec4(colour.rgb , 1.0);
}
