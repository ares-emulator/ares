#ifndef LUT
#define LUT

// only #include in the fragment stage
#pragma parameter LUT_Size1 "LUT Size 1" 16.0 1.0 64.0 1.0
#pragma parameter LUT_Size2 "LUT Size 2" 16.0 1.0 64.0 1.0

layout(set = 0, binding = 3) uniform sampler2D SamplerLUT1;
layout(set = 0, binding = 4) uniform sampler2D SamplerLUT2;

// This shouldn't be necessary but it seems some undefined values can
// creep in and each GPU vendor handles that differently. This keeps
// all values within a safe range
vec3 mixfix(vec3 a, vec3 b, float c)
{
	return (a.z < 1.0) ? mix(a, b, c) : a;
}

vec3 lut1(vec3 in_col)
{
	float red = ( in_col.r * (LUT_Size1 - 1.0) + 0.4999 ) / (LUT_Size1 * LUT_Size1);
	float green = ( in_col.g * (LUT_Size1 - 1.0) + 0.4999 ) / LUT_Size1;
	float blue1 = (floor( in_col.b  * (LUT_Size1 - 1.0) ) / LUT_Size1) + red;
	float blue2 = (ceil( in_col.b  * (LUT_Size1 - 1.0) ) / LUT_Size1) + red;
	float mixer = clamp(max((in_col.b - blue1) / (blue2 - blue1), 0.0), 0.0, 32.0);
	vec3 color1 = texture( SamplerLUT1, vec2( blue1, green )).rgb;
	vec3 color2 = texture( SamplerLUT1, vec2( blue2, green )).rgb;
	return mixfix(color1, color2, mixer);
}

vec3 lut2(vec3 in_col)
{
	float red = ( in_col.r * (LUT_Size2 - 1.0) + 0.4999 ) / (LUT_Size2 * LUT_Size2);
	float green = ( in_col.g * (LUT_Size2 - 1.0) + 0.4999 ) / LUT_Size2;
	float blue1 = (floor( in_col.b  * (LUT_Size2 - 1.0) ) / LUT_Size2) + red;
	float blue2 = (ceil( in_col.b  * (LUT_Size2 - 1.0) ) / LUT_Size2) + red;
	float mixer = clamp(max((in_col.b - blue1) / (blue2 - blue1), 0.0), 0.0, 32.0);
	vec3 color1 = texture( SamplerLUT2, vec2( blue1, green )).rgb;
	vec3 color2 = texture( SamplerLUT2, vec2( blue2, green )).rgb;
	return mixfix(color1, color2, mixer);
}

#endif
