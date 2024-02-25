#version 450

// Implementation based on the article "Efficient Gaussian blur with linear sampling"
// http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
/* A version for MasterEffect Reborn, a standalone version, and a custom shader version for SweetFX can be 
   found at http://reshade.me/forum/shader-presentation/27-gaussian-blur-bloom-unsharpmask */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float GaussStrength;
	float BloomStrength;
	float SW;
	float BRIGHT;
} params;

 /*-----------------------------------------------------------.
/                  Gaussian Blur settings                     /
'-----------------------------------------------------------*/

#pragma parameter GaussStrength "Gauss Strength" 0.30 0.0 1.0 0.01
#define GaussStrength params.GaussStrength
#pragma parameter BloomStrength "Bloom Strength" 0.33 0.000 1.000 0.005
#define BloomStrength params.BloomStrength
#pragma parameter SW             "Haze Width"     2.0 1.0 4.0 0.25
#define SW params.SW
#pragma parameter BRIGHT         "Brightness adjust" 0.5 0.3 0.6 0.01
#define BRIGHT params.BRIGHT

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

//GaussEffect
//0 = off, 1 = Blur, 2 = Unsharpmask (expensive), 3 = Bloom, 4 = Sketchy, 5 = effects image only.
#define GaussEffect 1

//GaussStrength
//[0.00 to 1.00]   Amount of effect blended into the final image.
//#define GaussStrength 0.45

//addBloom
//Set to 1 to add bloom to Blur or Unsharpmask. 0 = off. Set GaussEffect to 0 for bloom only.
#define addBloom 1
#define bloomPass 1
#define USE_addBlur 0

//Bloom Strength
//[0.00 to 1.00] Amount of addBloom added to the final image.
//#define BloomStrength 0.33

//Bloom Intensity *also affects blur and unsharpmask*
//Makes bright spots brighter. Only works when BrightPass is set to 1.
//#define BloomIntensity 3.00

//GaussBloomWarmth
//0 = neutral, 1 = warm, 2 = hazy/foggy 
#define GaussBloomWarmth 0

//SW is Slant. Higher numbers = wider bloom.
//#define SW 2.00

#define CoefLuma_G            vec3(0.2126, 0.7152, 0.0722)      // BT.709 & sRBG luma coefficient (Monitors and HD Television)
#define sharp_strength_luma_G (CoefLuma_G * GaussStrength + 0.2)
#define sharp_clampG        0.035

#define texCoordp vTexCoord

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
layout(set = 0, binding = 3) uniform sampler2D scanpass;

void main()
{
	vec2 texp = texCoordp;
	vec2 texcoord  = vTexCoord;
	vec2 PIXEL_SIZE = params.SourceSize.zw;	

	float sampleOffsets[5] = { 0.0, 1.4347826, 3.3478260, 5.2608695, 7.1739130 };
	float sampleWeights[5] = { 0.16818994, 0.27276957, 0.11690125, 0.024067905, 0.0021112196 };

	vec4 color = texture(Source, texcoord) * sampleWeights[0];
	for(int i = 1; i < 5; ++i) {
		color += texture(Source, texcoord + vec2(sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += texture(Source, texcoord - vec2(sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += texture(Source, texcoord + vec2(-sampleOffsets[i]*SW * PIXEL_SIZE.x, sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
		color += texture(Source, texcoord + vec2(sampleOffsets[i]*SW * PIXEL_SIZE.x, -sampleOffsets[i] * PIXEL_SIZE.y)) * sampleWeights[i];
	}  
	vec4 blur = BRIGHT*color;
	
	vec4 orig = texture(scanpass, texp); 	 	
	
	vec3 sharp;
	#if (GaussEffect == 0)
		orig = orig;
	#elif (GaussEffect == 1)
		// Blur...
		orig = mix(orig, blur, GaussStrength);
	#elif (GaussEffect == 2)
		// Sharpening
		sharp = orig.rgb - blur.rgb;
		float sharp_luma = dot(sharp, sharp_strength_luma_G);
		sharp_luma = clamp(sharp_luma, -sharp_clampG, sharp_clampG);
		orig = orig + sharp_luma;
	#elif (GaussEffect == 3)
		// Bloom
		#if (GaussBloomWarmth == 0)
			orig = mix(orig, blur *4, GaussStrength);                                     
// Neutral
		#elif (GaussBloomWarmth == 1)
			orig = mix(orig, max(orig *1.8 + (blur *5) - 1.0, 0.0), GaussStrength);       // Warm and cheap
		#else
			orig = mix(orig, (1.0 - ((1.0 - orig) * (1.0 - blur *1.0))), GaussStrength);  // Foggy bloom
		#endif
	#elif (GaussEffect == 4)
		// Sketchy
		sharp = orig.rgb - blur.rgb;		
		orig = vec4(1.0, 1.0, 1.0, 0.0) - min(orig, dot(sharp, sharp_strength_luma_G)) *3;
		// orig = vec4(1.0, 1.0, 1.0, 0.0) - min(blur, orig);      // Negative
	#else
		orig = blur;
	#endif

	#if (addBloom == 1)
		#if (GaussBloomWarmth == 0)
			orig += mix(orig, blur *4, BloomStrength);
			orig = orig * 0.5;
		#elif (GaussBloomWarmth == 1)
			orig += mix(orig, max(orig *1.8 + (blur *5) - 1.0, 0.0), BloomStrength);
			orig = orig * 0.5;
		#else
			orig += mix(orig, (1.0 - ((1.0 - orig) * (1.0 - blur *1.0))), BloomStrength);
			orig = orig * 0.5;
		#endif
	#else
		orig = orig;
	#endif
	
	#if (USE_addBlur == 1)
		orig += mix(orig, blur2, BlurStrength);
		//orig = blur2;
	#endif	
   FragColor = orig;
}