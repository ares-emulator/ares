layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 scalefx_pass0Size;
	float HSM_SCALEFX_ON;
	float SFX_CLR;
	float SFX_SAA;
	float SFX_SCN;
	float SFX_RAA;
	// float HSM_SCALEFX_SOURCE_RES_DIVISION_FACTOR;
} params;

#pragma parameter HSM_SCALEFX_EMPTY_LINE		" " 0 0 0.001 0.001
#pragma parameter HSM_SCALEFX_TITLE	            "[ --- SMOOTHING - SCALEFX ---- ]:" 0 0 0.01 0.01

#pragma parameter HSM_SCALEFX_ON 	            "          ScaleFx ON - Must Increase Core Res Sampling" 0 0 1 1
#define HSM_SCALEFX_ON params.HSM_SCALEFX_ON

#pragma parameter SFX_CLR 						"          ScaleFX Threshold" 0.43 0.01 1.00 0.01
#define SFX_CLR params.SFX_CLR

#pragma parameter SFX_SAA 						"          ScaleFX Filter AA" 1.00 0.00 1.00 1.00
#define SFX_SAA params.SFX_SAA

#pragma parameter SFX_SCN 						"          ScaleFX Filter Corners" 0 0.0 1.0 1.0
#define SFX_SCN params.SFX_SCN

vec4 HSM_TextureOffset(in sampler2D in_sampler, vec2 in_coord, vec2 pixel_offset, vec2 texture_size)
{
	vec2 uv_offset = pixel_offset / params.SourceSize.xy;
	return texture(in_sampler, in_coord + uv_offset);
}
