#version 150

// GritsScanlines by torridgristle
// license: public domain (https://forums.libretro.com/t/lightweight-lut-based-scanline-glow-concept-prototype-glsl/18336/7)

/* Runtime parameters
#in LuminanceDawnbringer
#in LuminanceLUT
#in ScanlinesOpacity
#in GammaCorrection
#in TrinitronColors
*/

// static parameters
//#define LuminanceDawnbringer
#define LuminanceLUT
//#define TrinitronColors
#define ScanlinesOpacity 0.9
//#define GammaCorrection 1.2

uniform sampler2D source[];
uniform vec4 sourceSize[];
uniform vec4 targetSize;
uniform sampler2D pixmap[];

in Vertex {
  vec2 texCoord;
};

out vec4 fragColor;

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
	float color1Lum = texture(pixmap[1], vec2( blue1Lum, greenLum )).x;
	float color2Lum = texture(pixmap[1], vec2( blue2Lum, greenLum )).x;
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
	vec4 color1Trinitron = texture(pixmap[2], vec2( blue1Trinitron, greenTrinitron ));
	vec4 color2Trinitron = texture(pixmap[2], vec2( blue2Trinitron, greenTrinitron ));
	vec4 fragColorTrinitron = mix(color1Trinitron, color2Trinitron, mixerTrinitron);
	return vec4(pow(fragColorTrinitron.rgb,vec3(GammaCorrection,GammaCorrection,GammaCorrection)),1.0);
} 
#endif

void main() {

//Source Image
	vec4 org = texture(source[0], texCoord);
   
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
	vec2 LUTeffectiveCoord = vec2(luminance,fract((texCoord.y*targetSize.y)/4.0));

// Scanline Layer
	vec4 screen = texture(pixmap[0], LUTeffectiveCoord);

// Output multiplying the scanlines into the original image, with control over opacity
#ifdef TrinitronColors
	org = TrinitronD50(org);
#endif
	fragColor = ((screen*ScanlinesOpacity)+(1.0 - ScanlinesOpacity)) * (org);
}

