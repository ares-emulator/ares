#version 450

// based on VHS Compression
// shadertoy by mpalko https://www.shadertoy.com/view/tsfXWj

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

#include "vhs_mpalko.inc"

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

//////////// -- SET UP PASSES AND LUTS HERE -- ////////////

#define iChannel1 Source

//////////////// -- END PASSES AND LUTS -- ////////////////

float iGlobalTime = float(params.FrameCount)*0.025;
float iTime = float(params.FrameCount)*0.025;
vec2 iResolution = params.OutputSize.xy;
#define FragCoord (params.OutputSize.xy * vTexCoord.xy)

//////////// -- PASTE SHADERTOY HERE -- ////////////

vec2 rotate(vec2 v, float a)
{
	float s = sin(a);
	float c = cos(a);
	mat2 m = mat2(c, -s, s, c);
	return m * v;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    vec2 resLuminance = min(maxResLuminance, vec2(iResolution));
    vec2 resChroma = min(maxResChroma, vec2(iResolution));
    
    vec2 uvLuminance = uv * (resLuminance / vec2(iResolution));
    vec2 uvChroma = uv * (resChroma / vec2(iResolution));
    
    vec3 result;
    float luminance = textureBicubic(iChannel1, uvLuminance).x;
    vec2 chroma = textureBicubic(iChannel1, uvChroma).yz;
    result = vec3(luminance, chroma) * yiq2rgb;
    
    fragColor = vec4(result, 1);
}

//////////// -- END SHADERTOY -- ////////////

void main()
{
   mainImage(FragColor, FragCoord.xy);
}