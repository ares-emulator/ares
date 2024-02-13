#version 450

#include "../../base/common/globals-and-screen-scale-params.inc"
#include "../../base/common/params-2-bezel.inc"
#include "../../base/common/common-functions-bezel.inc"

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BLOOM_STRENGTH;
	float SOURCE_BOOST;
	float OUTPUT_GAMMA;
} params;

#pragma parameter BLOOM_STRENGTH 	"          Bloom Strength"  0.10 0.0 1.0 0.01
#pragma parameter SOURCE_BOOST 		"          Bloom Color Boost" 1.15 1.0 2.0 0.05
#pragma parameter OUTPUT_GAMMA 	"OUTPUT GAMMA"       2.2 1.0 3.0  0.1

#define BLOOM_STRENGTH params.BLOOM_STRENGTH
#define SOURCE_BOOST params.SOURCE_BOOST
#define OUTPUT_GAMMA params.OUTPUT_GAMMA

#define INV_OUTPUT_GAMMA (1.0 / OUTPUT_GAMMA)
#define saturate(c) clamp(c, 0.0, 1.0)

// layout(std140, set = 0, binding = 0) uniform UBO
// {
// 	mat4 MVP;
// } global;

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
layout(set = 0, binding = 3) uniform sampler2D CRT_PASS;

void main()
{
// 	// HSM Added
// 	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
// 	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

// 	vec2 cache_bounds_coord = SCREEN_COORD;

// // If it's the potato preset render the whole frame
// #ifndef IS_POTATO_PRESET
// #ifndef IS_NO_REFLECT_PRESET
// 	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
// 	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
// 						TUBE_DIFFUSE_SCALE, 
// 						TUBE_SCALE, 
// 						SCREEN_ASPECT, 
// 						false,
// 						BEZEL_OUTSIDE_SCALE,
// 						BEZEL_OUTSIDE_COORD, 
// 						BEZEL_OUTSIDE_CURVED_COORD, 
// 						FRAME_OUTSIDE_CURVED_COORD);
// 	cache_bounds_coord = (FRAME_OUTSIDE_CURVED_COORD - 0.5) * 0.9 + 0.5;
// #endif
// #endif

// 	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
// 	{
//         FragColor = vec4(0);
//         return;
// 	}

	vec3 source = SOURCE_BOOST * texture(CRT_PASS, vTexCoord).rgb;
	vec3 bloom = texture(Source, vTexCoord).rgb;
	source += BLOOM_STRENGTH * bloom;

	// HSM Removed
	FragColor = vec4(pow(saturate(source), vec3(INV_OUTPUT_GAMMA,INV_OUTPUT_GAMMA,INV_OUTPUT_GAMMA)), 1.0);

	// HSM add output gamma
	// FragColor = HSM_Delinearize(vec4(saturate(source), 1), HSM_GAMMA_OUT_CRT);
	// End Addition
}
