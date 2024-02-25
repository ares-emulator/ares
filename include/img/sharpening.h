#ifndef SHARP_COORD
#define SHARP_COORD

// wrap texture coordinate when sampling

#pragma parameter ia_SHARPEN "Sharpen" 0.0 0.0 1.0 0.05

const vec2 offset = vec2(0.5, 0.5);

// based on "Improved texture interpolation" by Iñigo Quílez
// Original description: http://www.iquilezles.org/www/articles/texture/texture.htm
vec2 sharp(vec2 in_coord, vec4 tex_size){
	vec2 p = in_coord.xy;
	p = p * tex_size.xy + offset;
	vec2 i = floor(p);
	vec2 f = p - i;
	f = f * f * f * (f * (f * 6.0 - vec2(15.0, 15.0)) + vec2(10.0, 10.0));
	p = i + f;
	p = (p - offset) * tex_size.zw;
	p = mix(in_coord, p, ia_SHARPEN);
	return p;
}

#endif
