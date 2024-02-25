#version 450

// Analog Overshoot
// by Astherix

// adapted for slang from NTSC Codec (w/Overshoot & Noise) shadertoy
// https://www.shadertoy.com/view/flG3zd

// Licensed LGPLv3

// This pass applies a simulation of analog overshoot, which
// is the cause for ghosting artifacts around edges (sharp transitions)
// This is not necessary for encoding/decoding.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float hermite_tension, hermite_bias, decode_chroma_luma_separately;
} params;

// These two define the tension and bias parameters
// of the Hermite interpolation function
// Values closer to 0.5 on both parameters yield
// more aggressive ghosting artifacts
#pragma parameter hermite_tension "Hermite Tension" 0.25 0.01 1.0 0.01
#pragma parameter hermite_bias "Hermite Bias" 0.75 0.01 1.0 0.01

#pragma parameter decode_chroma_luma_separately "Decode Chroma and Luma Separately" 1.0 0.0 1.0 1.0
#define DECODE_CHROMA_LUMA_SEPARATELY params.decode_chroma_luma_separately

#define HERMITE_TENSION params.hermite_tension
#define HERMITE_BIAS params.hermite_bias

#define iChannel0 Source
#define iResolution params.OutputSize.xy
#define fragCoord (vTexCoord.xy * params.OutputSize.xy)
#define fragColor FragColor
#define iFrame params.FrameCount
#define iTime (params.FrameCount / 60)

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
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

float hermite(float y0, float y1, float y2, float y3, float m, float tension, float bias) {
    float m2 = m*m,
          m3 = m2*m,
          m0 =      (y1-y0)*(1.0+bias)*(1.0-tension)/2.0;
          m0 = m0 + (y2-y1)*(1.0-bias)*(1.0-tension)/2.0;
    float m1 =      (y2-y1)*(1.0+bias)*(1.0-tension)/2.0;
          m1 = m1 + (y3-y2)*(1.0-bias)*(1.0-tension)/2.0;
    float a0 =  2.0*m3 - 3.0*m2 + 1.0,
          a1 =      m3 - 2.0*m2 + m,
          a2 =      m3 -     m2,
          a3 = -2.0*m3 + 3.0*m2;
     
     return (a0*y1+a1*m0+a2*m1+a3*y2);
}

void main()
{
   vec2 px = vTexCoord.xy * params.OutputSize.zw;
    // Normalized pixel coordinates (from 0 to 1)
    vec2 fcm1 = vec2(fragCoord.x-4.0, fragCoord.y)/iResolution.xy,
         fcm0 = vec2(fragCoord.x-0.0, fragCoord.y)/iResolution.xy,
         fcp1 = vec2(fragCoord.x+4.0, fragCoord.y)/iResolution.xy,
         fcp2 = vec2(fragCoord.x+8.0, fragCoord.y)/iResolution.xy;
    
    vec3 y0 = texture(Source, fcm1).xyz,
         y1 = texture(Source, fcm0).xyz,
         y2 = texture(Source, fcp1).xyz,
         y3 = texture(Source, fcp2).xyz;
    
    vec3 o = y1;
         
    float l = HERMITE_TENSION, y = HERMITE_BIAS;
         
    vec3 f = vec3(
        hermite(y0.x, y1.x, y2.x, y3.x, l, y, y * 50.0),
        hermite(y0.y, y1.y, y2.y, y3.y, l, y, y * 50.0),
        hermite(y0.z, y1.z, y2.z, y3.z, l, y, y * 50.0)
    );
    
    fragColor = mix(vec4(f, 1.0), vec4(y1, 1.0), DECODE_CHROMA_LUMA_SEPARATELY);
}  
