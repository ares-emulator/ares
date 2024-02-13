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
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	float dx = params.SourceSize.z;
	float dy = params.SourceSize.w;

	vec4 c0 = texture(Source, vTexCoord);
	
	//[tl  t tr]
	//[ l     r]
	//[bl  b br]

	float t  = texture(Source, vTexCoord + vec2(  0, -dy)).a;
	float tl = texture(Source, vTexCoord + vec2(-dx, -dy)).a;
	float tr = texture(Source, vTexCoord + vec2( dx, -dy)).a;
	
	float l  = texture(Source, vTexCoord + vec2(-dx,   0)).a;
	float r  = texture(Source, vTexCoord + vec2( dx,   0)).a;
	
	float b  = texture(Source, vTexCoord + vec2(  0,  dy)).a;
	float bl = texture(Source, vTexCoord + vec2(-dx,  dy)).a;
	float br = texture(Source, vTexCoord + vec2( dx,  dy)).a;
	
	//Horizontal Gradient
	//[-1  0  1]
	//[-2  0  2]
	//[-1  0  1]
	float xgrad = (-tl + tr - l - l + r + r - bl + br);
	
	//Vertical Gradient
	//[-1 -2 -1]
	//[ 0  0  0]
	//[ 1  2  1]
	float ygrad = (-tl - t - t - tr + bl + b + b + br);
	
	//Computes the luminance's gradient and saves it in the unused alpha channel
	FragColor = vec4(c0.r, c0.g, c0.b, 1 - clamp(sqrt(xgrad * xgrad + ygrad * ygrad), 0, 1));
}