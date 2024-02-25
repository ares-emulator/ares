#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define SamplerColorVHS Original

#include "VHSPro_params.inc"
#include "VHSPro_constants.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 txcoord;

void main()
{
   gl_Position = global.MVP * Position;
   txcoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 txcoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Original;

#include "VHSPro_functions.inc"

void main()
{
    const float t = float(params.FrameCount) * 0.025;
    vec2 p = txcoord.xy;
    
    p.y=1-p.y;
    
    const vec2 p_ = p*_ScreenParams;
    
    float ns = 0.0; //signal noise //tex2D(_TapeTex, p);
    float nt = 0.0; //tape noise
    float nl = 0.0; //lines for tape noise
    float ntail = 0.0; //tails values for tape noise 
    
    if (VHS_TapeNoise) {

        //here is normilized p (0..1)
            nl = tapeNoiseLines(p, t*tapeNoiseSpeed)*1.0;//tapeNoiseAmount;
            nt = tapeNoise(nl, p, t*tapeNoiseSpeed)*1.0;//tapeNoiseAmount;
            ntail = hash12(p+ vec2(0.01,0.02) );
    }
    
    if (VHS_LineNoise) {
            ns += lineNoise(p_, t*lineNoiseSpeed)*lineNoiseAmount;
            // col += lineNoise(pn_, t*lineNoiseSpeed)*lineNoiseAmount;//i.uvn
    }
            
    //y noise from yiq
    if (VHS_FilmGrain) {
        //cheap n ok atm
        ns += filmGrain((p_-0.5*_ScreenParams.xy)*0.5, t, filmGrainPower ) * filmGrainAmount;
    }
        
    FragColor = vec4(nt,nl,ns,ntail);
}