#version 450

/*
	print-resolution-text-apply.slang
	Takes the output of the text generate pass and applies it on top of the image
*/ 

#pragma name TextApplyPass

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float PRINT_RESOLUTION_TEXT_ON;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter PRINT_RESOLUTION_TEXT_TITLE								"[ PRINT RESOLUTION TEXT ]" 0 0 0.001 0.001

#pragma parameter PRINT_RESOLUTION_TEXT_ON							"          Show Resolution Text" 1 0 1 1
float PRINT_RESOLUTION_TEXT_ON = params.PRINT_RESOLUTION_TEXT_ON;

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
layout(set = 0, binding = 1) uniform sampler2D TextGeneratePass;


/* Composite one image over top another using the alpha to blend
 * It is expected that the input colors have been already premultiplied
 * which means their rgb has already been multiplied by their alpha */
vec4 PreMultAlphaBlend(vec4 color_under, vec4 color_over)
{
	vec4 out_color = vec4(color_over.rgb + (color_under.rgb * (1 - color_over.a)), clamp(color_under.a + color_over.a, 0, 1));
	return out_color;
}

void main()
{
	vec4 in_pass_color = texture(TextGeneratePass, vTexCoord);
	FragColor = in_pass_color;

	//////// Draw text to show resolutions ////////// 
	if (PRINT_RESOLUTION_TEXT_ON == 1 && vTexCoord.x > params.OutputSize.z * 2 && vTexCoord.y > params.OutputSize.w * 2)
	{
		vec2 ps = params.OutputSize.zw;
		vec2 outline_offset_mult = ps;
		outline_offset_mult.x *= max(floor(params.OutputSize.x / 240 + 0.01), 1);
		outline_offset_mult.y *= max(floor(params.OutputSize.y / 200 + 0.01), 1);

		vec4 text_rgba = vec4(FragColor.a);
		text_rgba.rgb *= vec3(1, 1, 0);
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(1, 0)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(0, 1)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(1, 1)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(-1, 1)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(-1, 0)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(0, -1)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(-1, -1)).a;
		text_rgba.a += texture(TextGeneratePass, vTexCoord + outline_offset_mult * vec2(1, -1)).a;
		text_rgba = clamp(text_rgba, 0, 1);
		FragColor.rgb = PreMultAlphaBlend(FragColor, text_rgba).rgb;
	}

	return;
}