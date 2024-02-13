#version 450

//////////////////////////////////////////////////////////////////////////
//
// CC0 1.0 Universal (CC0 1.0)
// Public Domain Dedication 
//
// To the extent possible under law, J. Kyle Pittman has waived all
// copyright and related or neighboring rights to this implementation
// of CRT simulation. This work is published from the United States.
//
// For more information, please visit
// https://creativecommons.org/publicdomain/zero/1.0/
//
//////////////////////////////////////////////////////////////////////////

// This is the second step of the CRT simulation process,
// after the ntsc.fx shader has transformed the RGB values with a lookup table.
// This is where we apply effects "inside the screen," including spatial and temporal bleeding,
// an unsharp mask to simulate overshoot/undershoot, NTSC artifacts, and so on.

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float Tuning_Sharp;		// typically [0,1], defines the weighting of the sharpness taps
	float Tuning_Persistence_R;	// typically [0,1] per channel, defines the total blending of previous frame values
	float Tuning_Persistence_G;
	float Tuning_Persistence_B;
	float Tuning_Bleed;		// typically [0,1], defines the blending of L/R values with center value from prevous frame
	float Tuning_Artifacts;		// typically [0,1], defines the weighting of NTSC scanline artifacts (not physically accurate by any means)
	float NTSCLerp;			// Defines an interpolation between the two NTSC filter states. Typically would be 0 or 1 for vsynced 60 fps gameplay or 0.5 for unsynced, but can be whatever.
	float NTSCArtifactScale;
	float animate_artifacts;
} params;

#pragma parameter Tuning_Sharp "Composite Sharp" 0.2 0.0 1.0 0.05
#pragma parameter Tuning_Persistence_R "Red Persistence" 0.065 0.0 1.0 0.01
#pragma parameter Tuning_Persistence_G "Green Persistence" 0.05 0.0 1.0 0.01
#pragma parameter Tuning_Persistence_B "Blue Persistence" 0.05 0.0 1.0 0.01
#pragma parameter Tuning_Bleed "Composite Bleed" 0.5 0.0 1.0 0.05
#pragma parameter Tuning_Artifacts "Composite Artifacts" 0.5 0.0 1.0 0.05
#pragma parameter NTSCLerp "NTSC Artifacts" 1.0 0.0 1.0 1.0
#pragma parameter NTSCArtifactScale "NTSC Artifact Scale" 255.0 0.0 1000.0 5.0
#pragma parameter animate_artifacts "Animate NTSC Artifacts" 1.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// Weight for applying an unsharp mask at a distance of 1, 2, or 3 pixels from changes in luma.
// The sign of each weight changes in order to alternately simulate overshooting and undershooting.
const float SharpWeight[3] =
{
	1.0, -0.3162277, 0.1
};

// Calculate luma for an RGB value.
float Brightness(vec4 InVal)
{
	return dot(InVal, vec4(0.299, 0.587, 0.114, 0.0));
}

#pragma stage vertex
// Passthrough vertex shader. Nothing interesting here.
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
layout(set = 0, binding = 2) uniform sampler2D PassFeedback0;
#define prevFrameSampler PassFeedback0
layout(set = 0, binding = 3) uniform sampler2D Source;
#define curFrameSampler Source
layout(set = 0, binding = 4) uniform sampler2D NTSCArtifactSampler;

#define lerp(a, b, c) mix(a, b, c)
#define tex2D(a, b) texture(a, b)
#define half4 vec4
#define half2 vec2
#define half float
#define saturate(c) clamp(c, 0.0, 1.0)

void main()
{
	half2 scanuv = vec2(fract(vTexCoord * 1.0001 * params.SourceSize.xy / params.NTSCArtifactScale));
	half4 NTSCArtifact1 = tex2D(NTSCArtifactSampler, scanuv);
	half4 NTSCArtifact2 = tex2D(NTSCArtifactSampler, scanuv + vec2(0.0, 1.0 / params.SourceSize.y));
	float lerpfactor = (params.animate_artifacts > 0.5) ? mod(params.FrameCount, 2.0) : params.NTSCLerp;
	half4 NTSCArtifact = lerp(NTSCArtifact1, NTSCArtifact2, 1.0 - lerpfactor);
	
	half2 LeftUV = vTexCoord - vec2(1.0 / params.SourceSize.x, 0.0);
	half2 RightUV = vTexCoord + vec2(1.0 / params.SourceSize.x, 0.0);
	
	half4 Cur_Left = tex2D(curFrameSampler, LeftUV);
	half4 Cur_Local = tex2D(curFrameSampler, vTexCoord);
	half4 Cur_Right = tex2D(curFrameSampler, RightUV);
	
	half4 TunedNTSC = NTSCArtifact * params.Tuning_Artifacts;
		
	// Note: The "persistence" and "bleed" parameters have some overlap, but they are not redundant.
	// "Persistence" affects bleeding AND trails. (Scales the sum of the previous value and its scaled neighbors.)
	// "Bleed" only affects bleeding. (Scaling of neighboring previous values.)
	
	half4 Prev_Left = tex2D(prevFrameSampler, LeftUV);
	half4 Prev_Local = tex2D(prevFrameSampler, vTexCoord);
	half4 Prev_Right = tex2D(prevFrameSampler, RightUV);
	
	// Apply NTSC artifacts based on differences in luma between local pixel and neighbors..
	Cur_Local =
		saturate(Cur_Local +
		(((Cur_Left - Cur_Local) + (Cur_Right - Cur_Local)) * TunedNTSC));
	
	half curBrt = Brightness(Cur_Local);
	half offset = 0.;
	
	// Step left and right looking for changes in luma that would produce a ring or halo on this pixel due to undershooting/overshooting.
	// (Note: It would probably be more accurate to look at changes in luma between pixels at a distance of N and N+1,
	// as opposed to 0 and N as done here, but this works pretty well and is a little cheaper.)
	for (int i = 0; i < 3; ++i)
	{
		half2 StepSize = (half2(1.0/256.0,0) * (float(i + 1)));
		half4 neighborleft = tex2D(curFrameSampler, vTexCoord - StepSize);
		half4 neighborright = tex2D(curFrameSampler, vTexCoord + StepSize);
		
		half NBrtL = Brightness(neighborleft);
		half NBrtR = Brightness(neighborright);
		offset += ((((curBrt - NBrtL) + (curBrt - NBrtR))) * SharpWeight[i]);
	}
	
	// Apply the NTSC artifacts to the unsharp offset as well.
	Cur_Local = saturate(Cur_Local + (offset * params.Tuning_Sharp * lerp(ivec4(1,1,1,1), NTSCArtifact, params.Tuning_Artifacts)));
	
	vec4 Tuning_Persistence = vec4(params.Tuning_Persistence_R, params.Tuning_Persistence_G, params.Tuning_Persistence_B, 1.0);
	// Take the max here because adding is overkill; bleeding should only brighten up dark areas, not blow out the whole screen.
	Cur_Local = saturate(max(Cur_Local, Tuning_Persistence * (10.0 / (1.0 + (2.0 * params.Tuning_Bleed))) * (Prev_Local + ((Prev_Left + Prev_Right) * params.Tuning_Bleed))));
	
   FragColor = vec4(Cur_Local);
}
