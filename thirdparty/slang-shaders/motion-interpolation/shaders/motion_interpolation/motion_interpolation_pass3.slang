#version 450

// Video Motion Interpolation
// based on Drudgerist's shadertoy:
// https://www.shadertoy.com/view/MtVfRz
// which is, in turn, based on bodhid's V+
// https://github.com/bodhid/Vplus
// This pass uses the motion vectors to make an in-between frame

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float MOTION_SCALE_FACTOR;
   float MOTION_MIX_FACTOR;
} params;

#pragma parameter MOTION_SCALE_FACTOR "Motion Interp Factor" 300.0 100.0 1000.0 50.0
#pragma parameter MOTION_MIX_FACTOR "Motion Blur Factor" 0.5 0.0 1.0 0.01

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
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 4) uniform sampler2D Pass0;
layout(set = 0, binding = 5) uniform sampler2D Original;

// uncomment the next line to see the motion vectors for debugging purposes
//#define OUTPUT_MOTION_VECTOR
#define iChannel3 Source
#define iChannel2 Pass0
#define iChannel1 Original
#define iChannel0 OriginalHistory1

void main()
{
    vec2 uv = vTexCoord.xy;
    vec4 col = texture(iChannel3, uv);
#ifdef OUTPUT_MOTION_VECTOR
    FragColor = (col * 0.5) + 0.5;
	return;
#endif
    
    float testShift = params.MOTION_MIX_FACTOR;//iMouse.x / iResolution.x;
    vec2 FinalUvShift = texture(iChannel2, uv).rg / params.MOTION_SCALE_FACTOR;
    vec4 previousFrame = texture(iChannel0, uv - (FinalUvShift * 0.5));
    vec4 nextFrame = texture(iChannel1, uv - (FinalUvShift * (0.5)));
    
    FragColor = mix(previousFrame, nextFrame, testShift);
}