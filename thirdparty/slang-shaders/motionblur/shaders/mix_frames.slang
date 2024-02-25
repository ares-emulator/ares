#version 450

/*
	mix_frames - performs 50:50 blending between the current and previous
	frames.
	
	Author: jdgleaver
	
	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the Free
	Software Foundation; either version 2 of the License, or (at your option)
	any later version.
*/

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
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;

/*
    FRAGMENT SHADER
*/
void main()
{
	// Get colour of current pixel
	vec3 colour = texture(Source, vTexCoord.xy).rgb;
	
	// Get colour of previous pixel
	vec3 colourPrev = texture(OriginalHistory1, vTexCoord.xy).rgb;
	
	// Mix colours
	colour.rgb = mix(colour.rgb, colourPrev.rgb, 0.5);
	
	FragColor = vec4(colour.rgb, 1.0);
}
