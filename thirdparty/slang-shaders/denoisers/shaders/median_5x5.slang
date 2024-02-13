#version 450

/*
5x5 Median

GLSL 1.0
Morgan McGuire and Kyle Whitson, 2006
Williams College
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
#define t2(a, b)				s2(v[a], v[b]);
#define t24(a, b, c, d, e, f, g, h)			t2(a, b); t2(c, d); t2(e, f); t2(g, h); 
#define t25(a, b, c, d, e, f, g, h, i, j)		t24(a, b, c, d, e, f, g, h); t2(i, j);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t01;
layout(location = 2) out vec4 t02;
layout(location = 3) out vec4 t03;
layout(location = 4) out vec4 t04;
layout(location = 5) out vec4 t05;
layout(location = 6) out vec4 t06;
layout(location = 7) out vec4 t07;
layout(location = 8) out vec4 t08;
layout(location = 9) out vec4 t09;
layout(location = 10) out vec4 t10;
layout(location = 11) out vec4 t11;
layout(location = 12) out vec4 t12;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
   float dx1 = params.SourceSize.z;
	float dx2 = params.SourceSize.z + params.SourceSize.z;
	float dy1 = params.SourceSize.w;
	float dy2 = params.SourceSize.w + params.SourceSize.w;

	t01 = vTexCoord.xyxy + vec4(-dx2, -dy2, -dx1, -dy2);
	t02 = vTexCoord.xyxy + vec4(   0, -dy2,  dx1, -dy2);
	t03 = vTexCoord.xyxy + vec4( dx2, -dy2, -dx2, -dy1);
	t04 = vTexCoord.xyxy + vec4(-dx1, -dy1,    0, -dy1);
	t05 = vTexCoord.xyxy + vec4( dx1, -dy1,  dx2, -dy1);
	t06 = vTexCoord.xyxy + vec4(-dx2,    0, -dx1,    0);

	t07 = vTexCoord.xyxy + vec4( dx1,    0,  dx2,    0);
	t08 = vTexCoord.xyxy + vec4(-dx2,  dy1, -dx1,  dy1);
	t09 = vTexCoord.xyxy + vec4(   0,  dy1,  dx1,  dy1);
	t10 = vTexCoord.xyxy + vec4( dx2,  dy1, -dx2,  dy2);
	t11 = vTexCoord.xyxy + vec4(-dx1,  dy2,    0,  dy2);
	t12 = vTexCoord.xyxy + vec4( dx1,  dy2,  dx2,  dy2);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t01;
layout(location = 2) in vec4 t02;
layout(location = 3) in vec4 t03;
layout(location = 4) in vec4 t04;
layout(location = 5) in vec4 t05;
layout(location = 6) in vec4 t06;
layout(location = 7) in vec4 t07;
layout(location = 8) in vec4 t08;
layout(location = 9) in vec4 t09;
layout(location = 10) in vec4 t10;
layout(location = 11) in vec4 t11;
layout(location = 12) in vec4 t12;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  vec3 v[25];

  /*
  // Add the pixels which make up our window to the pixel array.
  for(int dX = -2; dX <= 2; ++dX) {
    for(int dY = -2; dY <= 2; ++dY) {
      vec2 offset = vec2(float(dX), float(dY));

      // If a pixel in the window is located at (x+dX, y+dY), put it at index (dX + R)(2R + 1) + (dY + R) of the
      // pixel array. This will fill the pixel array, with the top left pixel of the window at pixel[0] and the
      // bottom right pixel of the window at pixel[N-1].
      v[(dX + 2) * 5 + (dY + 2)] = texture(Source, vTexCoord.xy + offset * params.SourceSize.zw).rgb;
    }
  }
  */
  
  v[0]  = texture(Source, t01.xy).rgb;
  v[5]  = texture(Source, t01.zw).rgb;
  v[10] = texture(Source, t02.xy).rgb;
  v[15] = texture(Source, t02.zw).rgb;
  v[20] = texture(Source, t03.xy).rgb;
  v[1]  = texture(Source, t03.zw).rgb;
  v[6]  = texture(Source, t04.xy).rgb;
  v[11] = texture(Source, t04.zw).rgb;
  v[16] = texture(Source, t05.xy).rgb;
  v[21] = texture(Source, t05.zw).rgb;
  v[2]  = texture(Source, t06.xy).rgb;
  v[7]  = texture(Source, t06.zw).rgb;
  v[12] = texture(Source, vTexCoord.xy).rgb;
  v[17] = texture(Source, t07.xy).rgb;
  v[22] = texture(Source, t07.zw).rgb;
  v[3]  = texture(Source, t08.xy).rgb;
  v[8]  = texture(Source, t08.zw).rgb;
  v[13] = texture(Source, t09.xy).rgb;
  v[18] = texture(Source, t09.zw).rgb;
  v[23] = texture(Source, t10.xy).rgb;
  v[4]  = texture(Source, t10.zw).rgb;
  v[9]  = texture(Source, t11.xy).rgb;
  v[14] = texture(Source, t11.zw).rgb;
  v[19] = texture(Source, t12.xy).rgb;
  v[24] = texture(Source, t12.zw).rgb;

  vec3 temp;

  t25(0, 1,			3, 4,		2, 4,		2, 3,		6, 7);
  t25(5, 7,			5, 6,		9, 7,		1, 7,		1, 4);
  t25(12, 13,		11, 13,		11, 12,		15, 16,		14, 16);
  t25(14, 15,		18, 19,		17, 19,		17, 18,		21, 22);
  t25(20, 22,		20, 21,		23, 24,		2, 5,		3, 6);
  t25(0, 6,			0, 3,		4, 7,		1, 7,		1, 4);
  t25(11, 14,		8, 14,		8, 11,		12, 15,		9, 15);
  t25(9, 12,		13, 16,		10, 16,		10, 13,		20, 23);
  t25(17, 23,		17, 20,		21, 24,		18, 24,		18, 21);
  t25(19, 22,		8, 17,		9, 18,		0, 18,		0, 9);
  t25(10, 19,		1, 19,		1, 10,		11, 20,		2, 20);
  t25(2, 11,		12, 21,		3, 21,		3, 12,		13, 22);
  t25(4, 22,		4, 13,		14, 23,		5, 23,		5, 14);
  t25(15, 24,		6, 24,		6, 15,		7, 16,		7, 19);
  t25(3, 11,		5, 17,		11, 17,		9, 17,		4, 10);
  t25(6, 12,		7, 14,		4, 6,		4, 7,		12, 14);
  t25(10, 14,		6, 7,		10, 12,		6, 10,		6, 17);
  t25(12, 17,		7, 17,		7, 10,		12, 18,		7, 12);
  t24(10, 18,		12, 20,		10, 20,		10, 12);

  FragColor = vec4(v[12], 1.0);
}