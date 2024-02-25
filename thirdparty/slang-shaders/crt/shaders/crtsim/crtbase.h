layout(push_constant) uniform Push
{
	float CRTMask_Scale;
//	float CRTMask_Offset_X;
//	float CRTMask_Offset_Y;
//	float Tuning_Overscan;
//	float Tuning_Dimming;
	float Tuning_Satur;
//	float Tuning_ReflScalar;
//	float Tuning_Barrel;
	float Tuning_Mask_Brightness;
	float Tuning_Mask_Opacity;
//	float Tuning_Diff_Brightness;
//	float Tuning_Spec_Brightness;
//	float Tuning_Spec_Power;
//	float Tuning_Fres_Brightness;
//	float Tuning_LightPos_R;
//	float Tuning_LightPos_G;
//	float Tuning_LightPos_B;
} params;

#pragma parameter CRTMask_Scale "CRT Mask Scale" 1.0 0.0 10.0 0.5
//#pragma parameter CRTMask_Offset_X "CRT Mask Offset X" 0.0 0.0 1.0 0.05
//#pragma parameter CRTMask_Offset_Y "CRT Mask Offset Y" 0.0 0.0 1.0 0.05
//#pragma parameter Tuning_Overscan "Overscan" 0.95 0.0 1.0 0.05
//#pragma parameter Tuning_Dimming "Dimming" 0.0 0.0 1.0 0.05
#pragma parameter Tuning_Satur "Saturation" 1.0 0.0 1.0 0.05
//#pragma parameter Tuning_ReflScalar "Reflection" 0.0 0.0 1.0 0.05
//#pragma parameter Tuning_Barrel "Barrel Distortion" 0.25 0.0 1.0 0.05
#pragma parameter Tuning_Mask_Brightness "Mask Brightness" 0.5 0.0 1.0 0.05
#pragma parameter Tuning_Mask_Opacity "Mask Opacity" 0.3 0.0 1.0 0.05
//#pragma parameter Tuning_Diff_Brightness "Diff Brightness" 0.5 0.0 1.0 0.05
//#pragma parameter Tuning_Spec_Brightness "Spec Brightness" 0.5 0.0 1.0 0.05
//#pragma parameter Tuning_Fres_Brightness "Fres Brightness" 0.5 0.0 1.0 0.05
//#pragma parameter Tuning_LightPos_R "Light Position R" 1.0 0.0 1.0 0.05
//#pragma parameter Tuning_LightPos_G "Light Position G" 1.0 0.0 1.0 0.05
//#pragma parameter Tuning_LightPos_B "Light Position B" 1.0 0.0 1.0 0.05

#define CRTMask_Offset vec2(params.CRTMask_Offset_X, params.CRTMask_Offset_Y)

half4 SampleCRT(sampler2D shadowMaskSampler, sampler2D compFrameSampler, half2 uv)
{
	half2 ScaledUV = uv;
//	ScaledUV *= UVScalar;
//	ScaledUV += UVOffset;
	
	half2 scanuv = vec2(fract(uv * global.SourceSize.xy / params.CRTMask_Scale));
	vec4 phosphor_grid;
	half3 scantex = tex2D(shadowMaskSampler, scanuv).rgb;
	
	scantex += params.Tuning_Mask_Brightness;			// adding looks better
	scantex = lerp(ivec3(1,1,1), scantex, params.Tuning_Mask_Opacity);
/*  // commenting this to move to present shader
	// Apply overscan after scanline sampling is done.
	half2 overscanuv = (ScaledUV * params.Tuning_Overscan) - ((params.Tuning_Overscan - 1.0) * 0.5);
	
	// Curve UVs for composite texture inwards to garble things a bit.
	overscanuv = overscanuv - half2(0.5,0.5);
	half rsq = (overscanuv.x*overscanuv.x) + (overscanuv.y*overscanuv.y);
	overscanuv = overscanuv + (overscanuv * (params.Tuning_Barrel * rsq)) + half2(0.5,0.5);
*/
	half2 overscanuv = uv;
	half3 comptex = tex2D(compFrameSampler, overscanuv).rgb;

	half4 emissive = half4(comptex * scantex, 1);
	half desat = dot(half4(0.299, 0.587, 0.114, 0.0), emissive);
	emissive = lerp(half4(desat,desat,desat,1), emissive, params.Tuning_Satur);
	
	return emissive;
}