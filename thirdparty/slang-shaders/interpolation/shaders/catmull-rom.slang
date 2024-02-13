#version 450

/*
	Bicubic Catmull-Rom 9 taps (Fast) - ported by Hyllian - 2020

	The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae

	Ported from code: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1

	Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
	See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details

	ATENTION: This code only work using LINEAR filter sampling set on Retroarch!

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define mul(c,d) (d*c)

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
    vec2 samplePos = vTexCoord * params.SourceSize.xy;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    vec2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
 //   vec2 w3 = 1.0 - w0 - w1 - w2;

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    vec2 texPos0  = texPos1 - 1.;
    vec2 texPos3  = texPos1 + 2.;
    vec2 texPos12 = texPos1 + offset12;

    texPos0  *= params.SourceSize.zw;
    texPos3  *= params.SourceSize.zw;
    texPos12 *= params.SourceSize.zw;

    vec4 c00  = texture(Source, vec2(texPos0.x, texPos0.y));
    vec4 c10  = texture(Source, vec2(texPos12.x, texPos0.y));
    vec4 c20  = texture(Source, vec2(texPos3.x, texPos0.y));

    vec4 c01  = texture(Source, vec2(texPos0.x, texPos12.y));
    vec4 c11  = texture(Source, vec2(texPos12.x, texPos12.y));
    vec4 c21  = texture(Source, vec2(texPos3.x, texPos12.y));

    vec4 c02  = texture(Source, vec2(texPos0.x, texPos3.y));
    vec4 c12  = texture(Source, vec2(texPos12.x, texPos3.y));
    vec4 c22  = texture(Source, vec2(texPos3.x, texPos3.y));

    vec3 wx   = vec3(w0.x, w12.x, w3.x);
    vec3 wy   = vec3(w0.y, w12.y, w3.y);

    vec4 c1   = mul(wx, mat3x4(c00, c10, c20));
    vec4 c2   = mul(wx, mat3x4(c01, c11, c21));
    vec4 c3   = mul(wx, mat3x4(c02, c12, c22));

    FragColor = mul(wy, mat3x4(c1, c2, c3));
}