#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define SamplerColorVHS Source

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
layout(set = 0, binding = 2) uniform sampler2D Source;

#include "VHSPro_functions.inc"

void main()
{
    const float t = float(params.FrameCount) * 0.25;//_Time.y;                    
    vec2 p = txcoord.xy;    
    float SLN = screenLinesNum; //TODO use only SLN

    //basically if its 0 -> set it to fullscreen
    if(SLN==0.0) SLN = _ScreenParams.y;

     //TODO maybe make it based on num of lines? and height ? 
    //TODO make switch between real pixels and pixelated pixels!
    // ONE_X = 1.0 / screenLinesNum;

    float ONE_X = 1.0 / _ScreenParams.x;  // 1px
    ONE_X *= bleedAmount; // longer tails, more bleeding, default 1.                    

    //distortions
    if (VHS_FishEye){
        p = fishEye(p, fisheyeSize, fisheyeBend); // p = fishEye(p, 1.2, 2.0);
        // return vec4( fract(p*5.0)/2.5, 1.0, 1.0); //debug
    }

    int bleedLength = 21;

    if((VHS_BleedMode == 0 || VHS_BleedMode == 1 || VHS_BleedMode == 3))
    {
        bleedLength = 25;
    }
                      
    if((VHS_BleedMode == 2 || VHS_BleedMode == 4))
    {
        bleedLength = 32;
    }
     
#include "taps.inc"

    //Bleeding
    vec3 signal = vec3(0.0,0.0,0.0);

    if (VHS_Bleed){
                        
        vec3 norm =     vec3(0.0,0.0,0.0);
        vec3 adj =     vec3(0.0,0.0,0.0);
        
        const int taps = bleedLength-4;
        //int taps = bleedLength; //RetroArch

        for (int ii = 0; float(ii) < mod(float(taps) , float(1023)); ii++){

            const float offset = float(ii);
            const vec3 sums =     fetch_offset(offset - float(taps), ONE_X) +
                            fetch_offset(float(taps) - offset, ONE_X) ;

            adj = vec3(luma_filter[ii+3], chroma_filter[ii], chroma_filter[ii]);
            //adj = vec3(luma_filter[ii], chroma_filter[ii], chroma_filter[ii]); //RetroArch

            signal += sums * adj;
            norm += adj;
                            
        }

        adj = vec3(luma_filter[taps], chroma_filter[taps], chroma_filter[taps]);

        signal += t2d(fixCoord) * adj;
        norm += adj;
        signal = signal / norm;
    } else {
        //no bleeding
        signal = t2d(p);
    }

    //[Signal Tweak]
    if (VHS_SignalTweak){
                        
        //adjust
        signal.x += signalAdjustY; 
        signal.y += signalAdjustI; 
        signal.z += signalAdjustQ; 

        //shift
        signal.x *= signalShiftY; 
        signal.y *= signalShiftI; 
        signal.z *= signalShiftQ; 

        // //test YIQ
        // signal.x = 0.5;
        // signal.y = p.x*2.0-1.0;
        // signal.z = p.y*2.0-1.0;

        //signal noise
        // vec2 noise = n4rand_bw( p,t, 1.0-signalNoisePower )*signalNoiseAmount ; 
        // signal.y *= noise.x;
        // signal.z *= noise.y;


        //phase - it cant be dont coz of intervals
        // signal.x = fract( (signal.x +1.0)*0.5f )*2.0f - 1.0f ;
        //signal.y = fract( (signal.y +1.0)*0.5f )*2.0f - 1.0f ;
        //signal.z = fract( (signal.z +1.0)*0.5f )*2.0f - 1.0f ;
    }
                    
    vec3 rgb = yiq2rgb(signal);                    
                   
    if (VHS_SignalTweak){
        if(gammaCorection!=1.0) rgb = pow(abs(rgb), vec3(gammaCorection)); //vec3(gammaCorection).rgb
    }

    //cut trash after fish eye
    // if( p.x<0. || p.x>1. || p.y<0. || p.y>1.){
    //     rgb *= 0.0;
    // }

    //TODO maybe on yiq channel                    
    if (VHS_Vignette){
        rgb *= vignette(p, t*vignetteSpeed); //TODO params //txcoord.xy
    }

    FragColor = vec4(rgb, 1.0); 
}