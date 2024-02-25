#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#define SamplerColorVHS second
#define _FeedbackTex third

#include "VHSPro_params.inc"
#include "VHSPro_constants.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 texcoord;

void main()
{
   gl_Position = global.MVP * Position;
   texcoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 texcoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D second;
layout(set = 0, binding = 3) uniform sampler2D third;

#include "VHSPro_functions.inc"

void main()
{
   // needed to add distortion here or the trails don't match up to the image
   vec2 p = texcoord;
    //distortions
    if (VHS_FishEye){
        p = fishEye(p, fisheyeSize, fisheyeBend); // p = fishEye(p, 1.2, 2.0);
        // return vec4( fract(p*5.0)/2.5, 1.0, 1.0); //debug
    }
    vec3 col = texture( SamplerColorVHS, texcoord).rgb; //curent frame
    const vec3 fbb = texture( _FeedbackTex, p).rgb; //feedback buffer
    
    if (VHS_Feedback){
        col = bm_screen(col, fbb*feedbackAmp);
        if (feedbackDebug){
            col = fbb;
        }
    }
                   
    FragColor = vec4(col, 1.0); 
}