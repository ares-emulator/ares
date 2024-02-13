#version 450

//	DS Hybrid View
//	by hunterk
//	additional modifications by Dranyth
//	license: public domain
//
//	This shader requires 16:9 aspect ratio forced in Video > Scaling
//	and integer scaling OFF
//	Set the Screen Layout to top/bottom
//	A Screen Gap value can be set in the emulator to prevent
//	effects from bleeding across the division of the two screens
//	The Screen Gap itself will be compensated for and removed from the final output

layout(push_constant) uniform Push
{
	vec4 ds_hybrid_view_refpassSize;
	vec4 OutputSize;
	float screen_toggle;
	float show_both_screens;
	float filter_small;
} params;

#pragma parameter bogus_ds_hybrid_view "[ DS Hybrid View Settings ]:" 0.0 0.0 0.0 0.0
#pragma parameter screen_toggle "          Large Screen Toggle" 0.0 0.0 1.0 1.0
#pragma parameter show_both_screens "          Show Both Small Screens" 1.0 0.0 1.0 1.0
#pragma parameter filter_small "          Filter Small Screens" 1.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float ScaledScreenGap;
layout(location = 2) out float HalfScaledScreenGap;
layout(location = 3) out float VerticalAlignmentAdjustment;

void main()
{
	gl_Position = (global.MVP * Position);
	// screen gap setting increases fb size; modulo at original res to compensate
	float screen_gap = mod(params.ds_hybrid_view_refpassSize.y, 192.0);
	ScaledScreenGap = screen_gap * params.ds_hybrid_view_refpassSize.w;
	HalfScaledScreenGap = (screen_gap * 0.5) * params.ds_hybrid_view_refpassSize.w;
	VerticalAlignmentAdjustment = -0.25 + ((screen_gap * 0.5) * params.OutputSize.w);
	vTexCoord = TexCoord * 1.00001;
	vTexCoord *= vec2(1.333, 0.5 - HalfScaledScreenGap);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float ScaledScreenGap;
layout(location = 2) in float HalfScaledScreenGap;
layout(location = 3) in float VerticalAlignmentAdjustment;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D ds_hybrid_view_refpass;
layout(set = 0, binding = 3) uniform sampler2D Source;

vec2 ignoreScreenGap(vec2 coord)
{
	return (coord.y > 0.5 - HalfScaledScreenGap) ? coord + vec2(0., ScaledScreenGap) : coord;
}

float showWhichScreens(float yCoord)
{
	return (params.show_both_screens > 0.5) ? 1. : ((params.screen_toggle > 0.5) ? ceil((0.5 - HalfScaledScreenGap) - yCoord) : floor((0.5 + HalfScaledScreenGap) + yCoord));
}

void main()
{
	vec2 bigCoord = vTexCoord + vec2(0., (params.screen_toggle > 0.5) ? 0.5 + HalfScaledScreenGap : 0.);
	vec2 smallCoord = vTexCoord * vec2(3.) + vec2(-3., VerticalAlignmentAdjustment);
	FragColor = texture(Source, bigCoord);
	FragColor += ((params.filter_small > 0.5) ? texture(Source, ignoreScreenGap(smallCoord)) : texture(ds_hybrid_view_refpass, ignoreScreenGap(smallCoord))) * vec4(showWhichScreens(smallCoord.y));
}
