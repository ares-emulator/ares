#version 450

/*
	Bicubic B-Spline 4-taps (Fast) - ported by Hyllian - 2024

	The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

	Samples a texture with B-Spline filtering, using only 4 texture fetches instead of 16.
	See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
	Implementation: https://www.shadertoy.com/view/styXDh

	ATENTION: This code only work using LINEAR filter sampling set on Retroarch!

*/

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

    vec2 of  = vec2(1.)-f;
    vec2 of2 = of*of;
    vec2 of3 = of2*of;
    
    vec2 w0 = of3 / 6.0 ;
    vec2 w1 = (vec2(4.) + 3.*f3 - 6.*f2) / 6.0;
    vec2 w2 = (vec2(4.) + 3.*of3 - 6.*of2) / 6.0;
    vec2 w3 = f3 / 6.0;

    vec2 Weight[2];
    vec2 Sample[2];

    Weight[0] = w0 + w1;
    Weight[1] = w2 + w3;

    Sample[0] = tc - (vec2(1.) - w1/Weight[0]);
    Sample[1] = tc + vec2(1.)  + w3/Weight[1];

    Sample[0] *= invTexSize;
    Sample[1] *= invTexSize;

    float sampleWeight[4];
    sampleWeight[0] = Weight[0].x * Weight[0].y;
    sampleWeight[1] = Weight[1].x * Weight[0].y;
    sampleWeight[2] = Weight[0].x * Weight[1].y;
    sampleWeight[3] = Weight[1].x * Weight[1].y;

    vec3 Ctl = texture(Source, vec2(Sample[0].x, Sample[0].y)).rgb * sampleWeight[0];    
    vec3 Ctr = texture(Source, vec2(Sample[1].x, Sample[0].y)).rgb * sampleWeight[1];    
    vec3 Cbl = texture(Source, vec2(Sample[0].x, Sample[1].y)).rgb * sampleWeight[2];    
    vec3 Cbr = texture(Source, vec2(Sample[1].x, Sample[1].y)).rgb * sampleWeight[3];

    FragColor = vec4(Ctl+Ctr+Cbl+Cbr, 1.0);
}
