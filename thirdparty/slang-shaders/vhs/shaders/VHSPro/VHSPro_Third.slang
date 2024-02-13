#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#include "VHSPro_params.inc"
#include "VHSPro_constants.inc"

#define SamplerColorVHS Original
#define VHS_InputB OriginalHistory1

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
layout(set = 0, binding = 2) uniform sampler2D Original;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D PassFeedback0;

#define _FeedbackTex PassFeedback0

#include "VHSPro_functions.inc"

void main()
{
    vec2 p = texcoord.xy;
    const float one_x = 1.0/_ScreenParams.x;
    vec3 fc = texture( SamplerColorVHS, texcoord).rgb;    // curent frame
    
    //new feedback
    const vec3 fl = texture( VHS_InputB, texcoord).rgb;    //last frame without feedback
    float  diff = abs(fl.x-fc.x + fl.y-fc.y + fl.z-fc.z)/3.0; //dfference between frames
    if(diff<feedbackThresh) diff = 0.0;
    vec3 fbn = fc*diff*feedbackAmount; //feedback new
    // fbn = vec3(0.0, 0.0, 0.0);

    //feedback buffer
    vec3 fbb = ( //blur old bufffer a bit
            texture( _FeedbackTex, texcoord).rgb + 
            texture( _FeedbackTex, texcoord + vec2(one_x, 0.0)).rgb + 
            texture( _FeedbackTex, texcoord - vec2(one_x, 0.0)).rgb
        ) / 3.0;
    fbb *= feedbackFade;

    //blend
    fc = bm_screen(fc, fbb);
    fbn = bm_screen(fbn, fbb); //feedback
    vec4 feedbackoutput = vec4(fc, 1.0); 
    vec4 feedback = vec4(fbn * feedbackColor, 1.0);
    FragColor = feedback;
}