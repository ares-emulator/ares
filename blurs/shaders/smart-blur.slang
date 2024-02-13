#version 450

/*
   Hyllian Smart-Blur Shader
  
   Copyright (C) 2011-2020 Hyllian - sergiogdb@gmail.com

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
	float SB_BLUR_LEVEL;
	float SB_RED_THRESHOLD;
	float SB_GREEN_THRESHOLD;
	float SB_BLUE_THRESHOLD;
} params;

#pragma parameter SB_BLUR_LEVEL "Smart Blur Level" 0.66 0.00 1.00 0.02
#pragma parameter SB_RED_THRESHOLD "Smart Blur Red Threshold" 0.2 0.0 0.6 0.01
#pragma parameter SB_GREEN_THRESHOLD "Smart Blur Green Threshold" 0.2 0.0 0.6 0.01
#pragma parameter SB_BLUE_THRESHOLD "Smart Blur Blue Threshold" 0.2 0.0 0.6 0.01

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;
layout(location = 3) out vec4 t3;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	float dx = params.SourceSize.z;
	float dy = params.SourceSize.w;

	t1 = vTexCoord.xxxy + vec4(	-dx,	0.0,	dx,	-dy); //  A B C
	t2 = vTexCoord.xxxy + vec4(	-dx,	0.0,	dx,	0.0); //  D E F
	t3 = vTexCoord.xxxy + vec4( -dx,	0.0,	dx,	dy); //  G H I
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

// Below the thresholds, blur is applied for each color channel.
// Threshold is the max color differency among the eight pixel neighbors from central pixel.

bool eq(vec3 c1, vec3 c2) {
    vec3 df = abs(c1 - c2);
    return (df.r < params.SB_RED_THRESHOLD) && (df.g < params.SB_GREEN_THRESHOLD) && (df.b < params.SB_BLUE_THRESHOLD);
}

/*       
           A  B  C
           D  E  F 
           G  H  I
*/

void main()
{
	vec3   A = texture(Source, t1.xw).xyz;
	vec3   B = texture(Source, t1.yw).xyz;
	vec3   C = texture(Source, t1.zw).xyz;
	vec3   D = texture(Source, t2.xw).xyz;
	vec3   E = texture(Source, t2.yw).xyz;
	vec3   F = texture(Source, t2.zw).xyz;
	vec3   G = texture(Source, t3.xw).xyz;
	vec3   H = texture(Source, t3.yw).xyz;
	vec3   I = texture(Source, t3.zw).xyz;

	vec3 sum = vec3(0.,0.,0.);

	if (eq(E,F) && eq(E,H) && eq(E,I) && eq(E,B) && eq(E,C) && eq(E,A) && eq(E,D) && eq(E,G))
	{
		sum = (A+C+D+F+G+I+B+H)/8.0;
		E = mix(E, sum, params.SB_BLUR_LEVEL);
	}

	FragColor = vec4(E, 1.0);
}
