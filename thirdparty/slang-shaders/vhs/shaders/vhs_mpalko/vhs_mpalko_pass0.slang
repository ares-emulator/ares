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


//////////// -- SET UP LUTS HERE -- ////////////

#define iChannel0 Source

//////////////// -- END PASSES AND LUTS -- ////////////////

//////////// -- SET UP SHADERTOY VARIABLES -- ////////////

float iGlobalTime = float(params.FrameCount)*0.025;
float iTime = float(params.FrameCount)*0.025;
vec2 iResolution = params.OutputSize.xy;
#define FragCoord (params.OutputSize.xy * vTexCoord.xy)
int iFrame = int(params.FrameCount);

//////////// -- END SHADERTOY VARIABLES -- ////////////

//////////// -- PASTE SHADERTOY HERE -- ////////////

// Downsample buffer A and convert to YIQ color space


vec3 downsampleVideo(vec2 uv, vec2 pixelSize, ivec2 samples)
{
    //return texture(VIDEO_TEXTURE, uv).rgb * rgb2yiq;
    
    vec2 uvStart = uv - pixelSize / 2.0;
    vec2 uvEnd = uv + pixelSize;
    
    vec3 result = vec3(0.0, 0.0, 0.0);
    for (int i_u = 0; i_u < samples.x; i_u++)
    {
        float u = lerp(uvStart.x, uvEnd.x, float(i_u) / float(samples.x));
        
        for (int i_v = 0; i_v < samples.y; i_v++)
        {
            float v = lerp(uvStart.y, uvEnd.y, float(i_v) / float(samples.y));
            
            result += texture(VIDEO_TEXTURE, vec2(u, v)).rgb;
        }
    }    
    
    return (result / float(samples.x * samples.y)) * rgb2yiq;
}

vec3 downsampleVideo(vec2 fragCoord, vec2 downsampledRes)
{
   
    if (fragCoord.x > downsampledRes.x || fragCoord.y > downsampledRes.y)
    {
        return vec3(0.0);
    }
    
    vec2 uv = fragCoord / downsampledRes;
    vec2 pixelSize = 1.0 / downsampledRes;
    ivec2 samples = ivec2(8, 3);
    
    pixelSize *= 1.0 + blurAmount; // Slight box blur to avoid aliasing
    
    return downsampleVideo(uv, pixelSize, samples);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    vec2 resLuminance = min(maxResLuminance, vec2(iResolution));
    vec2 resChroma = min(maxResChroma, vec2(iResolution));    
    
    float luminance = downsampleVideo(fragCoord, resLuminance).r;
    vec2 chroma = downsampleVideo(fragCoord, resChroma).gb;
    
    fragColor = vec4(luminance, chroma, 1);
}

//////////// -- END SHADERTOY -- ////////////

void main()
{
   mainImage(FragColor, FragCoord.xy);
}