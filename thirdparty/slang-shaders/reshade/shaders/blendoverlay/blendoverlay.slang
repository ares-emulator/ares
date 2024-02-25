#version 450

// blendoverlay
// based on:
// https://github.com/jamieowen/glsl-blend for blendOverlay

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float OverlayMix;
	float LUTWidth;
	float LUTHeight;
} params;

#pragma parameter OverlayMix "Overlay Mix" 1.0 0.0 1.0 0.05
#pragma parameter LUTWidth "LUT Width" 6.0 1.0 1920.0 1.0
#pragma parameter LUTHeight "LUT Height" 4.0 1.0 1920.0 1.0

#define OverlayMix params.OverlayMix
#define LUTWidth params.LUTWidth
#define LUTHeight params.LUTHeight

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
layout(set = 0, binding = 3) uniform sampler2D overlay;

float blendOverlay(float base, float blend) {
	return base<0.5?(2.0*base*blend):(1.0-2.0*(1.0-base)*(1.0-blend));
}

void main()
{
    vec3  Picture    = texture(Source, vTexCoord).xyz;

    float Luminance    = 0.299*Picture.x + 0.587*Picture.y + 0.114*Picture.z;

    vec2  LutCoord   = vec2(fract(vTexCoord.x*params.OutputSize.x/LUTWidth),fract(vTexCoord.y*params.OutputSize.y/LUTHeight));

    vec3  ShadowMask = texture(overlay, LutCoord).xyz;
    
    vec3 ImageFinal  = Picture;
    
    ImageFinal.r = blendOverlay(ImageFinal.r,ShadowMask.r);
    ImageFinal.g = blendOverlay(ImageFinal.g,ShadowMask.g);
    ImageFinal.b = blendOverlay(ImageFinal.b,ShadowMask.b);
    
    ImageFinal   = mix(Picture,clamp(ImageFinal,0.0,1.0),OverlayMix);
    
    FragColor = vec4(ImageFinal,1.0);
}
