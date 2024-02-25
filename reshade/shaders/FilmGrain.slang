#version 450

/**
 * FilmGrain version 1.0
 * by Christian Cann Schuldt Jensen ~ CeeJay.dk
 *
 * Computes a noise pattern and blends it with the image to create a film grain look.
 * ----------------------------------------------------------------------------------
 * Ported from https://github.com/crosire/reshade-shaders/blob/019921117c49beb4d1569af48f33cbb4e13033af/Shaders/FilmGrain.fx
 */

layout(push_constant) uniform Push
{
    /* How visible the grain is. Higher is more visible. */
    float Intensity;

    /* Controls the variance of the Gaussian noise. Lower values look smoother. */
    float Variance;

    /* Affects the brightness of the noise. */
    float Mean;

    /* Higher Signal-to-Noise Ratio values give less grain to brighter pixels. 0 disables this feature. */
    float SignalToNoiseRatio;
} params;

#pragma parameter Intensity "Intensity"   0.50 0.00 1.00 0.01
#pragma parameter Variance  "Variance - lower is smoother"    0.40 0.00 1.00 0.01
#pragma parameter Mean      "Mean - affects brightness"        0.50 0.00 1.00 0.01
#pragma parameter SignalToNoiseRatio "SNR - higher is less grain on brighter pixels." 6.00 0.00 16.00 1.00

layout(std140, set = 0, binding = 0) uniform UBO
{
    mat4 MVP;
    vec4 OutputSize;
    vec4 OriginalSize;
    vec4 SourceSize;
    uint FrameCount;
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
layout(location = 0) in  vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define PI 3.1415927

vec3 FilmGrainPass(vec4 vpos, vec2 texcoord)
{
    vec3 color = texture(Source, texcoord).rgb;

    //float inv_luma = dot(color, vec3(-0.2126, -0.7152, -0.0722)) + 1.0;
    float inv_luma = dot(color, vec3(-1.0/3.0, -1.0/3.0, -1.0/3.0)) + 1.0; //Calculate the inverted luma so it can be used later to control the variance of the grain

    /*---------------------.
    | :: Generate Grain :: |
    '---------------------*/

    /* We use slang's FrameCount uniform variable instead of ReShade's Timer */
    /* We assume frame rate is 60 */
    float t = global.FrameCount * 16.667 * 0.0022337;



    //PRNG 2D - create two uniform noise values and save one DP2ADD
    float seed = dot(texcoord, vec2(12.9898, 78.233));// + t;
    float sine = sin(seed);
    float cosine = cos(seed);
    float uniform_noise1 = fract(sine * 43758.5453 + t); //I just salt with t because I can
    float uniform_noise2 = fract(cosine * 53758.5453 - t); // and it doesn't cost any extra ASM

    //Get settings
    float stn = params.SignalToNoiseRatio < 1.0 ?  1.0 : pow(abs(inv_luma), params.SignalToNoiseRatio); // Signal to noise feature - Brighter pixels get less noise.
    float variance = (params.Variance*params.Variance) * stn;
    float mean = params.Mean;

    //Box-Muller transform
    uniform_noise1 = (uniform_noise1 < 0.0001) ? 0.0001 : uniform_noise1; //fix log(0)

    float r = sqrt(-log(uniform_noise1));
    r = (uniform_noise1 < 0.0001) ? PI : r; //fix log(0) - PI happened to be the right answer for uniform_noise == ~ 0.0000517.. Close enough and we can reuse a constant.
    float theta = (2.0 * PI) * uniform_noise2;

    float gauss_noise1 = variance * r * cos(theta) + mean;
    //float gauss_noise2 = variance * r * sin(theta) + mean; //we can get two gaussians out of it :)

    //gauss_noise1 = (ddx(gauss_noise1) - ddy(gauss_noise1)) * 0.50  + gauss_noise2;


    //Calculate how big the shift should be
    //float grain = mix(1.0 - params.Intensity,  1.0 + params.Intensity, gauss_noise1);
    float grain = mix(1.0 + params.Intensity,  1.0 - params.Intensity, gauss_noise1);

    //float grain2 = (2.0 * params.Intensity) * gauss_noise1 + (1.0 - params.Intensity);

    //Apply grain
    color = color * grain;

    //color = (grain-1.0) *2.0 + 0.5;

    //color = mix(color,colorInput.rgb,sqrt(luma));

    /*-------------------------.
    | :: Debugging features :: |
    '-------------------------*/

    //color.rgb = fract(gauss_noise1).xxx; //show the noise
    //color.rgb = (gauss_noise1 > 0.999) ? vec3(1.0,1.0,0.0) : 0.0 ; //does it reach 1.0?

    return color.rgb;
}


void main()
{
    FragColor.rgb = FilmGrainPass(FragColor, vTexCoord);
}
