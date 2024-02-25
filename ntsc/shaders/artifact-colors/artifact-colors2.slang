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
	float SATURATION;
	float BRIGHTNESS;
	float F_LUMA_LP;
	float HUE;
	float split;
	float split_line;
} params;

#pragma parameter FIR_SIZE "FIR Size" 29.0 1.0 50.0 1.0
#pragma parameter F_COL "F Col" 0.25 0.25 0.5 0.25
#pragma parameter SATURATION "Saturation" 30.0 0.0 100.0 1.0
#pragma parameter BRIGHTNESS "Brightness" 1.0 0.0 2.0 0.01
#pragma parameter F_LUMA_LP "F Luma LP" 0.16667 0.0001 0.333333 0.02
#pragma parameter HUE "Hue" 0.0 0.0 1.0 0.01
#pragma parameter split "Split Toggle" 0.0 0.0 1.0 1.0
#pragma parameter split_line "Split Line Location" 0.5 0.0 1.0 0.05

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
layout(set = 0, binding = 3) uniform sampler2D artifact_colors_refpass;
layout(set = 0, binding = 4) uniform sampler2D Pass2;
layout(set = 0, binding = 5) uniform sampler2D Pass1;

//Composite color artifact simulator
//Change Buf A to change the input image.

//#define HUE 0.0 // moved to parameter
//#define SATURATION 30.0 // moved to parameter
//#define BRIGHTNESS 1.0 // moved to parameter

#define COMPOSITE 0 //Composite demodulated image
#define RGB 1 //Raw RGB input image
#define LUMA 2 //Luma component
#define CHROMA 3 //Chroma component
#define SIGNAL 4 //Modulated image
#define SPLIT 5 //Left = Input RGB, Right = Output composite

#define VIEW_MODE COMPOSITE

//#define F_COL (1.0 / 4.0) // moved to parameter
//#define F_LUMA_LP (1.0 / 6.0) // moved to parameter

//#define FIR_SIZE 29  // moved to parameter

float pi = 3.141592654;//atan(1.0)*4.0;
float tau = 6.283185308;//atan(1.0)*8.0;

mat3 yiq2rgb = mat3(1.000, 1.000, 1.000,
                    0.956,-0.272,-1.106,
                    0.621,-0.647, 1.703);

//Angle -> 2D rotation matrix 
mat2 rotate(float a)
{
    return mat2( cos(a), sin(a),
                -sin(a), cos(a));
}

//Non-normalized texture sampling.
vec4 sample2D(sampler2D tex,vec2 resolution, vec2 uv)
{
    return texture(tex, uv / resolution);
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

void main()
{
    float Fs = params.SourceSize.x;
    float Fcol = Fs * params.F_COL;
    float Flumlp = Fs * params.F_LUMA_LP;
    float n = floor(vTexCoord.x * params.OutputSize.x);
    
	vec2 uv = vTexCoord.xy * params.OutputSize.xy;
    
    float luma = sample2D(Source, params.SourceSize.xy, uv).r;
    vec2 chroma = vec2(0.);
    
    //Filtering out unwanted high freqency content from the chroma(IQ) signal.
    for(int i = 0;i < int(params.FIR_SIZE);i++)
    {
        int tpidx = int(params.FIR_SIZE) - i - 1;
        float lp = Lowpass(Flumlp, Fs, int(params.FIR_SIZE), tpidx);
        chroma += sample2D(Source, params.SourceSize.xy, uv - vec2(float(i) - params.FIR_SIZE / 2., 0.)).yz * lp;
    }
    
    chroma *= rotate(tau * params.HUE);
    
    vec3 color = yiq2rgb * vec3(params.BRIGHTNESS * luma, chroma * params.SATURATION);
    
    #if(VIEW_MODE == COMPOSITE)
    	FragColor = vec4(color, 0.);
    
    #elif(VIEW_MODE == RGB)
   		FragColor = texture(artifact_colors_refpass, uv / params.SourceSize.xy);
    
    #elif(VIEW_MODE == LUMA) 
    	FragColor = vec4(luma);
    
    #elif(VIEW_MODE == CHROMA)
    	FragColor = vec4(40.0*chroma+0.5,0.,0.);
    
    #elif(VIEW_MODE == SIGNAL)
    	FragColor = 0.5 * texture(Pass2, uv / params.SourceSize.xy).rrrr+0.25;
	#endif
    
    	if(params.split > 0.5)
        {
            FragColor = vec4(texture(Pass1, uv / params.SourceSize.xy).r);
        }
    		FragColor = (vTexCoord.x > params.split_line) ? vec4(color, 0.) : FragColor;
    
}
