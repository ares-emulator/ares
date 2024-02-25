#version 450
/*
   Author: Pokefan531
   License: Public domain
*/

// Shader that replicates the LCD dynamics from a GameBoy Advance

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

void main()
{
    gl_Position = global.MVP * Position;
    vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 FragCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


vec3 grayscale(vec3 col)
{
    // Non-conventional way to do grayscale,
    // but bSNES uses this as grayscale value.
    return vec3(dot(col, vec3(0.2126, 0.7152, 0.0722)));
}

void main()
{
//part 1
    float saturation    = 1.0;
    float Display_gamma = 1.02;
    float CRT_gamma     = 2.4;
    float luminance     = 1.0;

    vec3 gamma  = vec3(CRT_gamma / Display_gamma);
    vec3 res    = texture(Source, vTexCoord).xyz;
    res         = mix(grayscale(res), res, saturation); // Apply saturation
    res         = pow(res, gamma.rgb); // Apply gamma
    vec4 c      = vec4(clamp(res * luminance, 0.0, 1.0), 1.0);

//part 2
    float r = c.x;
    float g = c.y;
    float b = c.z;
    float a = c.w;
    float w = r * 0.714 + g * 0.251 + b * 0.000;
    float q = r * 0.071 + g * 0.643 + b * 0.216;
    float e = r * 0.071 + g * 0.216 + b * 0.643;

//part 3
    saturation      = 1.0;
    Display_gamma   = 3.6;
    CRT_gamma       = 2.4;
    luminance       = 1.01;

    res     = vec3(w, q, e);
    gamma   = gamma = vec3(CRT_gamma / Display_gamma);
    res     = mix(grayscale(res), res, saturation); // Apply saturation
    res     = pow(res, gamma.rgb); // Apply gamma
    
    FragColor = vec4(clamp(res * luminance, 0.0, 1.0), 1.0);
}
