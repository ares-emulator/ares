#version 450

/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////
//
//  autocrop-koko
//  Copyright (C) 2024 Antonio Orefice <kokoko3k@gmail.com>
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 3 of the License, or any later version.
//
////////////////////////////////////////////////////////////////////////////////

#include "autocrop_config.inc"

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

void main() {
	vec2 co = vTexCoord;
	if (AUTOCROP_MAX > 0.0)
		co = clamp(co,  params.OriginalSize.zw * AUTOCROP_MIN, 
		              1-params.OriginalSize.zw * AUTOCROP_MIN );

	FragColor = vec4(texture(Source, co).rgb, 1.0);
}