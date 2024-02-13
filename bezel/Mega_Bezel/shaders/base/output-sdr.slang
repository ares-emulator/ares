#version 450

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

	FragColor = HSM_Delinearize(texture(Source, vTexCoord), DEFAULT_SRGB_GAMMA);

	// If we have calculated an image then set -1 as a flag to show that we have
	if (vTexCoord.x > (1 - 2 / global.OutputSize.x) && vTexCoord.y > ( 1 - 2 / global.OutputSize.y))
		FragColor.a = -1;
}