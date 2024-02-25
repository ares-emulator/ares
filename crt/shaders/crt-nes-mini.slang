#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float BRIGHTBOOST;
	float INTENSITY;
	float SCANTHICK;
} params;

#pragma parameter SCANTHICK "Scanline Thickness" 2.0 2.0 4.0 2.0
#pragma parameter INTENSITY "Scanline Intensity" 0.15 0.0 1.0 0.01
#pragma parameter BRIGHTBOOST "Luminance Boost" 0.15 0.0 1.0 0.01

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

void main()
{
    vec3 texel = texture(Source, vTexCoord.xy).rgb;
    vec3 pixelHigh = ((1.0 + params.BRIGHTBOOST) - (0.2 * texel)) * texel;
    vec3 pixelLow  = ((1.0 - params.INTENSITY) + (0.1 * texel)) * texel;
    float selectY = mod(vTexCoord.y * params.SCANTHICK * params.SourceSize.y, 2.0);
    float selectHigh = step(1.0, selectY);
    float selectLow = 1.0 - selectHigh;
    vec3 pixelColor = (selectLow * pixelLow) + (selectHigh * pixelHigh);

    FragColor = vec4(pixelColor, 1.0);
}
