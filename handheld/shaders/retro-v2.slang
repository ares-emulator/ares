#version 450

/*
   Hyllian - Retro Shader - 2013

   A re-implementation from the original made by Hyllian and DOLLS!
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

layout(push_constant) uniform Push
{
    float RETRO_PIXEL_SIZE;
    float RETRO_COLOR_BOOST;
    float RETRO_GAMMA_IN;
    float RETRO_GAMMA_OUT;
} param;

// This value must be between 0.0 (totally black) and 1.0 (nearest neighbor)
#pragma parameter RETRO_PIXEL_SIZE "Retro Pixel Size"   0.84 0.5 1.0 0.01
#pragma parameter RETRO_COLOR_BOOST "Retro Color Boost" 1.0 1.0 2.0 0.01
#pragma parameter RETRO_GAMMA_IN "Retro Gamma In"       2.20 1.0 3.0 0.05
#pragma parameter RETRO_GAMMA_OUT "Retro Gamma LCD"     2.20 1.0 3.0 0.05

#define GAMMA_OUT  (1.0 / param.RETRO_GAMMA_OUT)

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/*
    VERTEX_SHADER
*/
void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

/*
    FRAGMENT SHADER
*/
void main()
{
    // Reading the texel
    vec3 E = pow(texture(Source, vTexCoord).xyz, vec3(param.RETRO_GAMMA_IN));

    vec2 fp = fract(vTexCoord*global.SourceSize.xy);
    vec2 ps = global.SourceSize.xy * global.OutputSize.zw;

    vec2 f = clamp(clamp(fp + 0.5*ps, 0.0, 1.0) - param.RETRO_PIXEL_SIZE, vec2(0.0), ps)/ps;

    float max_coord =  max(f.x, f.y);

    vec3 res = mix(E*(1.04+fp.x*fp.y), E*0.36, max_coord);

    // Product interpolation
    FragColor = vec4(clamp( pow(param.RETRO_COLOR_BOOST*res, vec3(GAMMA_OUT)), 0.0, 1.0 ), 1.0);
}
