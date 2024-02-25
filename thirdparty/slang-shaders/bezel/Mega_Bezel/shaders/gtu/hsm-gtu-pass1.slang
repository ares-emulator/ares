#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	float GTU_ON;
	float compositeConnection;
} params;



layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

////////////////////////////////////////////////////////
// GTU version 0.50
// Author: aliaspider - aliaspider@gmail.com
// License: GPLv3
////////////////////////////////////////////////////////

#include "config.h"

#define RGB_to_YIQ 	mat3x3( 0.299 , 0.595716 , 0.211456 ,	0.587    , -0.274453 , -0.522591 ,		0.114    , -0.321263 , 0.311135 )

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
	vec4	c	=	texture(Source, vTexCoord);
	// If GTU is off return the color from the last pass
	if (params.GTU_ON < 1)
	{
		FragColor = c;
		return;
	}

	if(params.compositeConnection > 0.0)
		c.rgb	=	c.rgb * RGB_to_YIQ;
   FragColor = c;
}