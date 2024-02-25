#version 450

/*
	PowerVR2 buffer shader

    Authors: leilei
 
    This program is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float INTERLACED, VGASIGNAL;
} params;

#pragma parameter INTERLACED "PVR - Interlace smoothing" 1.00 0.00 1.00 1.0
#pragma parameter VGASIGNAL "PVR - VGA signal loss" 0.00 0.00 1.00 1.0

#define INTERLACED params.INTERLACED
#define VGASIGNAL params.VGASIGNAL

#define HW 1.00

#define LUM_R (76.0f/255.0f)
#define LUM_G (150.0f/255.0f)
#define LUM_B (28.0f/255.0f)

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

float dithertable[16] = {
	16.,4.,13.,1.,   
	8.,12.,5.,9.,
	14.,2.,15.,3.,
	6.,10.,7.,11.		
};

void main()
{
	float blue;
	
	vec2 texcoord  = vTexCoord;
	vec2 texcoord2  = vTexCoord;
	texcoord2.x *= params.SourceSize.x;
	texcoord2.y *= params.SourceSize.y;
	vec4 color = texture(Source, texcoord);
	float fc = mod(float(params.FrameCount), 2.0);

	// Blend vertically for composite mode
	if (bool(INTERLACED))
	{
	int taps = int(3);

	float tap = 2.0f/taps;
	vec2 texcoord4  = vTexCoord;
	texcoord4.x = texcoord4.x;
	texcoord4.y = texcoord4.y + ((tap*(taps/2))/480.0f);
	vec4 blur1 = texture(Source, texcoord4);
	int bl;
	vec4 ble;
	
	ble.r = 0.00f;
	ble.g = 0.00f;
	ble.b = 0.00f;

	for (bl=0;bl<taps;bl++)
		{
			texcoord4.y += (tap / 480.0f);
			ble.rgb += texture(Source, texcoord4).rgb / taps;
		}

        	color.rgb = ( ble.rgb );
	}

	// Dither. ALWAYS do this for 16bpp
	int ditdex = 	int(mod(texcoord2.x, 4.0)) * 4 + int(mod(texcoord2.y, 4.0)); 	
	int yeh = 0;
	float ohyes;
	vec4 how;

	for (yeh=ditdex; yeh<(ditdex+16); yeh++) 	ohyes =  ((((dithertable[yeh-15]) - 1) * 0.1));
	color.rb -= (ohyes / 128);
	color.g -= (ohyes / 128);
	{
	vec4 reduct;		// 16 bits per pixel (5-6-5)
	reduct.r = 32;
	reduct.g = 64;	
	reduct.b = 32;
	how = color;
  	how = pow(how, vec4(1.0f, 1.0f, 1.0f, 1.0f));  	how *= reduct;  	how = floor(how);	how = how / reduct;  	how = pow(how, vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}

	color.rb = how.rb;
	color.g = how.g;

	// There's a bit of a precision drop involved in the RGB565ening for VGA
	// I'm not sure why that is. it's exhibited on PVR1 and PVR3 hardware too
	if (INTERLACED < 0.5)
	{
		if (mod(color.r*32, 2.0)>0) color.r -= 0.023;
		if (mod(color.g*64, 2.0)>0) color.g -= 0.01;
		if (mod(color.b*32, 2.0)>0) color.b -= 0.023;
	}


	// RGB565 clamp

	color.rb = round(color.rb * 32)/32;
	color.g = round(color.g * 64)/64;

	// VGA Signal Loss, which probably is very wrong but i tried my best
	if (bool(VGASIGNAL))
	{

	int taps = 32;
	float tap = 12.0f/taps;
	vec2 texcoord4  = vTexCoord;
	texcoord4.x = texcoord4.x + (2.0f/640.0f);
	texcoord4.y = texcoord4.y;
	vec4 blur1 = texture(Source, texcoord4);
	int bl;
	vec4 ble;
	for (bl=0;bl<taps;bl++)
		{
			float e = 1;
			if (bl>=3)
			e=0.35f;
			texcoord4.x -= (tap  / 640);
			ble.rgb += (texture(Source, texcoord4).rgb * e) / (taps/(bl+1));
		}

        	color.rgb += ble.rgb * 0.015;

		//color.rb += (4.0f/255.0f);
		color.g += (9.0f/255.0f);
	}

   	FragColor = vec4(color);
}
