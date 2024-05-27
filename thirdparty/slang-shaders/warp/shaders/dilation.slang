#version 450

/*
   Hyllian's dilation Shader
   
   Copyright (C) 2011-2024 Hyllian - sergiogdb@gmail.com

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
	float DILATION_STRENGTH;
} params;

#pragma parameter DILATION_STRENGTH "Dilation Strength" 0.5 0.0 1.0 0.1

#define DILATION_STRENGTH params.DILATION_STRENGTH

const vec3 luma = vec3(0.299, 0.587, 0.114);

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t2;
layout(location = 2) out vec4 t3;
layout(location = 3) out vec4 t4;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
   
	float dx = params.SourceSize.z;
	float dy = params.SourceSize.w;

	//    A1 B1 C1
	// A0  A  B  C C4
	// D0  D  E  F F4
	// G0  G  H  I I4
	//    G5 H5 I5

	t2 = vTexCoord.xxxy + vec4( -dx, 0., dx,    -dy ); //  A  B  C
	t3 = vTexCoord.xxxy + vec4( -dx, 0., dx,      0.); //  D  E  F
	t4 = vTexCoord.xxxy + vec4( -dx, 0., dx,     dy ); //  G  H  I
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t2;
layout(location = 2) in vec4 t3;
layout(location = 3) in vec4 t4;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


void main()
{
	vec3 B  = texture(Source, t2.yw).rgb;
	vec3 D  = texture(Source, t3.xw).rgb;
	vec3 E  = texture(Source, t3.yw).rgb;
	vec3 F  = texture(Source, t3.zw).rgb;

	vec3 H  = texture(Source, t4.yw).rgb;

	vec4 b = luma * mat4x3(B, D, H, F);
	float e = dot(E, luma);


	float di = max(max(b.x, b.y), max(b.z, max(b.w, e)));
	vec3 color = (di==b.x) ? B : (di==b.y) ? D : (di==b.z) ? H : (di==b.w) ? F : E;

	color = mix(E, color, DILATION_STRENGTH);

	FragColor = vec4(color, 1.0);
}
