#version 450

// NTSC-Adaptive
// based on Themaister's NTSC shader

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
   float quality, ntsc_sat, cust_fringing, cust_artifacting, ntsc_bright, ntsc_scale, ntsc_fields, ntsc_phase;
} global;

#pragma parameter quality "NTSC Preset (Svideo=0 Composite=1 RF=2 Custom=-1)" 1.0 -1.0 2.0 1.0
#pragma parameter ntsc_fields "NTSC Merge Fields" 0.0 0.0 1.0 1.0
#pragma parameter ntsc_phase "NTSC Phase: Auto | 2 phase | 3 phase" 1.0 1.0 3.0 1.0
#pragma parameter ntsc_scale "NTSC Resolution Scaling" 1.0 0.20 3.0 0.05
#pragma parameter ntsc_sat "NTSC Color Saturation" 1.0 0.0 2.0 0.01
#pragma parameter ntsc_bright "NTSC Brightness" 1.0 0.0 1.5 0.01
#pragma parameter cust_fringing "NTSC Custom Fringing Value" 0.0 0.0 5.0 0.1
#pragma parameter cust_artifacting "NTSC Custom Artifacting Value" 0.0 0.0 5.0 0.1

#define PI 3.14159265

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 pix_no;
layout(location = 2) out float phase;
layout(location = 3) out float BRIGHTNESS;
layout(location = 4) out float SATURATION;
layout(location = 5) out float FRINGING;
layout(location = 6) out float ARTIFACTING;
layout(location = 7) out float CHROMA_MOD_FREQ;
layout(location = 8) out float MERGE;

void main()
{
	float res = global.ntsc_scale;
	float OriginalSize = global.OriginalSize.y;
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	if (res < 1.0) pix_no = TexCoord * global.SourceSize.xy * (res * global.OutputSize.xy / global.SourceSize.xy); else
                   pix_no = TexCoord * global.SourceSize.xy * (      global.OutputSize.xy / global.SourceSize.xy);
	phase = (global.ntsc_phase < 1.5) ? ((OriginalSize > 300.0) ? 2.0 : 3.0) : ((global.ntsc_phase > 2.5) ? 3.0 : 2.0);
	
	res = max(res, 1.0);	
	CHROMA_MOD_FREQ = (phase < 2.5) ? (4.0 * PI / 15.0) : (PI / 3.0);
	ARTIFACTING = (global.quality > -0.5) ? global.quality * 0.5*(res+1.0) : global.cust_artifacting;
	FRINGING = (global.quality > -0.5) ? global.quality : global.cust_fringing;
	SATURATION = global.ntsc_sat;
	BRIGHTNESS = global.ntsc_bright;	
	pix_no.y = pix_no.y * res;

	MERGE = (int(global.quality) == 2 || phase < 2.5) ? 0.0 : 1.0;
	MERGE = (int(global.quality) == -1) ? global.ntsc_fields : MERGE;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 pix_no;
layout(location = 2) in float phase;
layout(location = 3) in float BRIGHTNESS;
layout(location = 4) in float SATURATION;
layout(location = 5) in float FRINGING;
layout(location = 6) in float ARTIFACTING;
layout(location = 7) in float CHROMA_MOD_FREQ;
layout(location = 8) in float MERGE;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

#define mix_mat mat3(BRIGHTNESS, FRINGING, FRINGING, ARTIFACTING, 2.0 * SATURATION, 0.0, ARTIFACTING, 0.0, 2.0 * SATURATION)

const mat3 yiq2rgb_mat = mat3(
   1.0, 0.956, 0.6210,
   1.0, -0.2720, -0.6474,
   1.0, -1.1060, 1.7046);

vec3 yiq2rgb(vec3 yiq)
{
   return yiq * yiq2rgb_mat;
}

const mat3 yiq_mat = mat3(
      0.2989, 0.5870, 0.1140,
      0.5959, -0.2744, -0.3216,
      0.2115, -0.5229, 0.3114
);

vec3 rgb2yiq(vec3 col)
{
   return col * yiq_mat;
}

void main()
{
   vec3 col = texture(Source, vTexCoord).rgb;
   vec3 yiq = rgb2yiq(col);
   vec3 yiq2 = yiq;	

   float mod1 = 2.0;
   float mod2 = 3.0; 

if (MERGE > 0.5)
{
   float chroma_phase2 = (phase < 2.5) ? PI * (mod(pix_no.x, mod1) + mod(global.FrameCount+1, 2.)) : 0.6667 * PI * (mod(pix_no.x, mod2) + mod(global.FrameCount+1, 2.));
   float mod_phase2 = chroma_phase2 + pix_no.y * CHROMA_MOD_FREQ;
   float i_mod2 = cos(mod_phase2);
   float q_mod2 = sin(mod_phase2);
   yiq2.yz *= vec2(i_mod2, q_mod2); // Modulate.
   yiq2 *= mix_mat; // Cross-talk.
   yiq2.yz *= vec2(i_mod2, q_mod2); // Demodulate.   
}
   
   float chroma_phase = (phase < 2.5) ? PI * (mod(pix_no.x, mod1) + mod(global.FrameCount, 2.)) : 0.6667 * PI * (mod(pix_no.x, mod2) + mod(global.FrameCount, 2.));
   float mod_phase = chroma_phase + pix_no.y * CHROMA_MOD_FREQ;

   float i_mod = cos(mod_phase);
   float q_mod = sin(mod_phase);

   yiq.yz *= vec2(i_mod, q_mod); // Modulate.
   yiq *= mix_mat; // Cross-talk.
   yiq.yz *= vec2(i_mod, q_mod); // Demodulate.
      
   yiq = (MERGE < 0.5) ? yiq : 0.5*(yiq+yiq2);
   
   FragColor = vec4(yiq, 1.0);
} 
