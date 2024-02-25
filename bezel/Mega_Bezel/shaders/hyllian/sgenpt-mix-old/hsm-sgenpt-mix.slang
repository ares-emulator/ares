#version 450

/*
   SGENPT-MIX - Sega Genesis Pseudo Transparency Mixer Shader - v8b
   
   2011-2020 Hyllian - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is 
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.

*/


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float SGPT_BLEND_OPTION;
	float SGPT_BLEND_LEVEL;
	float SGPT_ADJUST_VIEW;
	float SGPT_LINEAR_GAMMA;
	float SGPT_ON;
} params;

#pragma parameter SGPT_TITLE			"[ --- SGENPT-MIX DE-DITHERING V8b --- ]:" 0 0 0.01 0.01

#pragma parameter SGPT_ON 				"    SGENPT-MIX ON" 0 0 1 1
#pragma parameter SGPT_BLEND_OPTION 	"    Pass 1 Blend - OFF | VL | CB | CB-S | Both | Both2 | Both-S" 4.0 0.0 6.0 1.0
#pragma parameter SGPT_BLEND_LEVEL 		"    Pass 1 Both Blend Level" 1.0 0.0 1.0 0.1
// #pragma parameter SGPT_ADJUST_VIEW 	"          Adjust View (Debug)" 0.0 0.0 1.0 1.0
#pragma parameter SGPT_LINEAR_GAMMA 	"    Pass 1 Use Linear Gamma" 0.0 0.0 1.0 1.0
#define SGPT_BLEND_OPTION (params.SGPT_BLEND_OPTION * params.SGPT_ON)
#define SGPT_BLEND_LEVEL params.SGPT_BLEND_LEVEL
#define SGPT_ADJUST_VIEW 0
#define SGPT_LINEAR_GAMMA params.SGPT_LINEAR_GAMMA

#define GAMMA_EXP		(SGPT_LINEAR_GAMMA+1.0)
#define GAMMA_IN(color)		pow(color, vec3(GAMMA_EXP, GAMMA_EXP, GAMMA_EXP))
#define GAMMA_OUT(color)	pow(color, vec3(1.0 / GAMMA_EXP, 1.0 / GAMMA_EXP, 1.0 / GAMMA_EXP))

const vec3 Y = vec3(.2126, .7152, .0722);

vec3 min_s(vec3 central, vec3 adj1, vec3 adj2) {return min(central, max(adj1, adj2));}
vec3 max_s(vec3 central, vec3 adj1, vec3 adj2) {return max(central, min(adj1, adj2));}

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
	vec2 dx = vec2(1.0, 0.0)*params.SourceSize.zw;
	vec2 dy = vec2(0.0, 1.0)*params.SourceSize.zw;

	// Reading the texels.
	vec3 C = GAMMA_IN(texture(Source, vTexCoord    ).xyz);
	vec3 L = GAMMA_IN(texture(Source, vTexCoord -dx).xyz);
	vec3 R = GAMMA_IN(texture(Source, vTexCoord +dx).xyz);
	vec3 U = GAMMA_IN(texture(Source, vTexCoord -dy).xyz);
	vec3 D = GAMMA_IN(texture(Source, vTexCoord +dy).xyz);
	vec3 UL = GAMMA_IN(texture(Source, vTexCoord -dx -dy).xyz);
	vec3 UR = GAMMA_IN(texture(Source, vTexCoord +dx -dy).xyz);
	vec3 DL = GAMMA_IN(texture(Source, vTexCoord -dx +dy).xyz);
	vec3 DR = GAMMA_IN(texture(Source, vTexCoord +dx +dy).xyz);

	vec3 color = C;

	//  Get min/max samples
	vec3 min_sample = min_s(C, L, R);
	vec3 max_sample = max_s(C, L, R);

	float diff = dot(max(max(C, L), max(C, R)) - min(min(C, L), min(C, R)), Y);

	if (SGPT_BLEND_OPTION == 1) // Only Vertical Lines
	{
		min_sample = max_s(min_sample, min_s(C, DL, DR), min_s(C, UL, UR));
		max_sample = min_s(max_sample, max_s(C, DL, DR), max_s(C, UL, UR));

		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.25*( 1.0 - diff )*(L + R);
	}
	else if (SGPT_BLEND_OPTION == 2) // Only Checkerboard
	{
		min_sample = max(min_sample, min_s(C, U, D));
		max_sample = min(max_sample, max_s(C, U, D));

		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.125*( 1.0 - diff )*(L + R + U + D);
	}
	else if (SGPT_BLEND_OPTION == 3) // Only Checkerboard - Soft
	{
		min_sample = min_s(min_sample, U, D);
		max_sample = max_s(max_sample, U, D);

		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.125*( 1.0 - diff )*(L + R + U + D);
	}
	else if (SGPT_BLEND_OPTION == 4) // VL-CB
	{
		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.25*( 1.0 - diff )*(L + R);
	}
	else if (SGPT_BLEND_OPTION == 5) // VL-CB-2
	{
		min_sample = min_s(min_sample, U, D);
		max_sample = max_s(max_sample, U, D);

		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.25*( 1.0 - diff )*(L + R);
	}
	else if (SGPT_BLEND_OPTION == 6) // VL-CB-Soft
	{
		min_sample = min(min_sample, min(min_s(D, DL, DR), min_s(U, UL, UR)));
		max_sample = max(max_sample, max(max_s(D, DL, DR), max_s(U, UL, UR)));

		diff *= (1.0 - SGPT_BLEND_LEVEL);

		color = 0.5*( 1.0 + diff )*C + 0.25*( 1.0 - diff )*(L + R);
	}

	color = clamp(color, min_sample, max_sample);

	color = mix(color, vec3(dot(abs(C-color), vec3(1.0, 1.0, 1.0))), SGPT_ADJUST_VIEW);

	FragColor = vec4(GAMMA_OUT(color), 1.0);
}
