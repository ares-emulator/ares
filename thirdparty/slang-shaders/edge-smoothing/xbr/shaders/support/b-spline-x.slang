#version 450

/*
   b-spline-x Shader

   Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

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

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// B-Spline bicubic parameters
const float  B =  1.0; 
const float  C =  0.0;

const mat4 INV  = mat4(            (-B - 6.0*C)/6.0,         (3.0*B + 12.0*C)/6.0,     (-3.0*B - 6.0*C)/6.0,             B/6.0,
                                        (12.0 - 9.0*B - 6.0*C)/6.0, (-18.0 + 12.0*B + 6.0*C)/6.0,                      0.0, (6.0 - 2.0*B)/6.0,
                                       -(12.0 - 9.0*B - 6.0*C)/6.0, (18.0 - 15.0*B - 12.0*C)/6.0,      (3.0*B + 6.0*C)/6.0,             B/6.0,
                                                   (B + 6.0*C)/6.0,                           -C,                      0.0,               0.0);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;

void main()
{
   gl_Position = global.MVP * Position;
   
   vec2 ps = vec2(params.SourceSize.z, params.SourceSize.w);
	float dx = ps.x;
	float dy = ps.y;

        vTexCoord = TexCoord * 1.0004 - vec2(0.5, 0.0)*ps;

	t1 = vTexCoord.xyxy + vec4( -dx, 0.0,    0.0, 0.0); 
	t2 = vTexCoord.xyxy + vec4(  dx, 0.0, 2.0*dx, 0.0);

}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
  vec2 fp = fract(vTexCoord*params.SourceSize.xy);

  vec3 C0 = texture(Source, t1.xy).xyz;
  vec3 C1 = texture(Source, t1.zw).xyz;
  vec3 C2 = texture(Source, t2.xy).xyz;
  vec3 C3 = texture(Source, t2.zw).xyz;

  vec4 Px    = vec4(fp.x*fp.x*fp.x, fp.x*fp.x, fp.x, 1.0) * INV;
  vec3 color = mat4x3(C0, C1, C2, C3) * Px;

   FragColor = vec4(color, 1.0);
}
