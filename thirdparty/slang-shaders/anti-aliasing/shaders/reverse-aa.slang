#version 450

/*
   Reverse Antialiasing Shader

   Adapted from the C source (see Copyright below) to shader
   cg language by Hyllian/Jararaca - sergiogdb@gmail.com

   This shader works best in 2x scale.

*/

/*
 *
 *  Copyright (c) 2012, Christoph Feck <christoph@maxiom.de>
 *  All Rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OutputSize;
	float REVERSEAA_SHARPNESS;
} params;

#pragma parameter REVERSEAA_SHARPNESS "ReverseAA Sharpness" 2.0 0.0 10.0 0.01

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

#define P(x,y) texture(Source, coord + params.SourceSize.zw * vec2(x, y)).rgb

vec3 res2x(vec3 pre2, vec3 pre1, vec3 px, vec3 pos1, vec3 pos2)
{
    vec3 t, m;
    mat4x3 pre = mat4x3(pre2, pre1,   px, pos1);
    mat4x3 pos = mat4x3(pre1,   px, pos1, pos2);
    mat4x3  df = pos - pre;

    m = 0.5 - abs(px - 0.5);
    m = params.REVERSEAA_SHARPNESS * min(m, min(abs(df[1]), abs(df[2])));
    t = (7 * (df[1] + df[2]) - 3 * (df[0] + df[3])) / 16;
    t = clamp(t, -m, m);

    return t;
}

void main()
{
   vec2 pos = fract(vTexCoord * params.SourceSize.xy) - vec2(0.5, 0.5);
   vec2 coord = vTexCoord - pos * params.SourceSize.zw;

   vec3 E = P( 0, 0);
   vec3 _tx = res2x(P(-2,0), P(-1,0), E, P(1,0), P(2,0)) * pos.x;
   vec3 _ty = res2x(P(0,-2), P(0,-1), E, P(0,1), P(0,2)) * pos.y;
   vec3 res = clamp(E + 2.0*(_tx + _ty), 0.0, 1.0);

   FragColor = vec4(res, 1.0);
}
