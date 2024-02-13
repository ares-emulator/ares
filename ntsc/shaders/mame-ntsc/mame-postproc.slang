#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float scanlines;
	float scandark;
	float deconverge;
	float pincushion;
	float hertzroll;
} params;

#pragma parameter scanlines "Scanline Toggle" 1.0 0.0 1.0 1.0
#pragma parameter scandark "Scanline Intensity" 0.175 0.0 1.0 0.05
#pragma parameter deconverge "Deconvergence/Convolution" 1.0 0.0 1.0 1.0
#pragma parameter pincushion "Bezel Toggle" 0.0 0.0 1.0 1.0
#pragma parameter hertzroll "Refresh Roll Toggle" 1.0 0.0 1.0 1.0

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

vec4 ColorConvolution(vec2 UV, vec2 InverseRes)
{
	vec3 InPixel = texture(Source, UV).rgb;
	
	// Color Matrix
	float RedValue = dot(InPixel, RedMatrix);
	float GrnValue = dot(InPixel, GrnMatrix);
	float BluValue = dot(InPixel, BluMatrix);
	vec3 OutColor = vec3(RedValue, GrnValue, BluValue);
	
	// DC Offset & Scale
	OutColor = (OutColor * ColorScale) + DCOffset;
	
	// Saturation
	float Luma = dot(OutColor, Gray);
	vec3 Chroma = OutColor - Luma;
	OutColor = (Chroma * Saturation) + Luma;
	
	return vec4(OutColor, 1.0);
}

vec4 Deconverge(vec2 UV)
{
	vec2 InverseRes = 1.0 / params.SourceSize.xy;
	vec2 InverseSrcRes = 1.0 / params.OriginalSize.xy;

	vec3 CoordX = UV.x * RadialConvergeX;
	vec3 CoordY = UV.y * RadialConvergeY;

	CoordX += ConvergeX * InverseRes.x - (RadialConvergeX - 1.0) * 0.5;
	CoordY += ConvergeY * InverseRes.y - (RadialConvergeY - 1.0) * 0.5;

	float RedValue = ColorConvolution(vec2(CoordX.x, CoordY.x), InverseSrcRes).r;
	float GrnValue = ColorConvolution(vec2(CoordX.y, CoordY.y), InverseSrcRes).g;
	float BluValue = ColorConvolution(vec2(CoordX.z, CoordY.z), InverseSrcRes).b;

	if (params.deconverge > 0.5) return vec4(RedValue, GrnValue, BluValue, 1.0);
	else return vec4(texture(Source, UV));
}

vec4 ScanlinePincushion(vec2 UV)
{
	vec4 InTexel = Deconverge(UV);
	
	vec2 PinUnitCoord = UV * Two.xy - One.xy;
	float PincushionR2 = pow(length(PinUnitCoord), 2.0);
	vec2 PincushionCurve = PinUnitCoord * PincushionAmount * PincushionR2;
	vec2 BaseCoord = UV;
	vec2 ScanCoord = UV;
	
	BaseCoord *= One.xy - PincushionAmount * 0.2; // Warning: Magic constant
	BaseCoord += PincushionAmount * 0.1;
	BaseCoord += PincushionCurve;
	
	ScanCoord *= One.xy - PincushionAmount * 0.2; // Warning: Magic constant
	ScanCoord += PincushionAmount * 0.1;
	ScanCoord += PincushionCurve;
	
	vec2 CurveClipUnitCoord = UV * Two.xy - One.xy;
	float CurvatureClipR2 = pow(length(CurveClipUnitCoord), 2.0);
	vec2 CurvatureClipCurve = CurveClipUnitCoord * CurvatureAmount * CurvatureClipR2;
	vec2 ScreenClipCoord = UV;
	ScreenClipCoord -= Half.xy;
	ScreenClipCoord *= One.xy - CurvatureAmount * 0.2; // Warning: Magic constant
	ScreenClipCoord += Half.xy;
	ScreenClipCoord += CurvatureClipCurve;

	if (params.pincushion > 0.5){
		// -- Alpha Clipping --
		if (BaseCoord.x < 0.0) return vec4(0.0, 0.0, 0.0, 1.0);
		if (BaseCoord.y < 0.0) return vec4(0.0, 0.0, 0.0, 1.0);
		if (BaseCoord.x > 1.0) return vec4(0.0, 0.0, 0.0, 1.0);
		if (BaseCoord.y > 1.0) return vec4(0.0, 0.0, 0.0, 1.0);
	}
	
	// -- Scanline Simulation --
	float InnerSine = ScanCoord.y * params.OriginalSize.y * ScanlineScale;
	float ScanBrightMod = sin(InnerSine * Pi + ScanlineOffset * params.OriginalSize.y);
	float ScanBrightness = mix(1.0, (pow(ScanBrightMod * ScanBrightMod, ScanlineHeight) * ScanlineBrightScale + 1.0) * 0.5, params.scandark);
	vec3 ScanlineTexel = InTexel.rgb * ScanBrightness;
	
	// -- Color Compression (increasing the floor of the signal without affecting the ceiling) --
	ScanlineTexel = Floor + (One.xyz - Floor) * ScanlineTexel;
	if (params.scanlines > 0.5)	return vec4(ScanlineTexel, 1.0);
	else return vec4(InTexel);
}

vec4 SixtyHertz(vec2 UV)
{
	vec4 InPixel = ScanlinePincushion(UV);
	float Milliseconds = float(params.FrameCount) * 15.0;
	float TimeStep = fract(Milliseconds * SixtyHertzRate);
	float BarPosition = 1.0 - fract(UV.y + TimeStep) * SixtyHertzScale;
	vec4 OutPixel = InPixel * BarPosition;
	if (params.hertzroll > 0.5) return OutPixel;
	else return InPixel;
}

void main()
{
	vec4 OutPixel = SixtyHertz(vTexCoord.xy);
	FragColor = OutPixel;
}