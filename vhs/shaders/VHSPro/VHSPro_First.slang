#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   int FrameDirection;
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
layout(set = 0, binding = 3) uniform sampler2D tape;

#include "VHSPro_functions.inc"
#define SamplerTape tape

void main()
{
    const float t = float(params.FrameCount) * 0.0075;//_Time.y;                            
    vec2 p = txcoord.xy;
    
    float SLN = screenLinesNum; //TODO use only SLN
    float SLN_Noise = noiseLinesNum; //TODO only SLN_Noise
    float ONE_X = 0.0;
    float ONE_Y = 0.0;

    //basically if its 0 -> set it to fullscreen
    //TODO calc it before shader / already float done
    SLN = screenLinesNum; //TODO use only SLN
    SLN_Noise = noiseLinesNum; //TODO only SLN_Noise
    if(SLN==0.0) SLN = _ScreenParams.y;

    if(SLN_Noise==0 || SLN_Noise>SLN) SLN_Noise = SLN;                                    
                    
    ONE_X = 1.0/_ScreenParams.x; //assigning works only here 
    ONE_Y = 1.0/_ScreenParams.y;                     
                    

    if (VHS_Twitch_V){
        p = twitchVertical(0.5*twitchVFreq, p, t); 
    }

    if (VHS_Twitch_H){
        p = twitchHorizonal(0.1*twitchHFreq, p, t);
    }    

    //make discrete lines /w or wo float 
    if(VHS_LinesFloat){
        float sh = fract(-t*linesFloatSpeed); //shift  // float sh = fmod(t, 1.); //shift
        // if(p.x>0.5)
        p.y = -floor( -p.y * SLN + sh )/SLN + sh/SLN;  //v1.3
        // p.y = floor( p.y * SLN + sh )/SLN - sh/SLN; //v1.2
    } else {
        // if(p.x>0.5)
        p.y = -floor( -p.y * SLN )/SLN;  //v1.3
        // p.y = floor( p.y * SLN )/SLN; //v1.2
    }
                    
    if (VHS_Stretch){
        p = stretch(p, t, 15.0, 1.0, 0.5, 0.0);
        p = stretch(p, t, 8.0, 1.2, 0.45, 0.5);
        p = stretch(p, t, 11.0, 0.5, -0.35, 0.25); //up   
    }

    if (VHS_Jitter_H){
        if( fmod( p.y * SLN, 2.0)<1.0) 
        p.x += ONE_X*sin(t*130.0)*jitterHAmount;
    }
    
    //just init    
    vec3 col = vec3(0.0,0.0,0.0);
    vec3 signal = vec3(0.0,0.0,0.0);// rgb2yiq(col);

    //gotta initiate all these things here coz of tape noise distortion

    //[NOISE uv init]
    //if SLN_Noise different from SLN->recalc linefloat                    
    vec2 pn = p;
    if(SLN!=SLN_Noise){
        if(VHS_LineNoise){
            const float sh = fract(t); //shift  // float sh = fmod(t, 1.); //shift
            pn.y = floor( pn.y * SLN_Noise + sh )/SLN_Noise - sh/SLN_Noise;
        } else  {
            pn.y = floor( pn.y * SLN_Noise )/SLN_Noise;
        }                 
    }      

    //SLN_X is quantization of X. goest from _ScreenParams.x to SLN_X
    const float ScreenLinesNumX = SLN_Noise * _ScreenParams.x / _ScreenParams.y;
    const float SLN_X = noiseQuantizeX*(_ScreenParams.x - ScreenLinesNumX) + ScreenLinesNumX;
    pn.x = floor( pn.x * SLN_X )/SLN_X;

    const vec2 pn_ = pn*_ScreenParams.xy;

    //TODO probably it shud be 1.0/SLN_Noise
    const float ONEXN = 1.0/SLN_X;
    //[//noise uv init]

    float distShift = 0; // for 2nd part of tape noise

    if (VHS_TapeNoise) {

        //uv distortion part of tapenoise
        distShift = 0; // for 2nd part of tape noise 
        for (int ii = 0; ii < 20 % 1023; ii++){

            //this is t.n. line value at pn.y and down each pixel
            //TODO i guess ONEXN shud be 1.0/sln noise                            
            float tnl = textureLod(SamplerTape, vec2(0.0,pn.y-ONEXN*ii), 0.).y;
            // float tnl = texture(SamplerTape, vec2(0.0,pn.y-ONEXN*ii)).y;
            // float tnl = tapeNoiseLines(vec2(0.0,pn.y-ONEXN*i), t*tapeNoiseSpeed)*tapeNoiseAmount;
            tnl -= 0.15 * float(params.FrameDirection);
            // float fadediff = hash12(vec2(pn.x-ONEXN*i,pn.y)); 
            if(tnl>0.27) {                            
                    //TODO get integer part other way
                    const float sh = sin( 1.0*PI*(float(ii)/float(20))) ; //0..1                                
                    p.x -= float(int(sh)*4.0*ONEXN); //displacement
                    distShift += sh ; //for 2nd part
                    // p.x +=  ONEXN * float(int(((tnl-thth)/thth)*4.0));
                    // col.x = sh;    
                }
            }
        }

    //uv transforms over

    //picture proccess start
    if (VHS_Jitter_V){
        signal = yiqDist(p, jitterVAmount, t*jitterVSpeed);
    } else {
        col = texture(SamplerColorVHS, p).rgb;
        // col = vec3(p.xy, 0.0);//debug
        signal = rgb2yiq(col);
    }


    if (VHS_LineNoise || VHS_FilmGrain){
        signal.x += texture(SamplerTape, pn).z;
    }
                       
    //iq noise from yiq
    if (VHS_YIQNoise){
        if (signalNoiseType == 0) {
                        //TODO make cheaper noise                         
                        //type 1 (best) w Y mask
                        const vec2 noise = n4rand_bw( pn_,t,1.0-signalNoisePower ) ; 
                        signal.y += (noise.x*2.0-1.0)*signalNoiseAmount*signal.x;
                        signal.z += (noise.y*2.0-1.0)*signalNoiseAmount*signal.x;
                } else if (signalNoiseType == 1){
                        //type 2
                        const vec2 noise = n4rand_bw( pn_,t, 1.0-signalNoisePower ) ; 
                        signal.y += (noise.x*2.0-1.0)*signalNoiseAmount;
                        signal.z += (noise.y*2.0-1.0)*signalNoiseAmount;
                } else {
                        //type 3
                        const vec2 noise = n4rand_bw( pn_,t, 1.0-signalNoisePower )*signalNoiseAmount ; 
                        signal.y *= noise.x;
                        signal.z *= noise.y;    
                        signal.x += (noise.x*2.0-1.0)*0.05;
                }
            }

    //2nd part with noise, tail and yiq col shift
    if (VHS_TapeNoise){

    //here is normilized p (0..1)
    float tn = texture(SamplerTape, pn).x;
    signal.x = bms(signal.x, tn*tapeNoiseAmount ).x;  
    // float tn = tapeNoise(pn, t*tapeNoiseSpeed)*tapeNoiseAmount;

    //tape noise tail
    const int tailLength=10; //TODO adjustable

    for(int j = 0; j < tailLength % 1023; j++){

        const float jj = float(j);
        const vec2 d = vec2(pn.x-ONEXN*jj,pn.y);
        tn = textureLod(SamplerTape, vec2(d), 0. ).x;
        // tn = texture(SamplerTape, d).x;
        // tn = tapeNoise(vec2(pn.x-ONEXN*i,pn.y), t*tapeNoiseSpeed)*tapeNoiseAmount;

        float fadediff = 0.0;

        //for tails length difference
//        if(__RENDERER__ == 0x0A100 || __RENDERER__ == 0x0B000) {
//            fadediff = textureLod(SamplerTape, vec2(d), 0.).a; //hash12(d);
//        }

//        if(__RENDERER__ == 0x09300 || __RENDERER__ >= 0x10000) {
            fadediff = texture(SamplerTape, d).a; //hash12(d);
//        }

        if( tn > 0.8 ){                                
            float nsx =  0.0; //new signal x
            const float newlength = float(tailLength)*(1.0-fadediff); //tail lenght diff
            if( jj <= newlength ) nsx = 1.0-( jj/ newlength ); //tail
            signal.x = bms(signal.x, nsx*tapeNoiseAmount).x;                                    
        }                
    }

    //tape noise color shift
    if(distShift>0.4){
        // float tnl = tapeNoiseLines(vec2(0.0,pn.y), t*tapeNoiseSpeed)*tapeNoiseAmount;
        const float tnl = texture(SamplerTape, pn).y;//tapeNoiseLines(vec2(0.0,pn.y), t*tapeNoiseSpeed)*tapeNoiseAmount;
        signal.y *= 1.0/distShift;//tnl*0.1;//*distShift;//*signal.x;
        signal.z *= 1.0/distShift;//*distShift;//*signal.x;

        }
}

    //back to rgb color space
    //signal has negative values
    col = yiq2rgb(signal);

    //TODO put it into 2nd pass
    if (VHS_ScanLines){
        col *= scanLines(txcoord.xy, t);                         
    }

    //fisheye cutoff / outside fisheye
    //helps to remove noise outside the fisheye
    if (VHS_FishEye){

        p = txcoord.xy;

        float far;
        const vec2 hco = vec2(ONE_X*cutoffX, ONE_Y*cutoffY) * 0.25; //hard cuttoff x
        const vec2 sco = vec2(ONE_X*cutoffFadeX, ONE_Y*cutoffFadeY) * 0.25; //soft cuttoff x

        //hard cutoff
        if( p.x<=(0.0+hco.x) || p.x>=(1.0-hco.x) || p.y<=(0.0+hco.y) || p.y>=(1.0-hco.y) ){
            col = vec3(0.0,0.0,0.0);
        } else { 
            //fades
                            
            if( //X
                (p.x>(0.0+hco.x) && p.x<(0.0+(sco.x+hco.x) )) || (p.x>(1.0-(sco.x+hco.x)) && p.x<(1.0-hco.x)) 
            ){    
                if(p.x<0.5)    far = (0.0-hco.x+p.x)/(sco.x);                                    
                else            
                far = (1.0-hco.x-p.x)/(sco.x);
                                
                col *= float(far).xxx;
            }; 

                if( //Y
                    (p.y>(0.0+hco.y)              && p.y<(0.0+(sco.y+hco.y) )) || (p.y>(1.0-(sco.y+hco.y)) && p.y<(1.0-hco.y)) 
                ){
                    if(p.y<0.5)    far = (0.0-hco.y+p.y)/(sco.y);                                    
                    else
                    far = (1.0-hco.y-p.y)/(sco.y);
                                
                    col *= float(far).xxx;
                }
            }
        }
        
    // col = texture(SamplerTape, txcoord.xy).x;

    FragColor = vec4(col, 1.0); 
//    FragColor = (VHS_TapeNoise && params.FrameDirection < 0) ? FragColor + (vec4(texture(tape, txcoord).g)) * 2. : FragColor;
}