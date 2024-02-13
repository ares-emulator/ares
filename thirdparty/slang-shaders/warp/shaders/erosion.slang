#version 450

/*
   Hyllian's erosion Shader
   
   Copyright (C) 2011-2015 Hyllian - sergiogdb@gmail.com

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
} params;

const float y_weight        = 48.0;
const float u_weight        = 7.0;
const float v_weight        = 6.0;
const mat3 yuv          = mat3(0.299, 0.587, 0.114, -0.169, -0.331, 0.499, 0.499, -0.418, -0.0813);
const mat3 yuv_weighted = mat3(y_weight*yuv[0], u_weight*yuv[1], v_weight*yuv[2]);

#define mul(c,d) (d*c)

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

	vec4 b = mul( mat4x3(B, D, H, F), yuv_weighted[0] );
	vec4 e = mul( mat4x3(E, E, E, E), yuv_weighted[0] );


	float di = min(min(b.x, b.y), min(b.z, min(b.w, e.x)));
	vec3 res = (di==b.x) ? B : (di==b.y) ? D : (di==b.z) ? H : (di==b.w) ? F : E;

	FragColor = vec4(res, 1.0);
}