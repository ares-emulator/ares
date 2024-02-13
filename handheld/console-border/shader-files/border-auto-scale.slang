#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 SourceSize;
	vec4 OriginalSize;
	float border_texture_scale;
	float border_brightness;
} params;

#pragma parameter border_texture_scale "Border Scale" 4.0 1.0 20.0 0.005
#pragma parameter bogus_scale_instructions "[SCREEN SIZE]: In previous menu->Change lcd-grid scale->Apply" 0.0 0.0 1.0 1.0
#pragma parameter border_brightness "Border Brightness" 1.0 0.0 1.0 0.01

vec2 middle	= vec2(0.5, 0.5);
vec2 screen_ratio = params.OutputSize.xy * params.SourceSize.zw;
vec2 border_scale = vec2(3200, 1600) * (params.SourceSize.y / params.OriginalSize.y) / params.border_texture_scale;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 tex_border;

void main()
{
	gl_Position	=	global.MVP * Position;

	vec2 diff	=	TexCoord.xy - middle;
	vTexCoord	=	middle + diff * screen_ratio;
	tex_border	=	middle + diff * params.OutputSize.xy / border_scale;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 tex_border;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D BORDER;

void main()
{
	vec4 frame	=	texture(Source, vTexCoord).rgba;
	vec4 border	=	texture(BORDER, tex_border).rgba;
	FragColor	=	vec4(mix(frame, border * params.border_brightness, border.a));
}
