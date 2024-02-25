#version 450

/*
   Checkerboard Dedither - pass2
   
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
    float CD_BLEND_LEVEL;
} params;

#pragma parameter CD_BLEND_LEVEL               "    Blend Level"                1.0  0.0 1.0  0.1

#define CD_BLEND_LEVEL             params.CD_BLEND_LEVEL

#define PATTERN(A)                 step(Delta,A)

const float Delta = 0.000000001;
const vec3 Y = vec3( 0.299,  0.587,  0.114);

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
layout(set = 0, binding = 3) uniform sampler2D LinearGamma;

void main()
{
    vec2 dx = vec2(1.0, 0.0)*params.SourceSize.zw;
    vec2 dy = vec2(0.0, 1.0)*params.SourceSize.zw;

    // Reading the texels.
    vec4 C =  texture(Source, vTexCoord        );
    vec4 L =  texture(Source, vTexCoord -dx    );
    vec4 R =  texture(Source, vTexCoord +dx    );
    vec4 U =  texture(Source, vTexCoord     -dy);
    vec4 D =  texture(Source, vTexCoord     +dy);
    vec4 UL = texture(Source, vTexCoord -dx -dy);
    vec4 UR = texture(Source, vTexCoord +dx -dy);
    vec4 DL = texture(Source, vTexCoord -dx +dy);
    vec4 DR = texture(Source, vTexCoord +dx +dy);

    vec4 L2 =  texture(Source, vTexCoord -2.0*dx    );
    vec4 R2 =  texture(Source, vTexCoord +2.0*dx    );
    vec4 U2 =  texture(Source, vTexCoord     -2.0*dy);
    vec4 D2 =  texture(Source, vTexCoord     +2.0*dy);

    vec4 UL2 =  texture(Source, vTexCoord -2.0*dx -dy);
    vec4 UR2 =  texture(Source, vTexCoord +2.0*dx -dy);
    vec4 DL2 =  texture(Source, vTexCoord -2.0*dx +dy);
    vec4 DR2 =  texture(Source, vTexCoord +2.0*dx +dy);

    vec4 LU2 =  texture(Source, vTexCoord -dx -2.0*dy);
    vec4 RU2 =  texture(Source, vTexCoord +dx -2.0*dy);
    vec4 LD2 =  texture(Source, vTexCoord -dx +2.0*dy);
    vec4 RD2 =  texture(Source, vTexCoord +dx -2.0*dy);

    vec3 color =  C.rgb;

    vec3 oriC = texture(LinearGamma, vTexCoord        ).rgb;
    vec3 oriL = texture(LinearGamma, vTexCoord -dx    ).rgb;
    vec3 oriR = texture(LinearGamma, vTexCoord +dx    ).rgb;
    vec3 oriU = texture(LinearGamma, vTexCoord     -dy).rgb;
    vec3 oriD = texture(LinearGamma, vTexCoord     +dy).rgb;

    float count1 = 0.0;
    float count2 = 0.0;

    float diff  = (1.0 - CD_BLEND_LEVEL);

//      LU2  U2  RU2
//  UL2  UL   U  UR  UR2
//   L2   L   C   R   R2
//  DL2  DL   D  DR  DR2
//      LD2  D2  RD2

    count1 += PATTERN(L.a*D.a*R.a);
    count1 += PATTERN(L.a*U.a*R.a);
    count1 += PATTERN(L.a*UL.a*R.a*UR.a + L.a*DL.a*R.a*DR.a);

    count1 += PATTERN(U.a*D2.a*(UL.a*LD2.a + UR.a*RD2.a) + U2.a*D.a*(LU2.a*DL.a + RU2.a*DR.a));

    count2 += PATTERN(U.a*L.a*D.a);
    count2 += PATTERN(U.a*R.a*D.a);
    count2 += PATTERN(U.a*UR.a*D.a*DR.a + D.a*DL.a*U.a*UL.a);

    count2 += PATTERN(L.a*R2.a*(DL.a*DR2.a + UL.a*UR2.a) + L2.a*R.a*(DL2.a*DR.a + UL2.a*UR.a));

    if ((count1 * count2) > 0.0 && count1 == count2)
        color = 0.5*(1.0+diff)*oriC + 0.125*(1.0-diff)*(oriL + oriR + oriU + oriD);
    else if (count1 > 0.0 && count1 > count2)
        color = 0.5*(1.0+diff)*oriC + 0.25*(1.0-diff)*(oriL + oriR);
    else if (count2 > 0.0 && count2 > count1)
        color = 0.5*(1.0+diff)*oriC + 0.25*(1.0-diff)*(oriU + oriD);

    float luma_diff = abs(dot(oriC, Y)-dot(color, Y));

    FragColor = vec4(color, luma_diff);
}
