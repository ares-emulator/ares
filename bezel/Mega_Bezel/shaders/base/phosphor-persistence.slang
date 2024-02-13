#version 450

/*
    Mega Bezel - Creates a graphic treatment for the game play area to give a retro feel
    Copyright (C) 2019-2022 HyperspaceMadness - HyperspaceMadness@outlook.com

    Incorporates much great feedback from the libretro forum, and thanks 
    to Hunterk who helped me get started

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

#include "common/globals.inc"
#include "params-0-screen-scale.inc"
#include "common/common-functions.inc"

//---------------------------------------------------------------------------------------------------
//    PHOSPHOR PERSISTENCE
//---------------------------------------------------------------------------------------------------
#pragma parameter HSM_PERSISTENCE_TITLE						"[ PHOSPHOR PERSISTENCE ]:" 0 0 0.01 0.01

#pragma parameter HSM_PHOSPHOR_PERSISTENCE_BLEND          	"          Amount" 0 0 100 2
#define HSM_PHOSPHOR_PERSISTENCE_BLEND global.HSM_PHOSPHOR_PERSISTENCE_BLEND / 100

#pragma parameter HSM_PHOSPHOR_PERSISTENCE_RED          	"          Red Persistence" 50 0 100 5
#define HSM_PHOSPHOR_PERSISTENCE_RED global.HSM_PHOSPHOR_PERSISTENCE_RED / 100

#pragma parameter HSM_PHOSPHOR_PERSISTENCE_GREEN          	"          Green Persistence" 50 0 100 5
#define HSM_PHOSPHOR_PERSISTENCE_GREEN global.HSM_PHOSPHOR_PERSISTENCE_GREEN / 100

#pragma parameter HSM_PHOSPHOR_PERSISTENCE_BLUE          	"          Blue Persistence" 50 0 100 5
#define HSM_PHOSPHOR_PERSISTENCE_BLUE global.HSM_PHOSPHOR_PERSISTENCE_BLUE / 100


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
layout(set = 0, binding = 3) uniform sampler2D PersistencePassFeedback;

#define previous_pass_feedback PersistencePassFeedback
#define eps 1e-3

vec4 HSM_ApplyPhosphorPersistence(vec4 in_color, vec4 prev_color, vec2 in_coord)
{
	if (HSM_PHOSPHOR_PERSISTENCE_RED == 0 && HSM_PHOSPHOR_PERSISTENCE_GREEN == 0 && HSM_PHOSPHOR_PERSISTENCE_BLUE == 0) 
		return in_color;

	vec3 persistence = vec3(HSM_PHOSPHOR_PERSISTENCE_RED, HSM_PHOSPHOR_PERSISTENCE_GREEN, HSM_PHOSPHOR_PERSISTENCE_BLUE);

	float delta_time_red = clamp(5 - 5 * HHLP_EasePowerIn(abs(HSM_PHOSPHOR_PERSISTENCE_RED), 2) + 0.1, 0, 1);
	float delta_time_green = clamp(5 - 5 * HHLP_EasePowerIn(abs(HSM_PHOSPHOR_PERSISTENCE_GREEN), 2) + 0.1, 0, 1);
	float delta_time_blue = clamp(5 - 5 * HHLP_EasePowerIn(abs(HSM_PHOSPHOR_PERSISTENCE_BLUE), 2) + 0.1, 0, 1);

	prev_color.r *= pow(persistence.r, delta_time_red);
	prev_color.g *= pow(persistence.g, delta_time_green);
	prev_color.b *= pow(persistence.b, delta_time_blue);

   prev_color *= HSM_PHOSPHOR_PERSISTENCE_BLEND;

	vec4 out_color = in_color;
	out_color.r = max(prev_color.r, out_color.r);
	out_color.g = max(prev_color.g, out_color.g);
	out_color.b = max(prev_color.b, out_color.b);

	return out_color;
}

void main()
{
   FragColor = texture(Source, vTexCoord);
   FragColor = HSM_Linearize(FragColor, DEFAULT_SRGB_GAMMA);

	vec4 prev_color = texture(previous_pass_feedback, vTexCoord);
   prev_color = HSM_Linearize(prev_color, DEFAULT_SRGB_GAMMA);

   // Phosphor Persistence
   FragColor = HSM_ApplyPhosphorPersistence(FragColor, prev_color, vTexCoord);

   // FragColor = mix(texture(previous_pass_feedback, vTexCoord), texture(Source, vTexCoord), 0.5);
   FragColor = HSM_Delinearize(FragColor, DEFAULT_SRGB_GAMMA);
}