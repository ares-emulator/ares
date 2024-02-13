#version 450

// BigBlur by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float aspect_x;
	float aspect_y;
	float border_zoom;
	float BRIGHTNESS;
	float integer_scale;
	float overscale;
	float scanline_toggle;
	float interp_toggle;
	float THICKNESS;
	float DARKNESS;
	float OS_MASK_TOP;
	float OS_MASK_BOTTOM;
	float OS_MASK_LEFT;
	float OS_MASK_RIGHT;
} params;

#pragma parameter aspect_x "Aspect Ratio Numerator" 64.0 1.0 256. 1.0
#pragma parameter aspect_y "Aspect Ratio Denominator" 49.0 1.0 256. 1.0
#pragma parameter border_zoom "Border Zoom" 1.5 0.5 10 0.5
#pragma parameter BRIGHTNESS "Border Brightness Mod" 0.0 -1.0 1.0 0.05
#pragma parameter integer_scale "Force Integer Scaling" 1.0 0.0 1.0 1.0
#pragma parameter overscale "Integer Overscale" 0.0 0.0 1.0 1.0
#pragma parameter scanline_toggle "Scanline Toggle" 0.0 0.0 1.0 1.0
#pragma parameter interp_toggle "Sharpen Linear Scaling" 0.0 0.0 1.0 1.0
#pragma parameter THICKNESS "Scanline Thickness" 2.0 1.0 12.0 1.0
#pragma parameter DARKNESS "Scanline Darkness" 0.35 0.0 1.0 0.05
#pragma parameter OS_MASK_TOP "OS Mask Top" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_BOTTOM "OS Mask Bottom" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_LEFT "OS Mask Left" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_RIGHT "OS Mask Right" 0.0 0.0 1.0 0.005

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
	gl_Position = global.MVP * Position;
	vec2 out_res = params.OutputSize.xy;
	vec2 corrected_size = params.SourceSize.xy * vec2(params.aspect_x / params.aspect_y, 1.0)
		 * vec2(params.SourceSize.y / params.SourceSize.x, 1.0);
	float full_scale = (params.integer_scale > 0.5) ? floor(params.OutputSize.y /
		params.SourceSize.y) + params.overscale : params.OutputSize.y / params.SourceSize.y;
	vec2 scale = (params.OutputSize.xy / corrected_size) / full_scale;
	vec2 middle = vec2(0.49999, 0.49999);
	vec2 diff = TexCoord.xy - middle;
	vTexCoord = middle + diff * scale;
	vec2 zoom_coord = (((TexCoord.xy - middle) / params.border_zoom) * vec2(params.OutputSize.x / params.OutputSize.y, 1.0)
		/ vec2(params.aspect_x / params.aspect_y, 1.0)) + middle;
	tex_border = zoom_coord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 tex_border;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Reference;

vec4 scanlines(vec4 frame, vec2 coord, vec2 texture_size, vec2
	video_size, vec2 output_size)
{
	float lines = fract(coord.y * texture_size.y);
	float scale_factor = floor((output_size.y / video_size.y) + 0.4999);
    float lightness = 1.0 - params.DARKNESS;
	return (params.scanline_toggle > 0.5 && (lines < (1.0 / scale_factor * params.THICKNESS)))
		? frame * vec4(lightness, lightness, lightness, lightness) : frame;
}

vec2 interp_coord(vec2 coord, vec2 texture_size)
{
	vec2 p = coord.xy;
	p = p * texture_size.xy + vec2(0.5, 0.5);
	vec2 i = floor(p);
	vec2 f = p - i;
	// Smoothstep - amazingly, smoothstep() is slower than calculating directly the expression!
	f = f * f * f * f * (f * (f * (-20.0 * f + vec2(70.0, 70.0)) - vec2(84.0, 84.0)) + vec2(35.0, 35.0));
	p = i + f;
	p = (p - vec2(0.5, 0.5)) * 1.0 / texture_size;
	return p;
}

vec4 border(vec2 texture_size, vec2 video_size, vec2 output_size,
	float frame_count, vec2 tex, sampler2D decal, vec2 tex_border, sampler2D ORIG)
{
	vec4 effect = texture(decal, tex_border);
	effect += vec4(vec3(params.BRIGHTNESS), effect.w);
	
	vec2 coord = (params.interp_toggle < 0.5) ? tex : interp_coord(tex, texture_size);
	vec4 frame = texture(ORIG, coord);
	frame = scanlines(frame, tex, texture_size, video_size, output_size);
	vec2 fragcoord = (coord.xy);
	if (fragcoord.x < 1.0 - params.OS_MASK_RIGHT && fragcoord.x > 0.0 + params.OS_MASK_LEFT &&
		fragcoord.y < 1.0 - params.OS_MASK_BOTTOM && fragcoord.y > 0.0 + params.OS_MASK_TOP)
			return frame;
	
	else return effect;
}

void main()
{
	FragColor = border(params.SourceSize.xy, params.SourceSize.xy, params.OutputSize.xy,
		float(params.FrameCount), vTexCoord, Source, tex_border, Reference);
}