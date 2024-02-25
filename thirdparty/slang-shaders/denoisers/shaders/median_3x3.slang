#version 450

/*
3x3 Median optimized for GeForce 8800

Morgan McGuire and Kyle Whitson
Williams College

Register allocation tips by Victor Huang Xiaohuang
University of Illinois at Urbana-Champaign

http://graphics.cs.williams.edu


Copyright (c) Morgan McGuire and Williams College, 2006
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#define s2(a, b)				temp = a; a = min(a, b); b = max(temp, b);
#define mn3(a, b, c)			s2(a, b); s2(a, c);
#define mx3(a, b, c)			s2(b, c); s2(a, c);

#define mnmx3(a, b, c)			mx3(a, b, c); s2(a, b);                                   // 3 exchanges
#define mnmx4(a, b, c, d)		s2(a, b); s2(c, d); s2(a, c); s2(b, d);                   // 4 exchanges
#define mnmx5(a, b, c, d, e)	s2(a, b); s2(c, d); mn3(a, c, e); mx3(b, d, e);           // 6 exchanges
#define mnmx6(a, b, c, d, e, f) s2(a, d); s2(b, e); s2(c, f); mn3(a, b, c); mx3(d, e, f); // 7 exchanges

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
   
   t1 = vTexCoord.xxxy + vec4(-dx, 0, dx, -dy);
	t2 = vTexCoord.xxxy + vec4(-dx, 0, dx,   0);
	t3 = vTexCoord.xxxy + vec4(-dx, 0, dx,  dy);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  vec3 v[6];

  v[0] = texture(Source, t1.xw).rgb;
  v[1] = texture(Source, t1.yw).rgb;
  v[2] = texture(Source, t1.zw).rgb;
  v[3] = texture(Source, t2.xw).rgb;
  v[4] = texture(Source, t2.yw).rgb;
  v[5] = texture(Source, t2.zw).rgb;

  // Starting with a subset of size 6, remove the min and max each time
  vec3 temp;
  mnmx6(v[0], v[1], v[2], v[3], v[4], v[5]);

  v[5] = texture(Source, t3.xw).rgb;

  mnmx5(v[1], v[2], v[3], v[4], v[5]);

  v[5] = texture(Source, t3.yw).rgb;

  mnmx4(v[2], v[3], v[4], v[5]);

  v[5] = texture(Source, t3.zw).rgb;

  mnmx3(v[3], v[4], v[5]);
  FragColor = vec4(v[4], 1.0);
}