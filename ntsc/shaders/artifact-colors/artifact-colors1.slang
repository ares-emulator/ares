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
	float FIR_SIZE;
	float F_COL;
	float F_LUMA_LP;
	float F_COL_BW;
} params;

#pragma parameter FIR_SIZE "FIR Size" 29.0 1.0 50.0 1.0
#pragma parameter F_COL "F Col" 0.25 0.25 0.5 0.25
#pragma parameter F_LUMA_LP "F Luma LP" 0.16667 0.0001 0.333333 0.02
#pragma parameter F_COL_BW "F Col BW" 50.0 10.0 200.0 1.0

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
#pragma format R16G16B16A16_SFLOAT
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

//Demodulator

//#define F_COL (1.0 / 4.0) // moved to parameter
//#define F_LUMA_LP (1.0 / 6.0) // moved to parameter
//#define F_COL_BW (1.0 / 50.0) // moved to parameter


//#define FIR_SIZE 29 // moved to parameter

float pi = 3.141592654;//atan(1.0)*4.0;
float tau = 6.283185308;//atan(1.0)*8.0;

//Non-normalized texture sampling.
vec4 sample2D(sampler2D tex,vec2 resolution, vec2 uv)
{
    return texture(tex, uv / resolution);
}

//Complex multiply
vec2 cmul(vec2 a, vec2 b)
{
   return vec2((a.x * b.x) - (a.y * b.y), (a.x * b.y) + (a.y * b.x));
}

float sinc(float x)
{
	return (x == 0.0) ? 1.0 : sin(x*pi)/(x*pi);   
}

//https://en.wikipedia.org/wiki/Window_function
float WindowBlackman(float a, int N, int i)
{
    float a0 = (1.0 - a) / 2.0;
    float a1 = 0.5;
    float a2 = a / 2.0;
    
    float wnd = a0;
    wnd -= a1 * cos(2.0 * pi * (float(i) / float(N - 1)));
    wnd += a2 * cos(4.0 * pi * (float(i) / float(N - 1)));
    
    return wnd;
}

//FIR lowpass filter 
//Fc = Cutoff freq., Fs = Sample freq., N = # of taps, i = Tap index
float Lowpass(float Fc, float Fs, int N, int i)
{    
    float wc = (Fc/Fs);
    
    float wnd = WindowBlackman(0.16, N, i);
    
    return 2.0*wc * wnd * sinc(2.0*wc * float(i - N/2));
}

//FIR bandpass filter 
//Fa/Fb = Low/High cutoff freq., Fs = Sample freq., N = # of taps, i = Tap index
float Bandpass(float Fa, float Fb, float Fs, int N, int i)
{    
    float wa = (Fa/Fs);
    float wb = (Fb/Fs);
    
    float wnd = WindowBlackman(0.16, N, i);
    
    return 2.0*(wb-wa) * wnd * (sinc(2.0*wb * float(i - N/2)) - sinc(2.0*wa * float(i - N/2)));
}

//Complex oscillator, Fo = Oscillator freq., Fs = Sample freq., n = Sample index
vec2 Oscillator(float Fo, float Fs, float N)
{
    float phase = (tau*Fo*floor(N))/Fs;
    return vec2(cos(phase),sin(phase));
}

void main()
{
    float Fs = params.SourceSize.x;
    float Fcol = Fs * params.F_COL;
    float Fcolbw = Fs * (1.0 / params.F_COL_BW);
    float Flumlp = Fs * params.F_LUMA_LP;
    float n = floor(vTexCoord.x * params.OutputSize.x);
    
    float y_sig = 0.0;    
    float iq_sig = 0.0;
    
    vec2 cOsc = Oscillator(Fcol, Fs, n);
	
    n += float(params.FIR_SIZE)/2.0;
    
    //Separate luma(Y) & chroma(IQ) signals
    for(int i = 0;i < params.FIR_SIZE;i++)
    {
        int tpidx = int(params.FIR_SIZE) - i - 1;
        float lp = Lowpass(Flumlp, Fs, int(params.FIR_SIZE), tpidx);
        float bp = Bandpass(Fcol - Fcolbw, Fcol + Fcolbw, Fs, int(params.FIR_SIZE), tpidx);
        
        y_sig += sample2D(Source, params.SourceSize.xy, vec2(n - float(i), (vTexCoord.y * params.OutputSize.y))).r * lp;
        iq_sig += sample2D(Source, params.SourceSize.xy, vec2(n - float(i), (vTexCoord.y * params.OutputSize.y))).r * bp;
    }
    
    //Shift IQ signal down from Fcol to DC 
    vec2 iq_sig_mix = cmul(vec2(iq_sig, 0.), cOsc);
    
   FragColor = vec4(y_sig, iq_sig_mix, 0.);
}