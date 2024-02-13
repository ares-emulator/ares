#version 450

/*
Adapted for RetroArch from Flyguy's "Apple II-Like Artifact Colors" from shadertoy:
https://www.shadertoy.com/view/llyGzR

"Colors created through NTSC artifacting on 4-bit patterns, similar to the Apple II's lo-res mode."
*/ 

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float F_COL;
} params;

#pragma parameter F_COL "F Col" 0.25 0.25 0.5 0.25

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
   vTexCoord = TexCoord * 1.0004;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//Modulator

//#define F_COL (1.0 / 4.0) // moved to parameter

float tau = atan(1.0)*8.0;

mat3 rgb2yiq = mat3(0.299, 0.596, 0.211,
                    0.587,-0.274,-0.523,
                    0.114,-0.322, 0.312);

//Complex oscillator, Fo = Oscillator freq., Fs = Sample freq., n = Sample index
vec2 Oscillator(float Fo, float Fs, float n)
{
    float phase = (tau*Fo*floor(n))/Fs;
    return vec2(cos(phase),sin(phase));
}

void main()
{
    float Fs = params.SourceSize.x;
    float Fcol = Fs * params.F_COL;
    float n = floor(vTexCoord.x * params.OutputSize.x);
    
    vec3 cRGB = texture(Source, vTexCoord.xy).rgb;
    vec3 cYIQ = rgb2yiq * cRGB;
    
    vec2 cOsc = Oscillator(Fcol, Fs, n);
    
    float sig = cYIQ.x + dot(cOsc, cYIQ.yz);

   FragColor = vec4(sig,0.,0.,0.);
}