#version 450
/* Derived from https://github.com/libretro/slang-shaders/blob/master/motionblur/motionblur-blue.slang
   To increase the effect, uncomment more OriginalHistory# lines in each color channel
*/

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
    vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D OriginalHistory2;
layout(set = 0, binding = 5) uniform sampler2D OriginalHistory3;
layout(set = 0, binding = 6) uniform sampler2D OriginalHistory4;
layout(set = 0, binding = 7) uniform sampler2D OriginalHistory5;
layout(set = 0, binding = 8) uniform sampler2D OriginalHistory6;
layout(set = 0, binding = 9) uniform sampler2D OriginalHistory7;

void main()
{
    float red_result = 
        texture(OriginalHistory1, vTexCoord).r +
        texture(OriginalHistory2, vTexCoord).r /*+
        texture(OriginalHistory3, vTexCoord).r +
        texture(OriginalHistory4, vTexCoord).r +
        texture(OriginalHistory5, vTexCoord).r +
        texture(OriginalHistory6, vTexCoord).r +
        texture(OriginalHistory7, vTexCoord).r*/;

    float green_result = 
        texture(OriginalHistory1, vTexCoord).g +
        texture(OriginalHistory2, vTexCoord).g /*+
        texture(OriginalHistory3, vTexCoord).g +
        texture(OriginalHistory4, vTexCoord).g +
        texture(OriginalHistory5, vTexCoord).g +
        texture(OriginalHistory6, vTexCoord).g +
        texture(OriginalHistory7, vTexCoord).g*/;

    float blue_result = 
        texture(OriginalHistory1, vTexCoord).b +
        texture(OriginalHistory2, vTexCoord).b /*+
        texture(OriginalHistory3, vTexCoord).b +
        texture(OriginalHistory4, vTexCoord).b +
        texture(OriginalHistory5, vTexCoord).b +
        texture(OriginalHistory6, vTexCoord).b +
        texture(OriginalHistory7, vTexCoord).b*/;


    blue_result = clamp(blue_result, 0.0, 1.0);
    
    FragColor = 
        clamp(texture(Source, vTexCoord) + 0.4 * 
            vec4(clamp(red_result   - 7.0 * texture(Source, vTexCoord).r, 0.0, 1.0),
                 clamp(green_result - 7.0 * texture(Source, vTexCoord).g, 0.0, 1.0),
                 clamp(blue_result  - 7.0 * texture(Source, vTexCoord).b, 0.0, 1.0),
                 1.0),
            0.0, 
            1.0
        );
}
