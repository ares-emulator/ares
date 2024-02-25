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
// Yes, this code could greatly use some cleaning up.

// ported from UltraMoogleMan's "Full MAME/MESS Shader Pipe" shadertoy: https://www.shadertoy.com/view/ldf3Rf
// license: presumably MAME's license at the time, which was noncommercial

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#include "constants.inc"

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

#define iChannel0 Source

vec4 CompositeSample(vec2 UV, vec2 InverseRes) {
	vec2 InverseP = vec2(P, 0.0) * InverseRes;
	
	// UVs for four linearly-interpolated samples spaced 0.25 texels apart
	vec2 C0 = UV;
	vec2 C1 = UV + InverseP * 0.25;
	vec2 C2 = UV + InverseP * 0.50;
	vec2 C3 = UV + InverseP * 0.75;
	vec4 Cx = vec4(C0.x, C1.x, C2.x, C3.x);
	vec4 Cy = vec4(C0.y, C1.y, C2.y, C3.y);

	vec4 Texel0 = texture(iChannel0, C0);
	vec4 Texel1 = texture(iChannel0, C1);
	vec4 Texel2 = texture(iChannel0, C2);
	vec4 Texel3 = texture(iChannel0, C3);
	
	float Frequency = CCFrequency;
	//Frequency = Frequency;// Uncomment for bad color sync + (sin(UV.y * 2.0 - 1.0) / CCFrequency) * 0.001;

	// Calculated the expected time of the sample.
	vec4 T = A2 * Cy * vec4(params.SourceSize.y) + B + Cx;
	vec4 W = vec4(Pi2ScanTime * Frequency);
	vec4 TW = T * W;
	vec4 Y = vec4(dot(Texel0, YTransform), dot(Texel1, YTransform), dot(Texel2, YTransform), dot(Texel3, YTransform));
	vec4 I = vec4(dot(Texel0, ITransform), dot(Texel1, ITransform), dot(Texel2, ITransform), dot(Texel3, ITransform));
	vec4 Q = vec4(dot(Texel0, QTransform), dot(Texel1, QTransform), dot(Texel2, QTransform), dot(Texel3, QTransform));

	vec4 Encoded = Y + I * cos(TW) + Q * sin(TW);
	return (Encoded - MinC) * InvCRange;
}

vec4 NTSCCodec(vec2 UV, vec2 InverseRes)
{
	vec4 YAccum = Zero;
	vec4 IAccum = Zero;
	vec4 QAccum = Zero;
	float QuadXSize = params.SourceSize.x * 4.0;
	float TimePerSample = ScanTime / QuadXSize;
	
	// Frequency cutoffs for the individual portions of the signal that we extract.
	// Y1 and Y2 are the positive and negative frequency limits of the notch filter on Y.
	// Y3 is the center of the frequency response of the Y filter.
	// I is the center of the frequency response of the I filter.
	// Q is the center of the frequency response of the Q filter.
	float Fc_y1 = NotchLowerFrequency * TimePerSample;
	float Fc_y2 = NotchUpperFrequency * TimePerSample;
	float Fc_y3 = YFrequency * TimePerSample;
	float Fc_i = IFrequency * TimePerSample;
	float Fc_q = QFrequency * TimePerSample;
	float Pi2Fc_y1 = Fc_y1 * Pi2;
	float Pi2Fc_y2 = Fc_y2 * Pi2;
	float Pi2Fc_y3 = Fc_y3 * Pi2;
	float Pi2Fc_i = Fc_i * Pi2;
	float Pi2Fc_q = Fc_q * Pi2;
	float Fc_y1_2 = Fc_y1 * 2.0;
	float Fc_y2_2 = Fc_y2 * 2.0;
	float Fc_y3_2 = Fc_y3 * 2.0;
	float Fc_i_2 = Fc_i * 2.0;
	float Fc_q_2 = Fc_q * 2.0;
	vec4 CoordY = vec4(UV.y);
	
	// 83 composite samples wide, 4 composite pixels per texel
	for(float n = -31.0; n < 32.0; n += 4.0)
	{
		vec4 n4 = n + NotchOffset;
		vec4 CoordX = UV.x + InverseRes.x * n4 * 0.25;
		vec2 TexCoord = vec2(CoordX.x, CoordY.x);
		vec4 C = CompositeSample(TexCoord, InverseRes) * CRange + MinC;
		vec4 WT = W * (CoordX  + A2 * CoordY * params.SourceSize.y + B);
		vec4 Cosine = 0.54 + 0.46 * cos(Pi2Length * n4);

		vec4 SincYIn1 = Pi2Fc_y1 * n4;
		vec4 SincYIn2 = Pi2Fc_y2 * n4;
		vec4 SincYIn3 = Pi2Fc_y3 * n4;
		vec4 SincY1 = sin(SincYIn1) / SincYIn1;
		vec4 SincY2 = sin(SincYIn2) / SincYIn2;
		vec4 SincY3 = sin(SincYIn3) / SincYIn3;
		
		// These zero-checks could be made more efficient if WebGL supported mix(vec4, vec4, bvec4)
		// Unfortunately, the universe hates us
		if(SincYIn1.x == 0.0) SincY1.x = 1.0;
		if(SincYIn1.y == 0.0) SincY1.y = 1.0;
		if(SincYIn1.z == 0.0) SincY1.z = 1.0;
		if(SincYIn1.w == 0.0) SincY1.w = 1.0;
		if(SincYIn2.x == 0.0) SincY2.x = 1.0;
		if(SincYIn2.y == 0.0) SincY2.y = 1.0;
		if(SincYIn2.z == 0.0) SincY2.z = 1.0;
		if(SincYIn2.w == 0.0) SincY2.w = 1.0;
		if(SincYIn3.x == 0.0) SincY3.x = 1.0;
		if(SincYIn3.y == 0.0) SincY3.y = 1.0;
		if(SincYIn3.z == 0.0) SincY3.z = 1.0;
		if(SincYIn3.w == 0.0) SincY3.w = 1.0;
		vec4 IdealY = (Fc_y1_2 * SincY1 - Fc_y2_2 * SincY2) + Fc_y3_2 * SincY3;
		vec4 FilterY = Cosine * IdealY;		
		
		vec4 SincIIn = Pi2Fc_i * n4;
		vec4 SincI = sin(SincIIn) / SincIIn;
		if (SincIIn.x == 0.0) SincI.x = 1.0;
		if (SincIIn.y == 0.0) SincI.y = 1.0;
		if (SincIIn.z == 0.0) SincI.z = 1.0;
		if (SincIIn.w == 0.0) SincI.w = 1.0;
		vec4 IdealI = Fc_i_2 * SincI;
		vec4 FilterI = Cosine * IdealI;
		
		vec4 SincQIn = Pi2Fc_q * n4;
		vec4 SincQ = sin(SincQIn) / SincQIn;
		if (SincQIn.x == 0.0) SincQ.x = 1.0;
		if (SincQIn.y == 0.0) SincQ.y = 1.0;
		if (SincQIn.z == 0.0) SincQ.z = 1.0;
		if (SincQIn.w == 0.0) SincQ.w = 1.0;
		vec4 IdealQ = Fc_q_2 * SincQ;
		vec4 FilterQ = Cosine * IdealQ;
		
		YAccum += C * FilterY;
		IAccum += C * cos(WT) * FilterI;
		QAccum += C * sin(WT) * FilterQ;
	}
	
	float Y = dot(YAccum, One);
	float I = dot(IAccum, One) * 2.0;
	float Q = dot(QAccum, One) * 2.0;
	
	vec3 YIQ = vec3(Y, I, Q);
	vec3 OutRGB = vec3(dot(YIQ, YIQ2R), dot(YIQ, YIQ2G), dot(YIQ, YIQ2B));
	
	return vec4(OutRGB, 1.0);
}

void main()
{
   FragColor = vec4(NTSCCodec(vTexCoord, params.SourceSize.zw));
}