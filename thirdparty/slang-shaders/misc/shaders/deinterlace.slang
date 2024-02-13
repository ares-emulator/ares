#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	uint FrameCount;
	float interlace_bff;
	float force_noninterlaced;
	float interlace_1080;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

#pragma parameter interlace_bff "Interlacing BFF" 0.0 0.0 1.0 1.0
#pragma parameter force_noninterlaced "Force Non-interlaced" 0.0 0.0 1.0 1.0
#pragma parameter interlace_1080 "1080 is Interlaced" 0.0 0.0 1.0 1.0

/*
   Deinterlacing
   Author: Trogglemonkey, isolated from crt-royale by hunterk
   License: GPLv2
*/

bool is_interlaced(float num_lines)
{
    //  Detect interlacing based on the number of lines in the source.
        //  NTSC: 525 lines, 262.5/field; 486 active (2 half-lines), 243/field
        //  NTSC Emulators: Typically 224 or 240 lines
        //  PAL: 625 lines, 312.5/field; 576 active (typical), 288/field
        //  PAL Emulators: ?
        //  ATSC: 720p, 1080i, 1080p
        //  Where do we place our cutoffs?  Assumptions:
        //  1.) We only need to care about active lines.
        //  2.) Anything > 288 and <= 576 lines is probably interlaced.
        //  3.) Anything > 576 lines is probably not interlaced...
        //  4.) ...except 1080 lines, which is a crapshoot (user decision).
        //  5.) Just in case the main program uses calculated video sizes,
        //      we should nudge the float thresholds a bit.
        bool sd_interlace = ((num_lines > 288.5) && (num_lines < 576.5) && (registers.force_noninterlaced < 0.5));
        bool hd_interlace = (registers.interlace_1080 > 0.5);
        return (sd_interlace || hd_interlace);
}

vec3 tex2D_linearize(sampler2D tex, vec2 uv){
	return pow(texture(tex, uv).rgb, vec3(2.2));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float interlaced;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   interlaced = is_interlaced(registers.SourceSize.y) ? 1.0 : 0.0;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float interlaced;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
        const vec2 v_step = vec2(0.0, registers.SourceSize.w);
        const vec3 curr_line = tex2D_linearize(
            Source, vTexCoord).rgb;
        const vec3 last_line = tex2D_linearize(
            Source, vTexCoord - v_step).rgb;
        const vec3 next_line = tex2D_linearize(
            Source, vTexCoord + v_step).rgb;
        const vec3 interpolated_line = 0.5 * (last_line + next_line);
        //  If we're interlacing, determine which field curr_line is in:
        const float modulus = interlaced + 1.0;
        const float field_offset =
            mod(registers.FrameCount + registers.interlace_bff, modulus);
        const float curr_line_texel = vTexCoord.y * registers.SourceSize.y;
        //  Use under_half to fix a rounding bug around exact texel locations.
        const float line_num_last = floor(curr_line_texel - 0.4995);
        const float wrong_field = mod(line_num_last + field_offset, modulus);
        //  Select the correct color, and output the result:
        const vec3 color = mix(curr_line, interpolated_line, wrong_field);
   FragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}