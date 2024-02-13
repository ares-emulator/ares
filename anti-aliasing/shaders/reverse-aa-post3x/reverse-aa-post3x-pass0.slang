#version 450

/*
	rAA post-3x - Pass 0
	by Sp00kyFox, 2018-10-20

Filter:	Nearest
Scale:	1x

This is a generalized continuation of the reverse antialiasing filter by
Christoph Feck. Unlike the original filter this is supposed to be used on an
already upscaled image. Which makes it possible to combine rAA with other filters
just as ScaleFX, xBR or others.

Pass 0 does the horizontal filtering.



Copyright (c) 2018 Sp00kyFox - ScaleFX@web.de

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
	float RAA_SHR0;
	float RAA_SMT0;
	float RAA_DVT0;
} params;

#pragma parameter RAA_SHR0 "rAA-3x 0 Sharpness"  2.0 0.00 10.0 0.05
#pragma parameter RAA_SMT0 "rAA-3x 0 Smoothness" 0.5 0.05 10.0 0.05
#pragma parameter RAA_DVT0 "rAA-3x 0 Deviation"  1.0 0.05 10.0 0.05

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

const int scl = 3; // scale factor
const int rad = 7; // search radius

// core function of rAA - tilt of a pixel
vec3 res2x(vec3 pre2, vec3 pre1, vec3 px, vec3 pos1, vec3 pos2)
{
    float d1, d2, w;
	vec3 a, m, t, t1, t2;
    mat4x3 pre = mat4x3(pre2, pre1,   px, pos1);
    mat4x3 pos = mat4x3(pre1,   px, pos1, pos2);
    mat4x3  df = pos - pre;

    m.x = (px.x < 0.5) ? px.x : (1.0-px.x);
    m.y = (px.y < 0.5) ? px.y : (1.0-px.y);
    m.z = (px.z < 0.5) ? px.z : (1.0-px.z);
	m = params.RAA_SHR0 * min(m, min(abs(df[1]), abs(df[2])));   // magnitude
	t = (7 * (df[1] + df[2]) - 3 * (df[0] + df[3])) / 16; // tilt
	
	a.x = t.x == 0.0 ? 1.0 : m.x/abs(t.x);
	a.y = t.y == 0.0 ? 1.0 : m.y/abs(t.y);
	a.z = t.z == 0.0 ? 1.0 : m.z/abs(t.z);
	t1 = clamp(t, -m, m);                       // limit channels
	t2 = min(1.0, min(min(a.x, a.y), a.z)) * t; // limit length
	
	d1 = length(df[1]); d2 = length(df[2]);
	d1 = d1 == 0.0 ? 0.0 : length(cross(df[1], t1))/d1; // distance between line (px, pre1) and point px-t1
	d2 = d2 == 0.0 ? 0.0 : length(cross(df[2], t1))/d2; // distance between line (px, pos1) and point px+t1

	w = min(1.0, max(d1,d2)/0.8125); // color deviation from optimal value
	
	return mix(t1, t2, pow(w, params.RAA_DVT0));
}

void main()
{
	// read texels

	vec3 tx[2*rad+1];

	#define TX(n) tx[(n)+rad]
	
	TX(0) = texture(Source, vTexCoord).rgb;
	
	for(int i=1; i<=rad; i++){
		TX(-i) = texture(Source, vTexCoord + vec2(-i,0)*params.OutputSize.zw).rgb;
		TX( i) = texture(Source, vTexCoord + vec2( i,0)*params.OutputSize.zw).rgb;
	}
	
	
	// prepare variables for candidate search
	
	ivec2 i1 = ivec2(0), i2 = ivec2(0);
	vec3 df1, df2;
	vec2 d1, d2, d3;
	bvec2 cn;
	
	df1 = TX(1)-TX(0); df2 = TX(0)-TX(-1);
	
	d2 = vec2(length(df1), length(df2));
	d3 = d2.yx;
	
	
	// smoothness weight, protects smooth gradients
	float sw = d2.x + d2.y;
	sw = sw == 0.0 ? 1.0 : pow(length(df1-df2)/sw, params.RAA_SMT0);
	
	
	// look for proper candidates
	for(int i=1; i<rad; i++){
		d1 = d2;
		d2 = d3;
		d3 = vec2(distance(TX(-i-1), TX(-i)), distance(TX(i), TX(i+1)));
		cn.x = max(d1.x,d3.x)<d2.x;
		cn.y = max(d1.y,d3.y)<d2.y;
		i2.x = cn.x && i2.x==0 && i1.x!=0 ? i : i2.x;
		i2.y = cn.y && i2.y==0 && i1.y!=0 ? i : i2.y;
		i1.x = cn.x && i1.x==0 ? i : i1.x;
		i1.y = cn.y && i1.y==0 ? i : i1.y;
	}

	i2.x = i2.x == 0 ? i1.x+1 : i2.x;
	i2.y = i2.y == 0 ? i1.y+1 : i2.y;

	
	// rAA core with the candidates found above
	vec3 t = res2x(TX(-i2.x), TX(-i1.x), TX(0), TX(i1.y), TX(i2.y));

	// distance weight
	float dw = (i1.x == 0 || i1.y == 0) ? 0.0 : 2.0 * ((i1.x-1.0)/(i1.x+i1.y-2.0)) - 1.0;	
	
	// result
	vec3 res = TX(0) + (scl-1.0)/scl * sw*dw * t;
	
	
	// prevent ringing	
	vec3 lo  = min(min(TX(-1),TX(0)),TX(1));
    vec3 hi  = max(max(TX(-1),TX(0)),TX(1));
	
    FragColor = vec4(clamp(res, lo, hi), 1.0);
}
