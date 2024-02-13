#version 450

/*
    HDR Output - All color work by MajorPainTheCactus

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see [http://www.gnu.org/licenses/].
*/

// Pass HDR color format
#pragma format A2B10G10R10_UNORM_PACK32

/////////////// IMPORTS ///////////////
#include "../megatron/include/parameters-hdr-color.h"
#include "../megatron/include/gamma_correct.h"
#include "../megatron/include/inverse_tonemap.h"

#include "../base/common/globals-and-screen-scale-params.inc"
#include "../base/common/common-functions.inc"
#include "../base/common/params-2-bezel.inc"
#include "../base/common/common-functions-bezel.inc"

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma stage vertex

layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;

layout(location = 6) out vec2 vTexCoord;

//////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma stage fragment

layout(location = 6) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;


layout(set = 0, binding = 1) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 2) uniform sampler2D Source;

layout(set = 0, binding = 3) uniform sampler2D OutputPassFeedback;
#define PassFeedback OutputPassFeedback

//////////////////////////////////////////////////////////////////////////////////////////////////
void main()
{
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
						TUBE_DIFFUSE_SCALE, 
						TUBE_SCALE, 
						TUBE_DIFFUSE_ASPECT, 
						false,
						BEZEL_OUTSIDE_SCALE,
						BEZEL_OUTSIDE_COORD, 
						BEZEL_OUTSIDE_CURVED_COORD, 
						FRAME_OUTSIDE_CURVED_COORD);


	if (HHLP_IsOutsideCoordSpace(BEZEL_OUTSIDE_COORD))
	{
		vec4 feedback_color_test = texture(PassFeedback, vec2(1, 1));
		if (HSM_CACHE_GRAPHICS_ON > 0.5 && feedback_color_test.a < 0 && !CACHE_INFO_CHANGED)
		{
			FragColor = texture(PassFeedback, vTexCoord);
			return;
		}
	}

	vec3 output_colour = vec3(0);
	LinearToOutputColor(texture(Source, vTexCoord).rgb, output_colour);
	FragColor = vec4(output_colour, 1.0f);

	// If we have calculated an image then set -1 as a flag to show that we have
	if (vTexCoord.x > (1 - 2 / global.OutputSize.x) && vTexCoord.y > ( 1 - 2 / global.OutputSize.y))
		FragColor.a = -1;
}