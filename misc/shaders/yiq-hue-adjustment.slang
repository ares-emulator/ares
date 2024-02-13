#version 450

/* 
	YIQ Hue Adjustment Shader
	Ported from https://github.com/AnalyticalGraphicsInc/cesium/blob/master/Source/Shaders/Builtin/Functions/hue.glsl
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float hue_degrees;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter hue_degrees "Hue" 0.0 -360.0 360.0 1.0
#define M_PI 3.1415926535897932384626433832795

#include "../../include/colorspace-tools.h"

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
   vec3 rgb = texture(Source, vTexCoord).rgb;
   vec3 yiq = RGBtoYIQ(rgb);
   float hue_radians = params.hue_degrees * (M_PI / 180.0);
   float hue = atan(yiq.z, yiq.y) + hue_radians;
   float chroma = sqrt(yiq.z * yiq.z + yiq.y * yiq.y);
   
   vec3 color = vec3(yiq.x, chroma * cos(hue), chroma * sin(hue));
   FragColor = vec4(YIQtoRGB(color).rgb, 1.0);
}
