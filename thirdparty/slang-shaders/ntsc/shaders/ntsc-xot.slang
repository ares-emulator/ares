#version 450

//  NTSC Decoder
//
//  Decodes composite video signal generated in Buffer A.
//  Move mouse to display the original signal.
//
//  This is an intensive shader with a lot of sampling and 
//  iterated filtering. Reduce filter width N to trade quality 
//  for performance. N should be an integer multiple of four, 
//  plus one (4n+1). Apologies to owners of melted phones.
//
//  hunterk made the shader work in RGB instead of just a single
//  channel, though there's probably better ways to do it than
//  just tripling all of the operations. Improvements are welcome!
//
//  copyright (c) 2017, John Leffingwell
//  license CC BY-SA Attribution-ShareAlike
//  adapted for RetroAch by hunterk from this shadertoy:
//  https://www.shadertoy.com/view/Mdffz7

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

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
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define PI   3.14159265358979323846
#define TAU  6.28318530717958647693

//  TV adjustments
const float SAT = 1.0;      //  Saturation / "Color" (normally 1.0)
const float HUE = 1.0;      //  Hue / "Tint" (normally 0.0)
const float BRI = 1.0;      //  Brightness (normally 1.0)

//  Filter parameters
const int   N  = 21;        //  Filter Width (4n+1)
const float FC = 0.125;     //  Frequency Cutoff


const mat3 YIQ2RGB = mat3(1.000, 1.000, 1.000,
                          0.956,-0.272,-1.106,
                          0.621,-0.647, 1.703);

vec3 adjust(vec3 YIQ, float H, float S, float B) {
    mat3 M = mat3(  B,      0.0,      0.0,
                  0.0, S*cos(H),  -sin(H), 
                  0.0,   sin(H), S*cos(H) );
    return M * YIQ;
}

float sinc(float n) {
    if (n == 0.0) return 1.0;
	return sin(PI*n) / (PI*n);
}

float window_blackman(float n, float N) {
	return 0.42 - 0.5 * cos((2.0*PI*n)/(N-1.0)) + 0.08 * cos((4.0*PI*n)/(N-1.0));
}

float pulse(float a, float b, float x) {
    return step(a, x) * step(x, b);
}

void main()
{
	vec2 size = params.SourceSize.xy;
	vec2 uv = vTexCoord.xy;
    
    //  Compute sampling offsets and weights
    float sumR = 0.0;
	float sumG = 0.0;
	float sumB = 0.0;
    vec4 offsetR[N];
	vec4 offsetG[N];
	vec4 offsetB[N];

	//  R
    for (int i=0; i<N; i++) {
        float jR = float(i) - (float(N-1)/2.0);
        float kR = sinc( 2.0 * FC * jR) * window_blackman(float(i),float(N));
        offsetR[i] = vec4(jR/size.x, 0.0, kR, -kR);
        sumR += kR;
    }
	
    //  Low-pass filter input signal
    float tapR[N];
    for (int i=0; i<N; i++) {
        offsetR[i].zw /= sumR;
        tapR[i] = pulse(0.0, 1.0, uv.x + offsetR[i].x) * texture(Source, uv + offsetR[i].xy).r;
    }
    offsetR[(N-1)/2].w += 1.0;
    
    //  Extract luma signal
    float lumaR = 0.0;
    for (int i=0; i<N; i++) {
        lumaR += tapR[i] * offsetR[i].z;
    }
    
    //  Extract chroma signal
    float chromaR[N];
    for (int j=0; j<N; j++) {
        chromaR[j] = 0.0;
	    for (int i=0; i<N; i++) {
	   	    chromaR[j] += tapR[i+j-(N-1)/2] * offsetR[i].w;
    	}
    }
	
	//  G
    for (int i=0; i<N; i++) {
        float jG = float(i) - (float(N-1)/2.0);
        float kG = sinc( 2.0 * FC * jG) * window_blackman(float(i),float(N));
        offsetG[i] = vec4(jG/size.x, 0.0, kG, -kG);
        sumG += kG;
    }
	
    //  Low-pass filter input signal
    float tapG[N];
    for (int i=0; i<N; i++) {
        offsetG[i].zw /= sumG;
        tapG[i] = pulse(0.0, 1.0, uv.x + offsetG[i].x) * texture(Source, uv + offsetG[i].xy).g;
    }
    offsetG[(N-1)/2].w += 1.0;
    
    //  Extract luma signal
    float lumaG = 0.0;
    for (int i=0; i<N; i++) {
        lumaG += tapG[i] * offsetG[i].z;
    }
    
    //  Extract chroma signal
    float chromaG[N];
    for (int j=0; j<N; j++) {
        chromaG[j] = 0.0;
	    for (int i=0; i<N; i++) {
	   	    chromaG[j] += tapG[i+j-(N-1)/2] * offsetG[i].w;
    	}
    }
	
	//  B
    for (int i=0; i<N; i++) {
        float jB = float(i) - (float(N-1)/2.0);
        float kB = sinc( 2.0 * FC * jB) * window_blackman(float(i),float(N));
        offsetB[i] = vec4(jB/size.x, 0.0, kB, -kB);
        sumB += kB;
    }
	
    //  Low-pass filter input signal
    float tapB[N];
    for (int i=0; i<N; i++) {
        offsetB[i].zw /= sumB;
        tapB[i] = pulse(0.0, 1.0, uv.x + offsetB[i].x) * texture(Source, uv + offsetB[i].xy).b;
    }
    offsetB[(N-1)/2].w += 1.0;
    
    //  Extract luma signal
    float lumaB = 0.0;
    for (int i=0; i<N; i++) {
        lumaB += tapB[i] * offsetB[i].z;
    }
    
    //  Extract chroma signal
    float chromaB[N];
    for (int j=0; j<N; j++) {
        chromaB[j] = 0.0;
	    for (int i=0; i<N; i++) {
	   	    chromaB[j] += tapB[i+j-(N-1)/2] * offsetB[i].w;
    	}
    }
    
    //  Generate YIQ signal R
    float YR = lumaR;
    float IR = 0.0;
    float QR = 0.0;
    for (int j=0; j<N; j++) {
        float subcarrierR = TAU * 0.25 * size.x * (uv.s + float(j+(N-1)/2)/size.x);
        IR += cos(subcarrierR) * chromaR[j] * offsetR[j].z;
    	QR += sin(subcarrierR) * chromaR[j] * offsetR[j].z;
    }
	
	    //  Generate YIQ signal G
    float YG = lumaG;
    float IG = 0.0;
    float QG = 0.0;
    for (int j=0; j<N; j++) {
        float subcarrierG = TAU * 0.25 * size.x * (uv.s + float(j+(N-1)/2)/size.x);
        IG += cos(subcarrierG) * chromaG[j] * offsetG[j].z;
    	QG += sin(subcarrierG) * chromaG[j] * offsetG[j].z;
    }
	
	    //  Generate YIQ signal B
    float YB = lumaB;
    float IB = 0.0;
    float QB = 0.0;
    for (int j=0; j<N; j++) {
        float subcarrierB = TAU * 0.25 * size.x * (uv.s + float(j+(N-1)/2)/size.x);
        IB += cos(subcarrierB) * chromaB[j] * offsetB[j].z;
    	QB += sin(subcarrierB) * chromaB[j] * offsetB[j].z;
    }
    
        //  Apply TV adjustments to YIQ signal and convert to RGB
    	FragColor.r = (YIQ2RGB * adjust(vec3(YR, IR, QR), HUE, SAT, BRI)).r;
		FragColor.g = (YIQ2RGB * adjust(vec3(YG, IG, QG), HUE, SAT, BRI)).g;
		FragColor.b = (YIQ2RGB * adjust(vec3(YB, IB, QB), HUE, SAT, BRI)).b;
}