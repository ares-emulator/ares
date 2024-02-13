#ifndef INT_AR
#define INT_AR

// Forces integer scaling and arbitrary aspect ratio
// by hunterk
// license: public domain

// Apply in the vertex to the texCoord

#pragma parameter ar_num "Aspect Ratio Numerator" 64.0 1.0 256. 1.0
#pragma parameter ar_den "Aspect Ratio Denominator" 49.0 1.0 256. 1.0
#pragma parameter integer_scale "Force Integer Scaling" 0.0 0.0 1.0 1.0
#pragma parameter overscale "Integer Overscale" 0.0 0.0 1.0 1.0

const vec2 middle = vec2(0.4999999, 0.4999999);

vec2 int_ar(vec2 coord, vec4 tex_size, vec4 out_size){
	vec2 corrected_size = tex_size.xy * vec2(ar_num / ar_den, 1.0)
		 * vec2(tex_size.y / tex_size.x, 1.0);
	float full_scale = (integer_scale > 0.5) ? floor(out_size.y /
		tex_size.y) + overscale : out_size.y / tex_size.y;
	vec2 scale = (out_size.xy / corrected_size) / full_scale;
	vec2 diff = coord.xy - middle;
	vec2 ar_coord = middle + diff * scale;
	return ar_coord;
}

#endif
