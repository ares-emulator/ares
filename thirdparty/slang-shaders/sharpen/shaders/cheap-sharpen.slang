#version 450

/*
   Hyllian cheap sharpen Shader
  
   Copyright (C) 2011-2024 Hyllian - sergiogdb@gmail.com

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
    float CS_SHARPNESS;
} params;

#pragma parameter CS_SHARPNESS "Sharpness" 0.4 0.0 1.0 0.1  

#define CS_SHARPNESS params.CS_SHARPNESS

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 t1;
layout(location = 2) out vec4 t2;

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
    float dx = params.SourceSize.z;
    float dy = params.SourceSize.w;

    t1 = vTexCoord.xyyy + vec4( 0.0, -dy, 0.0,  dy); //  B E H
    t2 = vTexCoord.xxxy + vec4( -dx, 0.0,  dx, 0.0); //  D E F
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 t1;
layout(location = 2) in vec4 t2;
layout(location = 3) in vec4 t3;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


/*       
              B   
           D  E  F 
              H   
*/

void main()
{
    vec3   B = texture(Source, t1.xy).xyz;
    vec3   D = texture(Source, t2.xw).xyz;
    vec3   E = texture(Source, t2.yw).xyz;
    vec3   F = texture(Source, t2.zw).xyz;
    vec3   H = texture(Source, t1.xw).xyz;

    vec3 color = (1.0 + CS_SHARPNESS) * E - 0.25 * CS_SHARPNESS * (B + D + F + H);

    // Anti-ringing
    //  Get min/max samples
    vec3 min_sample = min(min(E,min(D,F)), min(B,H));
    vec3 max_sample = max(max(E,max(D,F)), max(B,H));

    vec3 aux = color;
    color = clamp(color, min_sample, max_sample);
    color = mix(aux, color, step(-0.00001, (E-D)*(F-E) + (E-B)*(H-E)));

    FragColor = vec4(color, 1.0);
}
