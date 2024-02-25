#version 450

/*
   Hyllian's RetroTiles Shader
   
   Copyright (C) 2011-2022 Hyllian - sergiogdb@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float TILE_FORMAT;
    float TILE_DEPTH;
    float TILE_SIZE;
    float COLOR_BOOST;
    float OVERSCAN;
    float InputGamma;
    float OutputGamma;
} param;

#pragma parameter TILE_FORMAT  "Tile Format [ Normal | Deep]"   0.0  0.0   1.0 1.0
#pragma parameter TILE_DEPTH   "Tile Depth"                     1.0  0.0   1.0 0.1
#pragma parameter TILE_SIZE    "Tile Size"                      0.5  0.4   0.6 0.01
#pragma parameter COLOR_BOOST  "Color Boost"                    1.0  1.0   2.0 0.05
#pragma parameter OVERSCAN     "Over Scan"                      0.0 -3.0   3.0 1.0
#pragma parameter InputGamma   "INPUT GAMMA"                    2.4  0.0   4.0 0.1
#pragma parameter OutputGamma  "OUTPUT GAMMA"                   2.2  0.0   3.0 0.1

#define TILE_FORMAT  param.TILE_FORMAT
#define TILE_DEPTH   param.TILE_DEPTH
#define TILE_SIZE    param.TILE_SIZE
#define COLOR_BOOST  param.COLOR_BOOST
#define OVERSCAN     param.OVERSCAN
#define InputGamma   param.InputGamma
#define OutputGamma  param.OutputGamma

#define GAMMA_IN(color)     pow(color, vec3(InputGamma, InputGamma, InputGamma))
#define GAMMA_OUT(color)    pow(color, vec3(1.0 / OutputGamma, 1.0 / OutputGamma, 1.0 / OutputGamma))


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

    vec2 box_scale = floor(param.OutputSize.xy / param.SourceSize.xy) + OVERSCAN;
    box_scale = vec2(min(box_scale.x, box_scale.y));

    vec2 scale = (param.OutputSize.xy / param.SourceSize.xy) / box_scale;
    vec2 middle = vec2(0.5);
    vec2 diff = TexCoord * 1.0001 - middle;
    vTexCoord = middle + diff * scale;

}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float fade(float size)
{
   size = clamp(size, -TILE_DEPTH, 1.0);

   size = 1.0 - size * size * size * mix(size * size, 1.0, TILE_FORMAT);

   return size * size;
}


void main()
{
    vec2 pos = fract(vTexCoord*param.SourceSize.xy)-vec2(0.5, 0.5); // pos = pixel position
    vec2 dir = sign(pos); // dir = pixel direction

    vec2 g1 = dir*vec2( param.SourceSize.z,  0);
    vec2 g2 = dir*vec2(  0, param.SourceSize.w);

    vec3 A = GAMMA_IN(texture(Source, vTexCoord       ).xyz);
    vec3 B = GAMMA_IN(texture(Source, vTexCoord +g1   ).xyz);
    vec3 C = GAMMA_IN(texture(Source, vTexCoord    +g2).xyz);
    vec3 D = GAMMA_IN(texture(Source, vTexCoord +g1+g2).xyz);

    vec2 dx = vec2(pos.x, 1.0-pos.x) / TILE_SIZE;
    vec2 dy = vec2(pos.y, 1.0-pos.y) / TILE_SIZE;

    vec2 wx = vec2(fade(dx.x), fade(dx.y));
    vec2 wy = vec2(fade(dy.x), fade(dy.y));

    vec3 color = (A*wx.x + B*wx.y)*wy.x + (C*wx.x + D*wx.y)*wy.y;

    color *= COLOR_BOOST;

    FragColor = vec4(GAMMA_OUT(color), 1.0);
}
