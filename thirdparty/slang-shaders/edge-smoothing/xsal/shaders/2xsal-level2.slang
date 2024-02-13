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

/*    VERTEX_SHADER    */
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

void main()
{
    vec2 tex = vTexCoord;   
    //vec2 texsize = IN.texture_size;
    float dx = 0.25*params.SourceSize.z;
    float dy = 0.25*params.SourceSize.w;
    vec3  dt = vec3(1.0, 1.0, 1.0);

    vec4 yx = vec4(dx, dy, -dx, -dy);
    vec4 xh = yx*vec4(3.0, 1.0, 3.0, 1.0);
    vec4 yv = yx*vec4(1.0, 3.0, 1.0, 3.0);

    vec3 c11 = texture(Source, tex        ).xyz;    
    vec3 s00 = texture(Source, tex + yx.zw).xyz; 
    vec3 s20 = texture(Source, tex + yx.xw).xyz; 
    vec3 s22 = texture(Source, tex + yx.xy).xyz; 
    vec3 s02 = texture(Source, tex + yx.zy).xyz;
    vec3 h00 = texture(Source, tex + xh.zw).xyz; 
    vec3 h20 = texture(Source, tex + xh.xw).xyz; 
    vec3 h22 = texture(Source, tex + xh.xy).xyz; 
    vec3 h02 = texture(Source, tex + xh.zy).xyz;
    vec3 v00 = texture(Source, tex + yv.zw).xyz; 
    vec3 v20 = texture(Source, tex + yv.xw).xyz; 
    vec3 v22 = texture(Source, tex + yv.xy).xyz; 
    vec3 v02 = texture(Source, tex + yv.zy).xyz;     

    float m1 = 1.0/(dot(abs(s00 - s22), dt) + 0.00001);
    float m2 = 1.0/(dot(abs(s02 - s20), dt) + 0.00001);
    float h1 = 1.0/(dot(abs(s00 - h22), dt) + 0.00001);
    float h2 = 1.0/(dot(abs(s02 - h20), dt) + 0.00001);
    float h3 = 1.0/(dot(abs(h00 - s22), dt) + 0.00001);
    float h4 = 1.0/(dot(abs(h02 - s20), dt) + 0.00001);
    float v1 = 1.0/(dot(abs(s00 - v22), dt) + 0.00001);
    float v2 = 1.0/(dot(abs(s02 - v20), dt) + 0.00001);
    float v3 = 1.0/(dot(abs(v00 - s22), dt) + 0.00001);
    float v4 = 1.0/(dot(abs(v02 - s20), dt) + 0.00001);

    vec3 t1 = 0.5*(m1*(s00 + s22) + m2*(s02 + s20))/(m1 + m2);
    vec3 t2 = 0.5*(h1*(s00 + h22) + h2*(s02 + h20) + h3*(h00 + s22) + h4*(h02 + s20))/(h1 + h2 + h3 + h4);
    vec3 t3 = 0.5*(v1*(s00 + v22) + v2*(s02 + v20) + v3*(v00 + s22) + v4*(v02 + s20))/(v1 + v2 + v3 + v4);

    float k1 = 1.0/(dot(abs(t1 - c11), dt) + 0.00001);
    float k2 = 1.0/(dot(abs(t2 - c11), dt) + 0.00001);
    float k3 = 1.0/(dot(abs(t3 - c11), dt) + 0.00001);

    FragColor = vec4((k1*t1 + k2*t2 + k3*t3)/(k1 + k2 + k3), 1.0);
}
