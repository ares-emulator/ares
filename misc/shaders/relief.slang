#version 450

/*
   Hyllian's Relief Shader
   
   Copyright (C) 2024 Hyllian

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

vec3 three_point(vec2 fp, vec3 A, vec3 B, vec3 C)
{
    return (A + fp.x*(B-A) + fp.y*(C-A));
}


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec2 loc;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;
   
    vec2 ps = vec2(params.SourceSize.z, params.SourceSize.w);
    float dx = ps.x;
    float dy = ps.y;

    t1.xy = vec2( dx,  0); // F
    t1.zw = vec2(  0, dy); // H
    loc = vTexCoord*params.SourceSize.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec2 loc;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec2 tc = (floor(loc)+vec2(0.5,0.5));

    vec2 fp = loc - tc;

    tc = tc/params.SourceSize.xy;

    vec2 dx = vec2(sign(fp.x), 0.0)/params.SourceSize.xy;
    vec2 dy = vec2(0.0, sign(fp.y))/params.SourceSize.xy;

    vec3 E  = texture(Source, tc          ).rgb;
    vec3 F  = texture(Source, tc + dx     ).rgb;
    vec3 H  = texture(Source, tc      + dy).rgb;
    vec3 I  = texture(Source, tc + dx + dy).rgb;

    //  E  F 
    //  H  I 

    vec3 res = mix(three_point(fp, E, F, H), three_point(1-fp, I, H, F), step(1.0, fp.x+fp.y));

    FragColor = vec4(res, 1.0);
}
