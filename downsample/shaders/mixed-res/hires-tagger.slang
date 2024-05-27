#version 450

/*
   Hyllian's high resolution tagger Shader
  
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
	float IR_SCALE;
} params;

#pragma parameter IR_SCALE "INTERNAL RESOLUTION SCALE" 1.0 1.0 8.0 1.0
#pragma parameter B_VIEW "SHOW HIRES DETECTION" 0.0 0.0 1.0 1.0
#pragma parameter B_TRESH "BLUR THRESHOLD" 0.2 0.0 1.0 0.01

#define IR_SCALE params.IR_SCALE

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

#define SCALE  1.0

void main()
{
    vec2 tex = (floor(vTexCoord*params.SourceSize.xy/IR_SCALE) + vec2(0.5, 0.5))*IR_SCALE/params.SourceSize.xy;

    vec2 g1 = vec2(IR_SCALE/params.SourceSize.x, 0.0);
    vec2 g2 = vec2(0.0, IR_SCALE/params.SourceSize.y);

    float k0 = texture(Source, tex +0.1*g1+0.1*g2).a;
    float k1 = texture(Source, tex).a;

    float hires = (k0==k1) ? 0.0 : 1.0;

    FragColor = vec4(texture(Source, vTexCoord).rgb, hires);
}
