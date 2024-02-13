#version 450

/**
 * Vibrance
 * by Christian Cann Schuldt Jensen ~ CeeJay.dk
 *
 * Vibrance intelligently boosts the saturation of pixels so pixels that had little color get a larger boost than pixels that had a lot.
 * This avoids oversaturation of pixels that were already very saturated.
 */

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

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
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	#ifndef Vibrance_RGB_balance //for backwards compatibility with setting presets for older version.
		#define Vibrance_RGB_balance vec3(1.00, 1.00, 1.00)
	#endif
	#define Vibrance 0.5
	#define Vibrance_coeff vec3(Vibrance_RGB_balance.r * Vibrance, Vibrance_RGB_balance.g * Vibrance, Vibrance_RGB_balance.b * Vibrance)

	vec3 col = texture(Source, vTexCoord).rgb;
	vec3 lumCoeff = vec3(0.212656, 0.715158, 0.072186);  //Values to calculate luma with

	float luma = dot(lumCoeff, col); //calculate luma (grey)

	float max_color = max(col.r, max(col.g,col.b)); //Find the strongest color
	float min_color = min(col.r, min(col.g,col.b)); //Find the weakest color

	float color_saturation = max_color - min_color; //The difference between the two is the saturation
  
    col.r = mix(luma, col.r, (1.0 + (Vibrance_coeff.r * (1.0 - (sign(Vibrance_coeff.r) * color_saturation)))));
    col.g = mix(luma, col.g, (1.0 + (Vibrance_coeff.g * (1.0 - (sign(Vibrance_coeff.g) * color_saturation)))));
    col.b = mix(luma, col.b, (1.0 + (Vibrance_coeff.b * (1.0 - (sign(Vibrance_coeff.b) * color_saturation)))));
   FragColor = vec4(col, 1.0);
}