#version 450

// This is a port of the NTSC encode/decode shader pair in MAME and MESS, modified to use only
// one pass rather than an encode pass and a decode pass. It accurately emulates the sort of
// signal decimation one would see when viewing a composite signal, though it could benefit from a
// pre-pass to re-size the input content to more accurately reflect the actual size that would
// be incoming from a composite signal source.
//
// To encode the composite signal, I convert the RGB value to YIQ, then subsequently evaluate
// the standard NTSC composite equation. Four composite samples per RGB pixel are generated from
// the incoming linearly-interpolated texels.
//
// The decode pass implements a Fixed Impulse Response (FIR) filter designed by MAME/MESS contributor
// "austere" in matlab (if memory serves correctly) to mimic the behavior of a standard television set
// as closely as possible. The filter window is 83 composite samples wide, and there is an additional
// notch filter pass on the luminance (Y) values in order to strip the color signal from the luminance
// signal prior to processing.
//
// - UltraMoogleMan [8/2/2013]

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

#define float2 vec2
#define float3 vec3
#define float4 vec4

//-----------------------------------------------------------------------------
// NTSC Pixel Shader
//-----------------------------------------------------------------------------

const float AValue = 0.5f;
const float BValue = 0.5f;
const float CCValue = 3.5795454f;
const float OValue = 0.0f;
const float PValue = 1.0f;
const float ScanTime = 52.6f;

const float NotchHalfWidth = 1.0f;
const float YFreqResponse = 6.0f;
const float IFreqResponse = 1.2f;
const float QFreqResponse = 0.6f;

const float SignalOffset = 0.0f;

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

 const float PI = 3.1415927f;
 const float PI2 = 6.2830854f;

 const float4 YDot = float4(0.299f, 0.587f, 0.114f, 0.0f);
 const float4 IDot = float4(0.595716f, -0.274453f, -0.321263f, 0.0f);
 const float4 QDot = float4(0.211456f, -0.522591f, 0.311135f, 0.0f);

 const float3 RDot = float3(1.0f, 0.956f, 0.621f);
 const float3 GDot = float3(1.0f, -0.272f, -0.647f);
 const float3 BDot = float3(1.0f, -1.106f, 1.703f);

 const float4 OffsetX = float4(0.0f, 0.25f, 0.50f, 0.75f);
 const float4 NotchOffset = float4(0.0f, 1.0f, 2.0f, 3.0f);

 const int SampleCount = 64;
 const int HalfSampleCount = 32;

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
layout(set = 0, binding = 3) uniform sampler2D Original;

void main()
{
	float4 BaseTexel = texture(Original, vTexCoord);
	
	vec2 size = params.SourceSize.xy;//(params.SourceSize.y > 400.0) ? params.SourceSize.xy : params.SourceSize.xy * vec2(1.0, 0.5);

	float TimePerSample = ScanTime / (size.x * 4.0f);

	float Fc_y1 = (CCValue - NotchHalfWidth) * TimePerSample;
	float Fc_y2 = (CCValue + NotchHalfWidth) * TimePerSample;
	float Fc_y3 = YFreqResponse * TimePerSample;
	float Fc_i = IFreqResponse * TimePerSample;
	float Fc_q = QFreqResponse * TimePerSample;
	float Fc_i_2 = Fc_i * 2.0f;
	float Fc_q_2 = Fc_q * 2.0f;
	float Fc_y1_2 = Fc_y1 * 2.0f;
	float Fc_y2_2 = Fc_y2 * 2.0f;
	float Fc_y3_2 = Fc_y3 * 2.0f;
	float Fc_i_pi2 = Fc_i * PI2;
	float Fc_q_pi2 = Fc_q * PI2;
	float Fc_y1_pi2 = Fc_y1 * PI2;
	float Fc_y2_pi2 = Fc_y2 * PI2;
	float Fc_y3_pi2 = Fc_y3 * PI2;
	float PI2Length = PI2 / SampleCount;

	float W = PI2 * CCValue * ScanTime;
	float WoPI = W / PI;

	float HOffset = (BValue + SignalOffset) / WoPI;
	float VScale = (AValue * size.y) / WoPI;

	float4 YAccum = vec4(0.0);
	float4 IAccum = vec4(0.0);
	float4 QAccum = vec4(0.0);

	float4 Cy = vec4(vTexCoord.y);
	float4 VPosition = Cy;

	for (float i = 0; i < SampleCount; i += 4.0f)
	{
		float n = i - HalfSampleCount;
		float4 n4 = n + NotchOffset;

		float4 Cx = vTexCoord.x + (n4 * 0.25f) / size.x;
		float4 HPosition = Cx;

		float4 C = texture(Source, float2(Cx.r, Cy.r));

		float4 T = HPosition + HOffset + VPosition * VScale;
		float4 WT = W * T + OValue;

		float4 SincKernel = 0.54f + 0.46f * cos(PI2Length * n4);

		float4 SincYIn1 = Fc_y1_pi2 * n4;
		float4 SincYIn2 = Fc_y2_pi2 * n4;
		float4 SincYIn3 = Fc_y3_pi2 * n4;
		float4 SincIIn = Fc_i_pi2 * n4;
		float4 SincQIn = Fc_q_pi2 * n4;

		float4 SincY1 = (notEqual(SincYIn1 , vec4(0.0)) == bvec4(true)) ? sin(SincYIn1) / SincYIn1 : vec4(0.5);
		float4 SincY2 = (notEqual(SincYIn2 , vec4(0.0)) == bvec4(true)) ? sin(SincYIn2) / SincYIn2 : vec4(0.5);
		float4 SincY3 = (notEqual(SincYIn3 , vec4(0.0)) == bvec4(true)) ? sin(SincYIn3) / SincYIn3 : vec4(0.5);

		float4 IdealY = (Fc_y1_2 * SincY1 - Fc_y2_2 * SincY2) + Fc_y3_2 * SincY3;
		float4 IdealI = Fc_i_2 * ((notEqual(SincIIn , vec4(0.0)) == bvec4(true)) ? sin(SincIIn) / SincIIn : vec4(0.5));
		float4 IdealQ = Fc_q_2 * ((notEqual(SincQIn , vec4(0.0)) == bvec4(true)) ? sin(SincQIn) / SincQIn : vec4(0.5));

		float4 FilterY = SincKernel * IdealY;
		float4 FilterI = SincKernel * IdealI;
		float4 FilterQ = SincKernel * IdealQ;

		YAccum = YAccum + C * FilterY;
		IAccum = IAccum + C * cos(WT) * FilterI;
		QAccum = QAccum + C * sin(WT) * FilterQ;
	}

	float3 YIQ = float3(
		(YAccum.r + YAccum.g + YAccum.b + YAccum.a),
		(IAccum.r + IAccum.g + IAccum.b + IAccum.a) * 2.0f,
		(QAccum.r + QAccum.g + QAccum.b + QAccum.a) * 2.0f);

	float3 RGB = float3(
		dot(YIQ, RDot),
		dot(YIQ, GDot),
		dot(YIQ, BDot));
   FragColor = vec4(RGB, BaseTexel.a);
}