#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float integer_scale;
	float overscale;
	float effect;
	float scanline_toggle;
	float SCANLINE_BASE_BRIGHTNESS;
	float SCANLINE_SINE_COMP_A;
	float SCANLINE_SINE_COMP_B;
	float interp_toggle;
	float OS_MASK_TOP;
	float OS_MASK_BOTTOM;
	float OS_MASK_LEFT;
	float OS_MASK_RIGHT;
	float aspect_x;
	float aspect_y;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter effect "Glow Brightness" 1.0 0.1 3.0 0.1
#pragma parameter aspect_x "Aspect Ratio Numerator" 64.0 1.0 256. 1.0
#pragma parameter aspect_y "Aspect Ratio Denominator" 49.0 1.0 256. 1.0
#pragma parameter integer_scale "Force Integer Scaling" 1.0 0.0 1.0 1.0
#pragma parameter overscale "Integer Overscale" 0.0 0.0 1.0 1.0
#pragma parameter interp_toggle "Sharpen Linear Scaling" 0.0 0.0 1.0 1.0
#pragma parameter scanline_toggle "Scanline Toggle" 0.0 0.0 1.0 1.0
#pragma parameter SCANLINE_BASE_BRIGHTNESS "Scanline Base Brightness" 0.95 0.0 1.0 0.01
#pragma parameter SCANLINE_SINE_COMP_A "Scanline Sine Comp A" 0.00 0.0 0.10 0.01
#pragma parameter SCANLINE_SINE_COMP_B "Scanline Intensity" 0.15 0.0 1.0 0.05
#pragma parameter OS_MASK_TOP "Overscan Mask Top" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_BOTTOM "Overscan Mask Bottom" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_LEFT "Overscan Mask Left" 0.0 0.0 1.0 0.005
#pragma parameter OS_MASK_RIGHT "Overscan Mask Right" 0.0 0.0 1.0 0.005

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 border_coord;
layout(location = 1) out vec2 screen_coord;

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
	screen_coord = middle + diff * scale;
	border_coord = TexCoord.xy;
}

#pragma stage fragment
layout(location = 0) in vec2 border_coord;
layout(location = 1) in vec2 screen_coord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Original;

vec4 scanline(vec2 coord, vec4 frame)
{
	vec2 omega = vec2(3.1415 * params.OutputSize.x, 2.0 * 3.1415 * params.SourceSize.y);
	vec2 sine_comp = vec2(params.SCANLINE_SINE_COMP_A, params.SCANLINE_SINE_COMP_B);
	vec3 res = frame.xyz;
	vec3 scanline = res * (params.SCANLINE_BASE_BRIGHTNESS + dot(sine_comp * sin(coord * omega), vec2(1.0, 1.0)));
	return vec4(scanline.x, scanline.y, scanline.z, 1.0);
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

vec4 border(vec2 screen_coord, vec2 border_coord, vec2 texture_size, vec2 video_size,
	vec2 output_size, float frame_count, sampler2D decal)
{
	vec4 background;
	vec4 color = texture(Source, border_coord, 10.0) * 1.25 + 0.125; // use mipmapping to get an average of the entire screen
	background = color * color * params.effect;
	
	vec2 coord = (params.interp_toggle < 0.5) ? screen_coord : interp_coord(screen_coord, texture_size);
	vec4 frame = texture(decal, coord);
	frame = (params.scanline_toggle > 0.5) ? scanline(coord, frame) : frame;
	vec2 fragcoord = (coord.xy);
	if (fragcoord.x < 1.0 - params.OS_MASK_RIGHT && fragcoord.x > 0.0 + params.OS_MASK_LEFT &&
		fragcoord.y < 1.0 - params.OS_MASK_BOTTOM && fragcoord.y > 0.0 + params.OS_MASK_TOP)
			return frame;
	
	else return background;
}

void main()
{
	FragColor = border(screen_coord, border_coord, params.SourceSize.xy, params.SourceSize.xy, 
		params.OutputSize.xy, float(params.FrameCount), Original);
}