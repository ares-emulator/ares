#version 450

/*
 * Deband shader by haasn
 * https://github.com/mpv-player/mpv/blob/master/video/out/opengl/video_shaders.c
 *
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpv.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can alternatively redistribute this file and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * Modified and optimized for RetroArch by hunterk
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float iterations;
	float threshold;
	float range;
	float grain;
} params;

#pragma parameter iterations "Deband Iterations" 2.0 1.0 10.0 1.0
#pragma parameter threshold "Deband Threshold" 1.0 1.0 16.0 0.5
#pragma parameter range "Deband Range" 1.5 0.0 10.0 0.5
#pragma parameter grain "Deband Grain" 0.0 0.0 2.0 0.1

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


// Wide usage friendly PRNG, shamelessly stolen from a GLSL tricks forum post.
// Obtain random numbers by calling rand(h), followed by h = permute(h) to
// update the state. Assumes the texture was hooked.
float mod289(float x)
{
	return x - floor(x / 289.0) * 289.0;
}

float permute(float x)
{
	return mod289((34.0 * x + 1.0) * x);
}

float rand(float x)
{
	return fract(x * 0.024390243);
}

vec4 average(sampler2D tex, vec2 coord, float range, inout float h)
{
	float dist = rand(h) * range;	h = permute(h);
	float dir = rand(h) * 6.2831853;	h = permute(h);
	vec2 o = vec2(cos(dir), sin(dir));
	vec2 pt = dist * params.SourceSize.zw;
	
	vec4 ref[4];
	ref[0] = texture(tex, coord + pt * vec2( o.x, o.y));
	ref[1] = texture(tex, coord + pt * vec2(-o.y, o.x));
	ref[2] = texture(tex, coord + pt * vec2(-o.x,-o.y));
	ref[3] = texture(tex, coord + pt * vec2( o.y,-o.x));
	
	return (ref[0] + ref[1] + ref[2] + ref[3]) * 0.25;
}

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
	// Initialize the PRNG by hashing the position + a random uniform
	vec3 m = vec3(vTexCoord, rand(sin(vTexCoord.x / vTexCoord.y) * mod(params.FrameCount, 79) + 22.759)) + vec3(1.0);
	float h = permute(permute(permute(m.x) + m.y) + m.z);
	
	vec4 avg;
	vec4 diff;
	
	// Sample the source pixel
	vec4 color = texture(Source, vTexCoord).rgba;
	
	for (int i = 1; i <= int(params.iterations); i++)
		{
			// Sample the average pixel and use it instead of the original if
			// the difference is below the given threshold
			avg = average(Source, vTexCoord, i * params.range, h);
			diff = abs(color - avg);
			color = mix(avg, color, greaterThan(diff, vec4(params.threshold / (i * 10.0))));
		}
	if (params.grain > 0.0)
		{
			// Add some random noise to smooth out residual differences
			vec3 noise;
			noise.x = rand(h); h = permute(h);
			noise.y = rand(h); h = permute(h);
			noise.z = rand(h); h = permute(h);
			color.rgb += params.grain * (noise - vec3(0.5));
		}
	
   FragColor = vec4(color);
}