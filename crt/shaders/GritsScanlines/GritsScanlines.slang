#version 450

// GritsScanlines by torridgristle
// license: public domain (https://forums.libretro.com/t/lightweight-lut-based-scanline-glow-concept-prototype-glsl/18336/7)

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float ScanlinesOpacity;
   float GammaCorrection;
} params;

#pragma parameter ScanlinesOpacity "Scanline Opacity" 0.9 0.0 1.0 0.05
#pragma parameter GammaCorrection "Gamma Correction" 1.2 0.5 2.0 0.1

#define ScanlinesOpacity params.ScanlinesOpacity
#define GammaCorrection params.GammaCorrection

//#define LuminanceDawnbringer
#define LuminanceLUT
//#define TrinitronColors

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
layout(set = 0, binding = 3) uniform sampler2D luminance_LUT;
layout(set = 0, binding = 4) uniform sampler2D color_LUT;
layout(set = 0, binding = 5) uniform sampler2D scanlines_LUT;

#ifdef LuminanceLUT

#define LUT_SizeLum 16.0

	// Code taken from RetroArch's LUT shader
float luminancelut(vec4 org)
{
	vec4 imgColorLum = org;
	float redLum = ( imgColorLum.r * (LUT_SizeLum - 1.0) + 0.4999 ) / (LUT_SizeLum * LUT_SizeLum);
	float greenLum = ( imgColorLum.g * (LUT_SizeLum - 1.0) + 0.4999 ) / LUT_SizeLum;
	float blue1Lum = (floor( imgColorLum.b  * (LUT_SizeLum - 1.0) ) / LUT_SizeLum) + redLum;
	float blue2Lum = (ceil( imgColorLum.b  * (LUT_SizeLum - 1.0) ) / LUT_SizeLum) + redLum;
	float mixerLum = clamp(max((imgColorLum.b - blue1Lum) / (blue2Lum - blue1Lum), 0.0), 0.0, 32.0);
	float color1Lum = texture(luminance_LUT, vec2( blue1Lum, greenLum )).x;
	float color2Lum = texture(luminance_LUT, vec2( blue2Lum, greenLum )).x;
	return mix(color1Lum, color2Lum, mixerLum);
}
#endif

#ifdef TrinitronColors

#define LUT_SizeTrinitron 32.0

vec4 TrinitronD50(vec4 org)
{
	vec4 imgColorTrinitron = org;
	float redTrinitron = ( imgColorTrinitron.r * (LUT_SizeTrinitron - 1.0) + 0.4999 ) / (LUT_SizeTrinitron * LUT_SizeTrinitron);
	float greenTrinitron = ( imgColorTrinitron.g * (LUT_SizeTrinitron - 1.0) + 0.4999 ) / LUT_SizeTrinitron;
	float blue1Trinitron = (floor( imgColorTrinitron.b  * (LUT_SizeTrinitron - 1.0) ) / LUT_SizeTrinitron) + redTrinitron;
	float blue2Trinitron = (ceil( imgColorTrinitron.b  * (LUT_SizeTrinitron - 1.0) ) / LUT_SizeTrinitron) + redTrinitron;
	float mixerTrinitron = clamp(max((imgColorTrinitron.b - blue1Trinitron) / (blue2Trinitron - blue1Trinitron), 0.0), 0.0, 32.0);
	vec4 color1Trinitron = texture(color_LUT, vec2( blue1Trinitron, greenTrinitron ));
	vec4 color2Trinitron = texture(color_LUT, vec2( blue2Trinitron, greenTrinitron ));
	vec4 fragColorTrinitron = mix(color1Trinitron, color2Trinitron, mixerTrinitron);
	return vec4(pow(fragColorTrinitron.rgb,vec3(GammaCorrection,GammaCorrection,GammaCorrection)),1.0);
} 
#endif

void main()
{
//Source Image
	vec4 org = texture(Source, vTexCoord);
   
#ifdef LuminanceLUT
// Use a 3DLUT instead of an equation so that it can use any arbitrary mess you can come up with.
	float luminance = luminancelut(org);
#elif defined LuminanceDawnbringer
// Dawnbringer's brightness equation from Dawnbringer's Toolbox scripts for Grafx2
	float luminance = sqrt(org.r*org.r*0.0676 + org.g*org.g*0.3025 + org.b*org.b*0.0361) * 1.5690256395005606;
#else
// Plain, standard, fine; slightly faster
	float luminance = ((0.299*org.r) + (0.587*org.g) + (0.114*org.b));
#endif

// Don't let it exceed 1.0
	luminance = clamp(luminance, 0.0, 1.0);

// Scanline Mapping, based on the Phosphor LUT shader's method of tiling a texture over the screen
	vec2 LUTeffectiveCoord = vec2(luminance,fract(vTexCoord.y*params.SourceSize.y));

// Scanline Layer
	vec4 screen = texture(scanlines_LUT, LUTeffectiveCoord);

// Output multiplying the scanlines into the original image, with control over opacity
#ifdef TrinitronColors
	org = TrinitronD50(org);
#endif
	FragColor = ((screen*ScanlinesOpacity)+(1.0 - ScanlinesOpacity)) * (org);
}