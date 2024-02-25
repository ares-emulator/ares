#version 450

// Gendither
// 
// 	Copyright (C) 2013-2014 leilei
//  adapted for slang format by hunterk
// 
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// This table is a lazy jailbar pattern
int erroredtable[16] = {
	0,1,0,1,   
	16,15,16,15,
	0,1,0,1,   
	16,15,16,15
};

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
	vec3 final = texture(Source, vTexCoord).rgb;	
	vec2 ditheu = vTexCoord.xy * params.SourceSize.xy;

	// Dither
	int ditdex = 	int(mod(ditheu.x, 4.0)) * 4 + int(mod(ditheu.y, 4.0)); // 4x4!
	ivec3 color;
	ivec3 colord;
	color.r = int(final.r) * 224;
	color.g = int(final.g) * 224;
	color.b = int(final.b) * 224;
	int yeh = 0;
	int ohyes = 0;


	// looping through a lookup table matrix
	for (yeh=ditdex; yeh<(ditdex+16); yeh++) ohyes = erroredtable[yeh-15];

	colord.r = color.r + ohyes;
	colord.g = color.g + ohyes;
	colord.b = color.b + ohyes;
	final.rgb += colord.rgb * 0.003921568627451; // divide by 255, i don't trust em

	// Reduce color depth
	float why = 1.0;
	vec3 reduceme = vec3(1.0);
	float radooct = 4.4;	// 32 is usually the proper value // 4.4 was eyeballed

	reduceme.r = pow(final.r, why);  
	reduceme.r *= radooct;	
	reduceme.r = int(floor(reduceme.r));
	reduceme.r /= radooct; 
	reduceme.r = pow(reduceme.r, why);

	reduceme.g = pow(final.g, why);  
	reduceme.g *= radooct;		
	reduceme.g = int(floor(reduceme.g));	
	reduceme.g /= radooct;	
	reduceme.g = pow(reduceme.g, why);

	reduceme.b = pow(final.b, why);  
	reduceme.b *= radooct;	
	reduceme.b = int(floor(reduceme.b));	
	reduceme.b /= radooct;	
	reduceme.b = pow(reduceme.b, why);

	// Brightness cap
	reduceme.rgb = clamp(reduceme.rgb, vec3(0.0), vec3(0.875));
	
   FragColor = vec4(reduceme.rgb, 1.0);
}