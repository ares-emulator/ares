/*
    Mega Bezel - Creates a graphic treatment for the game play area to give a retro feel
    Copyright (C) 2019-2022 HyperspaceMadness - HyperspaceMadness@outlook.com

	Texture interpolation based on "Improved texture interpolation" by Inigo Quilez
	Original description: http://www.iquilezles.org/www/articles/texture/texture.htm
	Expects the texture to be using linear filtering

    See more at the libretro forum
    https://forums.libretro.com/t/hsm-mega-bezel-reflection-shader-feedback-and-updates

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

float SOURCE_MATTE_PREMULTIPLIED = 0;
float SOURCE_MATTE_WHITE = 1;
float SOURCE_MATTE_NONE = 2;

float BLEND_MODE_OFF = 0;
float BLEND_MODE_NORMAL = 1;
float BLEND_MODE_ADD = 2;
float BLEND_MODE_MULTIPLY = 3;

vec4 HSM_ApplyGamma(vec4 in_color, float in_gamma)
{
	vec3 out_color = pow(in_color.rgb, vec3(1 / in_gamma));
	return vec4(out_color, in_color.a);
}

// 'Removes' encoded gamma from color to put the color in linear space
vec4 HSM_Linearize(vec4 in_color, float encoded_gamma)
{
	return HSM_ApplyGamma(in_color, 1 / encoded_gamma);
}

// Adds gamma onto color in linear space to get a color with encoded gamma
vec4 HSM_Delinearize(vec4 in_color, float in_gamma)
{
	return HSM_ApplyGamma(in_color, in_gamma);
}

vec3 HSM_RGBtoHSV(vec3 c)
{
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = c.g < c.b ? vec4(c.bg, K.wz) : vec4(c.gb, K.xy);
    vec4 q = c.r < p.x ? vec4(p.xyw, c.r) : vec4(c.r, p.yzx);

    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 HSM_HSVtoRGB(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 HSM_ApplyHSVAdjustment(vec3 in_color_rgb, float in_hue, float in_saturation, float in_brightness, float in_colorize_on, float in_gamma_adjust)
{
	if (!(in_colorize_on == 1 || in_hue != 0 || in_saturation != 1 || in_brightness != 1 || in_gamma_adjust != 1))
		return in_color_rgb;
	
	vec3 color_hsv = HSM_RGBtoHSV(in_color_rgb);

	if (in_colorize_on > 0.5)
	{
		color_hsv.x = in_hue;
		color_hsv.y = mix(mix(0, color_hsv.y, clamp(in_saturation, 0, 1)), 1, clamp(in_saturation - 1, 0, 1) );
	}
	else
	{
		color_hsv.x += in_hue;
		color_hsv.y *= in_saturation;
	}

	color_hsv.z *= in_brightness;

	vec3 color_rgb = HSM_HSVtoRGB(color_hsv);

	if (in_gamma_adjust != 1)
		color_rgb = HSM_ApplyGamma(vec4(color_rgb.r, color_rgb.g, color_rgb.b, 1), in_gamma_adjust).rgb;

	return color_rgb;
}

vec4 HSM_GetPreMultipliedColorLinear(vec4 in_color, float matte_type, float encoded_gamma)
{
	vec4 out_color = in_color;

	if (matte_type == SOURCE_MATTE_WHITE)
		out_color.rgb = clamp(out_color.rgb - (1 - out_color.a), 0, 1);

	out_color = HSM_Linearize(out_color, encoded_gamma);

	// If the color was not already premultiplied (matted with black) premultiply it now
	if (matte_type == SOURCE_MATTE_NONE)
		out_color.rgb *= out_color.a;

	return out_color;
}

/* Composite one image over top another using the alpha to blend
 * It is expected that the input colors have been already premultiplied
 * which means their rgb has already been multiplied by their alpha */
vec4 HSM_PreMultAlphaBlend(vec4 color_under, vec4 color_over)
{
	vec4 out_color = vec4(color_over.rgb + (color_under.rgb * (1 - color_over.a)), clamp(color_under.a + color_over.a, 0, 1));
	return out_color;
}

vec4 HSM_BlendMultiply(vec4 color_under, vec4 color_over, float opacity)
{
	float final_opacity = color_over.a * opacity;
	return vec4(color_under.rgb * (final_opacity * color_over.rgb + (1 - final_opacity) * vec3(1)), color_under.a);
}

// Assumes Opacity is already encoded in alpha
vec4 HSM_BlendModeLayerMix(vec4 color_under, vec4 color_over, float blend_mode, float layer_opacity)
{
	if (blend_mode == 0)
		return color_under;

	if (blend_mode == BLEND_MODE_OFF)
		return color_under;
	
	color_over.a *= layer_opacity;

	vec4 out_color = vec4(0);

	if (blend_mode == BLEND_MODE_NORMAL)
	{
		color_over.rgb *= color_over.a;
		out_color = HSM_PreMultAlphaBlend(color_under, color_over);
	}
	else
	{
		vec4 blend_color = color_under; 
		if (blend_mode == BLEND_MODE_ADD)  	 		blend_color.rgb = color_under.rgb + color_over.rgb ;
		if (blend_mode == BLEND_MODE_MULTIPLY)  	blend_color.rgb = color_under.rgb * color_over.rgb ;

		out_color = vec4(clamp(mix(color_under.rgb, blend_color.rgb, color_over.a), 0, 1), color_under.a);
	}
	return out_color;
}

vec4 HSM_TextureQuilez(sampler2D in_sampler_2D, vec2 in_texture_size, vec2 p)
{
	vec2 tex_size = textureSize(in_sampler_2D, 0);
	p = p * in_texture_size + vec2(0.5, 0.5);

	vec2 i = floor(p);
	vec2 f = p - i;
	f = f * f * f * (f * (f * 6.0 - vec2(15.0, 15.0)) + vec2(10.0, 10.0));
	p = i + f;

	p = (p - vec2(0.5, 0.5)) * (1/in_texture_size);

	// final sum and weight normalization
	return texture(in_sampler_2D, p);
}

float HHLP_GetMaskCenteredOnValue(float in_value, float value_to_match, float threshold)
{
	float edge_0 = value_to_match - threshold;
	float edge_1 = value_to_match - 0.5 * threshold;
	float edge_2 = value_to_match + 0.5 * threshold;
	float edge_3 = value_to_match + threshold;
	float out_mask = 1.0;
	out_mask *= smoothstep(edge_0, edge_1, in_value);
	out_mask *= smoothstep(edge_3, edge_2, in_value);
	return out_mask;
}

// Quadratic Bezier allows us to have a controlled falloff between 0 and 1 
// One use is to avoid the perception of discontinuity at the outer edge experienced with a linear gradients
float HHLP_QuadraticBezier (float x, vec2 a)
{
  // Originally adapted by @kyndinfo from BEZMATH.PS (1993) by Don Lancaster
  // http://www.tinaja.com/text/bezmath.html
  
  float epsilon = 0.00001;
  a.x = clamp(a.x,0.0,1.0); 
  a.y = clamp(a.y,0.0,1.0); 
  if (a.x == 0.5){
    a += epsilon;
  }
  
  // solve t from x (an inverse operation)
  float om2a = 1.0 - 2.0 * a.x;
  float t = (sqrt(a.x*a.x + om2a*x) - a.x)/om2a;
  float y = (1.0-2.0*a.y)*(t*t) + (2.0*a.y)*t;
  return y;
}

float HHLP_EasePowerIn(float x, float in_exponent) 
{
  x = max(0, min(x, 1));
  return pow(x, in_exponent);
}

float HHLP_EasePowerOut(float x, float in_exponent) 
{
  x = 1.0 - max(0, min(x, 1));
  return 1.0 - pow(x, in_exponent);
}

float HHLP_EasePowerInOut(float x, float in_exponent) 
{
  x = max(0, min(x, 1));
  if (x < 0.5) 
  {
    return pow(x * 2, in_exponent) * 0.5;
  } 
  else 
  {
    return 1.0 - pow((1 - x) * 2, in_exponent) * 0.5;
  }
}

float HHLP_GetDistanceToLine(float x1, float y1, float a, float b, float c)
{
    float d = abs((a * x1 + b * y1 + c)) /  
              (sqrt(a * a + b * b));
    return d; 
}

// Returns 1 if in_value < compare_value
// Useful when ifs are bad for performance
float HHLP_IsUnderValue(float in_value, float compare_value)
{
	return clamp((compare_value - in_value) * 100000, 0, 1);
}

// Returns 1 if in_value > compare_value
// Useful when ifs are bad for performance
float HHLP_IsOverValue(float in_value, float compare_value)
{
	return clamp(-1 * (compare_value - in_value) * 100000, 0, 1);
}

// Returns 1 if in_value == compare_value within the epsilon value provided
// Useful when ifs are bad for performance
float HHLP_EqualsValue(float in_value, float compare_value, float epsilon)
{
	return HHLP_IsUnderValue(in_value, compare_value + epsilon) * HHLP_IsOverValue(in_value, compare_value - epsilon);
}

float HHLP_EqualsResolution(vec2 in_res, vec2 test_res)
{
  float hardcoded_epsilon = 0.001;
  return  HHLP_EqualsValue(in_res.x, test_res.x, hardcoded_epsilon) * 
          HHLP_EqualsValue(in_res.y, test_res.y, hardcoded_epsilon);
}

vec4 HHLP_GetBilinearTextureSample(sampler2D in_sampler, vec2 in_coord, vec4 in_size)
{
   vec2 uv = in_coord * in_size.xy - 0.5; // Shift by 0.5 since the texel sampling points are in the texel center.
   vec2 a = fract(uv);
   vec2 tex = (floor(uv) + 0.5) * in_size.zw; // Build a sampling point which is in the center of the texel.

   // Sample the bilinear footprint.
   vec4 t0 = textureLodOffset(in_sampler, tex, 0.0, ivec2(0, 0));
   vec4 t1 = textureLodOffset(in_sampler, tex, 0.0, ivec2(1, 0));
   vec4 t2 = textureLodOffset(in_sampler, tex, 0.0, ivec2(0, 1));
   vec4 t3 = textureLodOffset(in_sampler, tex, 0.0, ivec2(1, 1));

   // Bilinear filter.
   vec4 result = mix(mix(t0, t1, a.x), mix(t2, t3, a.x), a.y);

   return result;
}

bool HHLP_IsOutsideCoordSpace(vec2 in_coord)
{
	return (abs(in_coord.x - 0.5) > 0.501 || abs(in_coord.y - 0.5) > 0.501);
}

float rand(vec2 co, float size){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453) * size;
}
