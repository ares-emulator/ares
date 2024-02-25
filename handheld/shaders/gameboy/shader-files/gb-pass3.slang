#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
} registers;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

///////////////////////////////////////////////////////////////////////////
//                                                                       //
// Gameboy Classic Shader v0.2.2                                         //
//                                                                       //
// Copyright (C) 2013 Harlequin : unknown92835@gmail.com                 //
//                                                                       //
// This program is free software: you can redistribute it and/or modify  //
// it under the terms of the GNU General Public License as published by  //
// the Free Software Foundation, either version 3 of the License, or     //
// (at your option) any later version.                                   //
//                                                                       //
// This program is distributed in the hope that it will be useful,       //
// but WITHOUT ANY WARRANTY; without even the implied warranty of        //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         //
// GNU General Public License for more details.                          //
//                                                                       //
// You should have received a copy of the GNU General Public License     //
// along with this program.  If not, see <http://www.gnu.org/licenses/>. //
//                                                                       //
///////////////////////////////////////////////////////////////////////////

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 texel;
layout(location = 2) out vec2 lower_bound;
layout(location = 3) out vec2 upper_bound;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord * 1.0001;

    texel = registers.SourceSize.zw;
    lower_bound = vec2(0.0);
    upper_bound = vec2(texel * (registers.OutputSize.xy - 1.0));
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 texel;
layout(location = 2) in vec2 lower_bound;
layout(location = 3) in vec2 upper_bound;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

/*
sigma = 4.0, normalized for 5 pixel offset       sigma = 4.0, normalized for 4 pixel offset
Raw Gaussian weights:                            Raw Gaussian weights:   
0.09973561222190091607086808117254 @position0    0.09973561222190091607086808117254 @position0
0.09666707205829167156101677393475 @position1    0.09666707205829167156101677393475 @position1
0.08801637626376240452162358324964 @position2    0.08801637626376240452162358324964 @position2
0.07528440407628116669052257071979 @position3    0.07528440407628116669052257071979 @position3
0.06049272702308815188099267447364 @position4    0.06049272702308815188099267447364 @position4
0.04566231462789672460813692086928 @position5            

sum [p0 + 2(p1 + p2 + p3 + p4 + p5)]:            sum [p0 + 2(p1 + p2 + p3 + p4)]:
0.83198140032054115459545312766674               0.74065677106474770537917928592818

Normalizing factor [1 / sum]:                    Normalizing factor [1 / sum]:
1.2019499469756482251051310195171                1.350153052084338115052273748029

Normalized Gaussian weights:                     Normalized Gaussian weights:
0.11987721382169761913280166382392 @position0    0.13465834124289953661305802732548 @position0
0.11618898213475484076479592086597 @position1    0.13051534237555914090930704141833 @position1
0.10579127878321792515352079488329 @position2    0.11883557904592230273554609080014 @position2
0.09048808548757942339961181362524 @position3    0.10164546793794160274995705611009 @position3
0.07270923003781316665844409497651 @position4    0.08167444001912718529866079800870 @position4
0.05488381664578583445722654373702 @position5
*/
void main()
{
    // Define offsets and weights - change this for both the X and Y passes if you change the sigma value or number of texels sampled
    float offsets[5] = float[](0.0, 1.0, 2.0, 3.0, 4.0);
    
    /*
    Precalculated using the Gaussian function:
    G(x) = (1 / sqrt(2 * pi * sigma^2)) * e^(-x^2 / (2 * sigma^2))
    
    Where sigma = 4.0 and x = offset in range [0, 5]
    Normalized to 1 to prevent image darkening by multiplying each weight by:
    1 / sum(all weights)
    */
    float weights[5] = float[]( 0.13465834124289953661305802732548,       
                                0.13051534237555914090930704141833,        
                                0.11883557904592230273554609080014,         
                                0.10164546793794160274995705611009,      
                                0.08167444001912718529866079800870 );

    // Sample the current fragment and apply its weight
    vec4 out_color = texture(Source, clamp(vTexCoord, lower_bound, upper_bound)) * weights[0];

    // Iterate across the offsets in both directions sampling texels
    // and adding their weighted alpha values to the total
    for (int i = 1; i < 5; i++)
    {
        out_color.a += texture(Source, clamp(vTexCoord + vec2(0.0, offsets[i] * texel.y), lower_bound, upper_bound)).a * weights[i];
        out_color.a += texture(Source, clamp(vTexCoord - vec2(0.0, offsets[i] * texel.y), lower_bound, upper_bound)).a * weights[i];
    }

    FragColor = out_color;
}
