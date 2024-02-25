#version 450

/*
    Copyright (C) 2016 guest(r) - guest.r@gmail.com

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
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in  vec4 Position;
layout(location = 1) in  vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

/* Default Vertex shader */
void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in  vec2 vTexCoord;
layout(location = 1) in  vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

struct deltas
{
    vec2 UL, UR, DL, DR;
};

void main()
{
    vec2 texsize = params.SourceSize.xy;
    float dx     = pow(texsize.x, -1.0) * 0.5;
    float dy     = pow(texsize.y, -1.0) * 0.5;
    vec3  dt     = vec3(1.0, 1.0, 1.0);

    deltas VAR = { 
        vTexCoord + vec2(-dx, -dy),
        vTexCoord + vec2( dx, -dy),
        vTexCoord + vec2(-dx,  dy),
        vTexCoord + vec2( dx,  dy)
    };

    vec3 c00 = texture(Source, VAR.UL).xyz;
    vec3 c20 = texture(Source, VAR.UR).xyz;
    vec3 c02 = texture(Source, VAR.DL).xyz;
    vec3 c22 = texture(Source, VAR.DR).xyz;

    float m1 = dot(abs(c00 - c22), dt) + 0.001;
    float m2 = dot(abs(c02 - c20), dt) + 0.001;

    FragColor = vec4((m1*(c02 + c20) + m2*(c22 + c00))/(2.0*(m1 + m2)), 1.0);
}
