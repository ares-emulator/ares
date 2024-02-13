#version 450

// film noise
// by hunterk
// license: public domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float x_off_r, y_off_r, x_off_g, y_off_g, x_off_b, y_off_b, grain_str,
	   hotspot, vignette, noise_toggle, jitter, vig_flicker, vig_amt, technicolor;
} params;

#pragma parameter jitter "Jitter Toggle" 1.0 0.0 1.0 1.0
#pragma parameter noise_toggle "Film Scratches" 1.0 0.0 1.0 1.0
#pragma parameter hotspot "Hotspot Toggle" 1.0 0.0 1.0 1.0
#pragma parameter vignette "Vignette Toggle" 1.0 0.0 1.0 1.0
#pragma parameter vig_flicker "Vignette Flicker Toggle" 1.0 0.0 1.0 1.0
#pragma parameter technicolor "Color Shift Intensity" 0.5 0.0 1.0 0.01
#pragma parameter x_off_r "X Offset Red" 0.12 -1.0 1.0 0.01
#pragma parameter y_off_r "Y Offset Red" 0.07 -1.0 1.0 0.01
#pragma parameter x_off_g "X Offset Green" -0.06 -1.0 1.0 0.01
#pragma parameter y_off_g "Y Offset Green" -0.08 -1.0 1.0 0.01
#pragma parameter x_off_b "X Offset Blue" -0.11 -1.0 1.0 0.01
#pragma parameter y_off_b "Y Offset Blue" 0.09 -1.0 1.0 0.01
#pragma parameter grain_str "Grain Strength" 24.0 0.0 60.0 1.0
#pragma parameter vig_amt "Vignette Amount" 0.15 0.0 1.0 0.01

#define x_off_r params.x_off_r
#define x_off_g params.x_off_g
#define x_off_b params.x_off_b
#define y_off_r params.y_off_r
#define y_off_g params.y_off_g
#define y_off_b params.y_off_b

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

float hash( float n ){
   return fract(sin(n)*43758.5453123);
}

// global timer
float time = mod(params.FrameCount, 4268.);

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 centerCoord;
layout(location = 2) out vec2 noiseCoord;
layout(location = 3) out float hash_num1;
layout(location = 4) out float hash_num2;
layout(location = 5) out float hash_num3;
layout(location = 6) out float frame_hash;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   
// move the image to center-origin for consistent deconvergence effects
   centerCoord = TexCoord - 0.5;
// jitter on the y-axis
   centerCoord.y += 0.003 * hash(sin(mod(time, 403.))) * params.jitter;
   
// calculate some magic pseudo-random numbers in the vertex for performance reasons
   hash_num1 = sin(hash(mod(time, 147.0)));
   hash_num2 = hash(cos(mod(time, 292.0)));
   hash_num3 = cos(hash(mod(time, 361.0)));
   frame_hash = hash(time);
   
// flip the x/y axes pseudo-randomly to give the noise texture some variety
   noiseCoord.x = (hash_num1 > 0.15) ? vTexCoord.x : 1.0 - vTexCoord.x;
   noiseCoord.y = ((hash_num1 + hash_num2 - 0.5) > 0.66) ? vTexCoord.y : 1.0 - vTexCoord.y;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 centerCoord;
layout(location = 2) in vec2 noiseCoord;
layout(location = 3) in float hash_num1;
layout(location = 4) in float hash_num2;
layout(location = 5) in float hash_num3;
layout(location = 6) in float frame_hash;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D noise1;

//https://www.shadertoy.com/view/4sXSWs strength= 16.0
float filmGrain(vec2 uv, float strength, float timer ){       
   float x = (uv.x + 4.0 ) * (uv.y + 4.0 ) * ((mod(timer, 800.0) + 10.0) * 10.0);
   return (mod((mod(x, 13.0) + 1.0) * (mod(x, 123.0) + 1.0), 0.01)-0.005) * strength;
}

const vec3 redfilter = vec3(1.0, 0.0, 0.0);
const vec3 bluegreenfilter = vec3(0.0, 1.0, 0.7);

void main()
{
// a simple calculation for the vignette/hotspot effects
	vec2 middle = centerCoord;
	float len = length(middle) / 1.25;
	float vig = smoothstep(0.0, 1.0 - params.vig_amt, len);

// create the noise/scratching effects from a LUT of actual film noise
	vec4 film_noise1 = texture(noise1, noiseCoord.xx + (hash_num1 + hash_num3 - 0.85));
	vec4 film_noise2 = texture(noise1, noiseCoord.xy + (hash_num1 + hash_num2 - 0.85));

// set up color channel offsets / deconvergence
	vec2 red_coord = (centerCoord + 0.01 * vec2(x_off_r, y_off_r)) + 0.5;
	vec3 red_light = texture(Source, red_coord).rgb;
	vec2 green_coord = (centerCoord + 0.01 * vec2(x_off_g, y_off_g)) + 0.5;
	vec3 green_light = texture(Source, green_coord).rgb;
	vec2 blue_coord = (centerCoord + 0.01 * vec2(x_off_r, y_off_r)) + 0.5;
	vec3 blue_light = texture(Source, blue_coord).rgb;

// composite the color channels
	vec3 film = vec3(red_light.r, green_light.g, blue_light.b);
	
// technicolor effect from aybe:
// https://github.com/aybe/RetroArch-shaders/blob/master/shaders/technicolor1.cg
	vec3 redrecord = film * redfilter;
	vec3 bluegreenrecord = film * bluegreenfilter;
	vec3 rednegative = vec3(redrecord.r);
	vec3 bluegreennegative = vec3((bluegreenrecord.g + bluegreenrecord.b) / 2.0);
	vec3 redoutput = rednegative * redfilter;
	vec3 bluegreenoutput = bluegreennegative * bluegreenfilter;
	vec3 result = redoutput + bluegreenoutput;
	film = mix(film, result, vec3(params.technicolor));

// apply film grain
	film += filmGrain(vTexCoord.xy, params.grain_str, time / 10.);

// apply vignetting and hotspot and vary the size a bit pseudo-randomly
   float vig_mod = 1.0 + 0.1 * hash_num3 * params.vig_flicker;
	film *= mix(1.0, 1.0 - vig, params.vignette * vig_mod); // Vignette
	film += ((1.0 - vig) * 0.2) * params.hotspot * vig_mod; // Hotspot

// Apply noise/scratching effects (or not) pseudo-randomly
	if (frame_hash > 0.99 && params.noise_toggle > 0.5)
		FragColor = vec4(mix(film, film_noise1.rgb, film_noise1.a), 1.0);
	else if (frame_hash < 0.01 && params.noise_toggle > 0.5)
		FragColor = vec4(mix(film, film_noise2.rgb, film_noise2.a), 1.0);
	else
		FragColor = vec4(film, 1.0);
}
