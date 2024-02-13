#version 450

/*
   Sgenpt-Mix - pass4
   
   2011-2022 Hyllian - sergiogdb@gmail.com

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
    float SGPT_BLEND_OPTION;
    float VL_BLEND_LEVEL;
} params;

#pragma parameter SGPT_BLEND_OPTION    "SGENPT-MIX: [ OFF | CB | VL | BOTH ]"               3.0  0.0 3.0  1.0
#pragma parameter VL_BLEND_LEVEL       "    Vertical Lines Blend Level"                     0.8  0.0 1.0  0.1

#define SGPT_BLEND_OPTION             params.SGPT_BLEND_OPTION
#define VL_BLEND_LEVEL                params.VL_BLEND_LEVEL

const vec3 Y = vec3( 0.299,  0.587,  0.114);

vec3 min_s(vec3 central, vec3 adj1, vec3 adj2) {return min(central, max(adj1, adj2));}
vec3 max_s(vec3 central, vec3 adj1, vec3 adj2) {return max(central, min(adj1, adj2));}

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

void main()
{
    vec2 dx = vec2(1.0, 0.0)*params.SourceSize.zw;
    vec2 dy = vec2(0.0, 1.0)*params.SourceSize.zw;

    // Reading the texels.
    vec3 C  = texture(Source, vTexCoord    ).xyz;
    vec3 L  = texture(Source, vTexCoord -dx).xyz;
    vec3 R  = texture(Source, vTexCoord +dx).xyz;
    vec3 U  = texture(Source, vTexCoord -dy).xyz;
    vec3 D  = texture(Source, vTexCoord +dy).xyz;
    vec3 UL = texture(Source, vTexCoord -dx -dy).xyz;
    vec3 UR = texture(Source, vTexCoord +dx -dy).xyz;
    vec3 DL = texture(Source, vTexCoord -dx +dy).xyz;
    vec3 DR = texture(Source, vTexCoord +dx +dy).xyz;

    vec3 color = C;

    if (SGPT_BLEND_OPTION == 2 || SGPT_BLEND_OPTION == 3)
    {
        float diff = dot(max(C, max(L, R)) - min(C, min(L, R)), Y);

        diff *= (1.0 - VL_BLEND_LEVEL);

        vec3 min_sample = max_s(min_s(C, L, R), min_s(C, DL, DR), min_s(C, UL, UR));
        vec3 max_sample = min_s(max_s(C, L, R), max_s(C, DL, DR), max_s(C, UL, UR));

        color = 0.5*( 1.0 + diff )*C + 0.25*( 1.0 - diff )*(L + R);

        color = clamp(color, min_sample, max_sample);
    }

    float luma_diff = abs(dot(C, Y)-dot(color, Y));

    FragColor = vec4(color, luma_diff);
}
