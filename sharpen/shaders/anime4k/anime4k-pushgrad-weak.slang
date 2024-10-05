#version 450

// MIT License

// Copyright (c) 2019 bloc97

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float STRENGTH_PUSH, STRENGTH_GRAD;
} params;

#pragma parameter STRENGTH_GRAD "Gradient strength" 0.3 0.0 1.0 0.1
#define STRENGTH_GRAD params.STRENGTH_GRAD

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

//#define strength 0.7

float min_3(vec4 a, vec4 b, vec4 c) {
	return min(min(a.a, b.a), c.a);
}
float max_3(vec4 a, vec4 b, vec4 c) {
	return max(max(a.a, b.a), c.a);
}

vec4 getAverage(vec4 cc, vec4 a, vec4 b, vec4 c) {
	return cc * (1 - STRENGTH_GRAD) + ((a + b + c) / 3) * STRENGTH_GRAD;
}

void main()
{
	float dx = params.SourceSize.z;
	float dy = params.SourceSize.w;

	vec4 cc = texture(Source, vTexCoord); //Current Color
	
	vec4 t  = texture(Source, vTexCoord + vec2(  0, -dy));
	vec4 tl = texture(Source, vTexCoord + vec2(-dx, -dy));
	vec4 tr = texture(Source, vTexCoord + vec2( dx, -dy));
	
	vec4 l  = texture(Source, vTexCoord + vec2(-dx,   0));
	vec4 r  = texture(Source, vTexCoord + vec2( dx,   0));
	
	vec4 b  = texture(Source, vTexCoord + vec2(  0,  dy));
	vec4 bl = texture(Source, vTexCoord + vec2(-dx,  dy));
	vec4 br = texture(Source, vTexCoord + vec2( dx,  dy));
	
	
	vec4 lightestColor = cc;

	//Kernel 0 and 4
	float maxDark  = max_3(br, b, bl);
	float minLight = min_3(tl, t, tr);
	
	if (minLight > cc.a && minLight > maxDark) {
		FragColor = getAverage(cc, tl, t, tr);
      return;
	} else {
		maxDark  = max_3(tl, t, tr);
		minLight = min_3(br, b, bl);
		if (minLight > cc.a && minLight > maxDark) {
			FragColor = getAverage(cc, br, b, bl);
         return;
		}
	}
	
	//Kernel 1 and 5
	maxDark  = max_3(cc, l, b);
	minLight = min_3(r, t, tr);
	
	if (minLight > maxDark) {
		FragColor = getAverage(cc, r, t, tr);
         return;
	} else {
		maxDark  = max_3(cc, r, t);
		minLight = min_3(bl, l, b);
		if (minLight > maxDark) {
			FragColor = getAverage(cc, bl, l, b);
         return;
		}
	}
	
	//Kernel 2 and 6
	maxDark  = max_3(l, tl, bl);
	minLight = min_3(r, br, tr);
	
	if (minLight > cc.a && minLight > maxDark) {
		FragColor = getAverage(cc, r, br, tr);
         return;
	} else {
		maxDark  = max_3(r, br, tr);
		minLight = min_3(l, tl, bl);
		if (minLight > cc.a && minLight > maxDark) {
			FragColor = getAverage(cc, l, tl, bl);
         return;
		}
	}
	
	//Kernel 3 and 7
	maxDark  = max_3(cc, l, t);
	minLight = min_3(r, br, b);
	
	if (minLight > maxDark) {
		FragColor = getAverage(cc, r, br, b);
         return;
	} else {
		maxDark  = max_3(cc, r, b);
		minLight = min_3(t, l, tl);
		if (minLight > maxDark) {
			FragColor = getAverage(cc, t, l, tl);
         return;
		}
	}
	
	FragColor = cc;
}
