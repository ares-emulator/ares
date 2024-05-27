#version 450

/*
	Bicubic 5-taps (Fast) - ported by Hyllian - 2024

	Samples a texture with B-Spline filtering, using only 4 texture fetches instead of 16.
	See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
	Source: https://www.shadertoy.com/view/styXDh
	http://www.profhua.com/Sub/Article/BicubicFiltering/BicubicFiltering.html

	ATENTION: This code only work using LINEAR filter sampling set on Retroarch!

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float B, C;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


#pragma parameter B "Bicubic Coeff B" 0.33 0.0 1.0 0.01
#pragma parameter C "Bicubic Coeff C" 0.33 0.0 1.0 0.01

#define B params.B
#define C params.C

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
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    vec2 texSize = params.SourceSize.xy;
    vec2 invTexSize = 1.0 / texSize;
    vec2 iTc = vTexCoord * texSize;

    vec2 tc = floor(iTc - vec2(0.5)) + vec2(0.5);

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the B-Spline function to get our filter weights.
    vec2 f  = iTc - tc;
    vec2 f2 = f * f;
    vec2 f3 = f2 * f;

    vec2 lf  = vec2(1.) + f;
    vec2 lf2 = lf * lf;
    vec2 lf3 = lf2 * lf;

    vec2 rf  = vec2(2.) - f;
    vec2 rf2 = rf * rf;
    vec2 rf3 = rf2 * rf;

    vec2 w0 = ((-B-6.*C)*lf3+(6.*B+30.*C)*lf2+(-12.*B-48.*C)*lf+(8.*B+24.*C))/6.0;
    vec2 w1 = ((12.-9.*B-6.*C)*f3+(-18.+12.*B+6.*C)*f2+(6.-2.*B))/6.0;
    vec2 w3 = ((-B-6.*C)*rf3+(6.*B+30.*C)*rf2+(-12.*B-48.*C)*rf+(8.*B+24.*C))/6.0;
    vec2 w2 = vec2(1.) - w0 - w1 - w3;

    vec2 Weight[3];
    vec2 Sample[3];

    Weight[0] = w0;
    Weight[1] = w1 + w2;
    Weight[2] = w3;

    Sample[0] = tc - vec2(1.);
    Sample[1] = tc + w2 / Weight[1];
    Sample[2] = tc + vec2(2.);

    Sample[0] *= invTexSize;
    Sample[1] *= invTexSize;
    Sample[2] *= invTexSize;

    float sampleWeight[5];
    sampleWeight[0] = Weight[1].x * Weight[0].y;
    sampleWeight[1] = Weight[0].x * Weight[1].y;
    sampleWeight[2] = Weight[1].x * Weight[1].y;
    sampleWeight[3] = Weight[2].x * Weight[1].y;
    sampleWeight[4] = Weight[1].x * Weight[2].y;

    vec3 Ct = texture(Source, vec2(Sample[1].x, Sample[0].y)).rgb * sampleWeight[0];    
    vec3 Cl = texture(Source, vec2(Sample[0].x, Sample[1].y)).rgb * sampleWeight[1];    
    vec3 Cc = texture(Source, vec2(Sample[1].x, Sample[1].y)).rgb * sampleWeight[2];    
    vec3 Cr = texture(Source, vec2(Sample[2].x, Sample[1].y)).rgb * sampleWeight[3];    
    vec3 Cb = texture(Source, vec2(Sample[1].x, Sample[2].y)).rgb * sampleWeight[4];
    
    float WeightMultiplier = 1./(sampleWeight[0]+sampleWeight[1]+sampleWeight[2]+sampleWeight[3]+sampleWeight[4]);

    FragColor = vec4((Ct+Cl+Cc+Cr+Cb)*WeightMultiplier, 1.0);
}
