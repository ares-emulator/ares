#ifndef FILM_GRAIN
#define FILM_GRAIN

#pragma parameter ia_GRAIN_STR "Grain Strength" 0.0 0.0 72.0 3.0

#include "col_tools.h"

//https://www.shadertoy.com/view/4sXSWs strength= 16.0
vec3 rgb_grain(vec2 uv, float strength, uint counter){       
    float x = ((uv.x + 4.0 ) * (uv.y + 5.0 ) * ((mod(vec2(counter, counter).x, 800.0) + 10.0) * 8.0));
    float y = ((uv.x + 5.0 ) * (uv.y + 6.0 ) * ((mod(vec2(counter, counter).x, 678.0) + 8.0) * 6.0));
    float z = ((uv.x + 6.0 ) * (uv.y + 4.0 ) * ((mod(vec2(counter, counter).x, 498.0) + 6.0) * 10.0));
    float r = (mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005);
    float g = (mod((mod(y, 13.0) + 1.0) * (mod(y, 123.0) + 1.0), 0.01)-0.005);
    float b = (mod((mod(z, 13.0) + 1.0) * (mod(z, 123.0) + 1.0), 0.01)-0.005);
	return  vec3(r, g, b) * strength;
}

vec3 luma_grain(vec3 in_col, vec2 uv, float strength, uint counter){
    float x = (uv.x + 4.0 ) * (uv.y + 4.0 ) * ((mod(vec2(counter, counter).x, 800.0) + 10.0) * 10.0);
	 x = (mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * strength;
	 vec3 out_col = RGBtoYIQ(in_col);
	 out_col.r += x;
	 return YIQtoRGB(out_col);
}

#endif
