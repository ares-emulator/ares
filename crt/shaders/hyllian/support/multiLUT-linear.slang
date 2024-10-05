#version 450

// A version of the LUT shader that loads 2 LUTs.
// Can turn LUT off too.

layout(push_constant) uniform Push
{
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
    uint FrameCount;
    float LUT_selector_param;
    float H_InputGamma;
} params;

#pragma parameter LUT_selector_param "LUT [ Off | Grade RGB | Grade Composite ]" 1.0 0.0 2.0 1.0
#pragma parameter H_InputGamma "INPUT GAMMA" 2.4 1.0 3.0 0.05

#define H_InputGamma params.H_InputGamma

#define GAMMA_IN(color)     pow(color, vec3(H_InputGamma, H_InputGamma, H_InputGamma))

int LUT_selector = int(params.LUT_selector_param);

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
   vTexCoord = TexCoord;
}

#pragma stage fragment
#pragma format R8G8B8A8_SRGB
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D SamplerLUT1;
layout(set = 0, binding = 4) uniform sampler2D SamplerLUT2;

// This shouldn't be necessary but it seems some undefined values can
// creep in and each GPU vendor handles that differently. This keeps
// all values within a safe range
vec3 mixfix(vec3 a, vec3 b, float c)
{
    return (a.z < 1.0) ? mix(a, b, c) : a;
}

void main()
{
    vec3 imgColor = texture(Source, vTexCoord.xy).rgb;
    vec3 color;

    if (LUT_selector == 0.0) {
        color = imgColor;
    }
    else {

        float LUT_Size = mix(textureSize(SamplerLUT1, 0).y, textureSize(SamplerLUT2, 0).y, params.LUT_selector_param - 1.0);
        vec3  color1, color2 = vec3(0.,0.,0.);
        float red, green, blue1, blue2, mixer = 0.0;
    
        red   = ( imgColor.r * (LUT_Size - 1.0) + 0.4999 ) / (LUT_Size * LUT_Size);
        green = ( imgColor.g * (LUT_Size - 1.0) + 0.4999 ) / LUT_Size;
        blue1 = (floor( imgColor.b  * (LUT_Size - 1.0) ) / LUT_Size) + red;
        blue2 = ( ceil( imgColor.b  * (LUT_Size - 1.0) ) / LUT_Size) + red;
        mixer = clamp(max((imgColor.b - blue1) / (blue2 - blue1), 0.0), 0.0, 32.0);
    
        if(LUT_selector == 1)
        {
            color1 = texture( SamplerLUT1, vec2( blue1, green )).rgb;
            color2 = texture( SamplerLUT1, vec2( blue2, green )).rgb; 
        }
        else
        {
            color1 = texture( SamplerLUT2, vec2( blue1, green )).rgb;
            color2 = texture( SamplerLUT2, vec2( blue2, green )).rgb;
        }

        color = mixfix(color1, color2, mixer);
    }

    FragColor = vec4(GAMMA_IN(color), 1.0);
}
