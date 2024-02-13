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
 
 float4 GetCompositeYIQ(sampler2D tex, float2 TexCoord, float2 size)
{
	float2 PValueSourceTexel = float2(PValue / size.x, 0.0f);

	float2 C0 = TexCoord + PValueSourceTexel * OffsetX.x;
	float2 C1 = TexCoord + PValueSourceTexel * OffsetX.y;
	float2 C2 = TexCoord + PValueSourceTexel * OffsetX.z;
	float2 C3 = TexCoord + PValueSourceTexel * OffsetX.w;
	float4 Cx = float4(C0.x, C1.x, C2.x, C3.x);
	float4 Cy = float4(C0.y, C1.y, C2.y, C3.y);
	float4 Texel0 = texture(tex, C0);
	float4 Texel1 = texture(tex, C1);
	float4 Texel2 = texture(tex, C2);
	float4 Texel3 = texture(tex, C3);

	float4 HPosition = Cx;
	float4 VPosition = Cy;

	float4 Y = float4(dot(Texel0, YDot), dot(Texel1, YDot), dot(Texel2, YDot), dot(Texel3, YDot));
	float4 I = float4(dot(Texel0, IDot), dot(Texel1, IDot), dot(Texel2, IDot), dot(Texel3, IDot));
	float4 Q = float4(dot(Texel0, QDot), dot(Texel1, QDot), dot(Texel2, QDot), dot(Texel3, QDot));

	float W = PI2 * CCValue * ScanTime;
	float WoPI = W / PI;

	float HOffset = (BValue + SignalOffset) / WoPI;
	float VScale = (AValue * size.y) / WoPI;

	float4 T = HPosition + HOffset + VPosition * VScale;
	float4 TW = T * W;

	float4 CompositeYIQ = Y + I * cos(TW) + Q * sin(TW);

	return CompositeYIQ;
}

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
   FragColor = GetCompositeYIQ(Source, vTexCoord, params.SourceSize.xy);
}