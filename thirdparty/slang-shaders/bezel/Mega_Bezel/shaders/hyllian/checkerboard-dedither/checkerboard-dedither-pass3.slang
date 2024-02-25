#version 450

/*
   Checkerboard Dedither - pass3
   
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
    float CD_MITIG_NEIGHBRS;
    float CD_MITIG_LINES;
    float CD_ADJUST_VIEW;
	float HSM_DEDITHER_MODE;
} params;

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

#pragma parameter CD_MITIG_NEIGHBRS            "          Hyllian CB - Reduce Neighbor Errors" 4.0  0.0 4.0  1.0
// CD_MITIG_NEIGHBRS default 1, changed to 4
#pragma parameter CD_MITIG_LINES               "          Hyllian CB - Reduce Region Errors"   1.0  0.0 1.0  1.0
// CD_MITIG_LINES default 0, changed to 1
#pragma parameter CD_ADJUST_VIEW               "          Hyllian CB - Debug View"                0.0  0.0 1.0  1.0
// #pragma parameter UseGamma                  "          Gamma Slider"                1.0  0.0 1.0  0.1

#define CD_MITIG_NEIGHBRS   params.CD_MITIG_NEIGHBRS
#define CD_MITIG_LINES      params.CD_MITIG_LINES
#define CD_ADJUST_VIEW      params.CD_ADJUST_VIEW
// #define UseGamma            params.UseGamma
// #define OuputGamma          (UseGamma+1.0)

// #define GAMMA_OUT(color)    pow(color, vec3(1.0 / OuputGamma, 1.0 / OuputGamma, 1.0 / OuputGamma))

#define PATTERN(A)          step(Delta,A)

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
	if (HSM_DEDITHER_MODE < 2.5 || HSM_DEDITHER_MODE > 4.5)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

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

    vec4 L3 =  texture(Source, vTexCoord -3.0*dx    );
    vec4 R3 =  texture(Source, vTexCoord +3.0*dx    );
    vec4 U3 =  texture(Source, vTexCoord     -3.0*dy);
    vec4 D3 =  texture(Source, vTexCoord     +3.0*dy);

    vec3 color =  C.rgb;

    vec3 oriC = texture(LinearGamma, vTexCoord        ).rgb;

    float count   = 0.0;
    float count2  = 0.0;
    float count3  = 0.0;

//              U3
//              U2     
//          UL   U  UR     
//  L3  L2   L   C   R   R2  R3
//          DL   D  DR     
//              D2     
//              D3

    count += PATTERN(L.a);
    count += PATTERN(R.a);
    count += PATTERN(U.a);
    count += PATTERN(D.a);
    count += PATTERN(UL.a*UR.a*DL.a*DR.a);

    count2 += PATTERN(L.a*UL.a*U.a + U.a*UR.a*R.a + R.a*DR.a*D.a + D.a*DL.a*L.a);

    count3 += PATTERN(L3.a*L2.a*L.a);
    count3 += PATTERN(L2.a*L.a*R.a);
    count3 += PATTERN(L.a*R.a*R2.a);
    count3 += PATTERN(R.a*R2.a*R3.a);

    count3 += PATTERN(U3.a*U2.a*U.a);
    count3 += PATTERN(U2.a*U.a*D.a);
    count3 += PATTERN(U.a*D.a*D2.a);
    count3 += PATTERN(D.a*D2.a*D3.a);

    if ((count < CD_MITIG_NEIGHBRS)  && (count2 < 1.0))
        color = oriC;
    else if ((CD_MITIG_LINES == 1.0) && (count3 < 1.0))
        color = oriC;

    float luma_diff = abs(dot(oriC, Y)-dot(color, Y));

    color = mix(color, vec3(luma_diff), CD_ADJUST_VIEW);

	// FragColor = vec4(GAMMA_OUT(color), 1.0);
	FragColor = vec4(color, 1.0);
}
