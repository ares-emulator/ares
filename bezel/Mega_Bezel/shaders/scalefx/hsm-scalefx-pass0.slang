#version 450

/*
	ScaleFX - Pass 0
	by Sp00kyFox, 2017-03-01

Filter:	Nearest
Scale:	1x

ScaleFX is an edge interpolation algorithm specialized in pixel art. It was
originally intended as an improvement upon Scale3x but became a new filter in
its own right.
ScaleFX interpolates edges up to level 6 and makes smooth transitions between
different slopes. The filtered picture will only consist of colours present
in the original.

Pass 0 prepares metric data for the next pass.



Copyright (c) 2016 Sp00kyFox - ScaleFX@web.de

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

#include "hsm-scalefx.inc"

layout(set = 0, binding = 0, std140) uniform UBO
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
layout(binding = 1) uniform sampler2D Source;


// Reference: http://www.compuphase.com/cmetric.htm
float dist(vec3 A, vec3 B)
{
	float r = 0.5 * (A.r + B.r);
	vec3 d = A - B;
	vec3 c = vec3(2 + r, 4, 3 - r);

	return sqrt(dot(c*d, d)) / 3;
}


void main()
{
	if (HSM_SCALEFX_ON < 0.5) 
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

	/*	grid		metric

		A B C		x y z
		  E F		  o w
	*/

// HSM Removed
// #define TEX(x, y) textureOffset(Source, vTexCoord, ivec2(x, y)).rgb

// HSM Added
#define TEX(x, y) HSM_TextureOffset(Source, vTexCoord, vec2(x, y), params.SourceSize.xy).rgb

	// read texels
	vec3 A = TEX(-1,-1);
	vec3 B = TEX( 0,-1);
	vec3 C = TEX( 1,-1);
	vec3 E = TEX( 0, 0);
	vec3 F = TEX( 1, 0);

	// output
	FragColor = vec4(dist(E,A), dist(E,B), dist(E,C), dist(E,F));
}
