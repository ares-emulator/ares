#version 450

// NTSC-Adaptive
// based on Themaister's NTSC shader


layout(push_constant) uniform Push
{
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   float ntsc_scale;
   float ntsc_phase;
   float ntsc_ring;
   float ntsc_cscale;
   float ntsc_cscale1;   
   float ntsc_taps;
   float auto_res;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter auto_res "          SNES/Amiga Hi-Res Auto Mode" 0.0 0.0 1.0 1.0
#pragma parameter ntsc_scale   "NTSC Resolution Scaling" 1.0 0.20 2.5 0.025
#pragma parameter ntsc_phase   "NTSC Phase: Auto | 2 phase | 3 phase | Mixed" 1.0 1.0 4.0 1.0
#pragma parameter ntsc_taps    "NTSC # of Taps (Filter Width)" 32.0 6.0 32.0 1.0
#pragma parameter ntsc_cscale  "NTSC Chroma Scaling / Bleeding (2-phase)" 1.0 0.50 4.00 0.05
#pragma parameter ntsc_cscale1 "NTSC Chroma Scaling / Bleeding (3-phase)" 1.0 0.20 2.25 0.05
#pragma parameter ntsc-row4 "------------------------------------------------" 0.0 0.0 0.0 1.0
#pragma parameter ntsc_ring    "NTSC Anti-Ringing" 0.5 0.0 1.0 0.10

float auto_rez = mix(1.0, 0.5, clamp(params.auto_res * round(params.OriginalSize.x/300.0)-1.0, 0.0, 1.0));
#define ntsc_scale params.ntsc_scale * auto_rez

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord - vec2(0.25 * (params.OriginalSize.z/auto_rez)/4.0, 0.0); // Compensate for decimate-by-2.
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D PrePass0;


vec3 fetch_offset2(vec2 dx)
{
   return texture(Source, vTexCoord + dx).xyz + texture(Source, vTexCoord - dx).xyz;
}

vec3 fetch_offset3(vec3 dx)
{
   return vec3(texture(Source, vTexCoord + dx.xz).x + texture(Source, vTexCoord - dx.xz).x, texture(Source, vTexCoord + dx.yz).yz + texture(Source, vTexCoord - dx.yz).yz);
}

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


const int TAPS_2_phase = 32;
const float luma_filter_2_phase[33] = float[33](
  -0.000174844,
  -0.000205844,
  -0.000149453,
  -0.000051693,
   0.000000000,
  -0.000066171,
  -0.000245058,
  -0.000432928,
  -0.000472644,
  -0.000252236,
   0.000198929,
   0.000687058,
   0.000944112,
   0.000803467,
   0.000363199,
   0.000013422,
   0.000253402,
   0.001339461,
   0.002932972,
   0.003983485,
   0.003026683,
  -0.001102056,
  -0.008373026,
  -0.016897700,
  -0.022914480,
  -0.021642347,
  -0.028863273,
   0.027271957,
   0.054921920,
   0.098342579,
   0.139044281,
   0.168055832,
   0.178571429);


const int TAPS_3_phase = 24;

const float chroma_filter_3_phase[25] = float[25](
  -0.000118847,
  -0.000271306,
  -0.000502642,
  -0.000930833,
  -0.001451013,
  -0.002064744,
  -0.002700432,
  -0.003241276,
  -0.003524948,
  -0.003350284,
  -0.002491729,
  -0.000721149,
   0.002164659,
   0.006313635,
   0.011789103,
   0.018545660,
   0.026414396,
   0.035100710,
   0.044196567,
   0.053207202,
   0.061590275,
   0.068803602,
   0.074356193,
   0.077856564,
   0.079052396);

const float luma_filter_4_phase[25] = float[25](
  -0.000472644,
  -0.000252236,
   0.000198929,
   0.000687058,
   0.000944112,
   0.000803467,
   0.000363199,
   0.000013422,
   0.000253402,
   0.001339461,
   0.002932972,
   0.003983485,
   0.003026683,
  -0.001102056,
  -0.008373026,
  -0.016897700,
  -0.022914480,
  -0.021642347,
  -0.028863273,
   0.027271957,
   0.054921920,
   0.098342579,
   0.139044281,
   0.168055832,
   0.178571429);

void main()
{

float luma_filter_3_phase[25] = float[25](
  -0.000012020,
  -0.000022146,
  -0.000013155,
  -0.000012020,
  -0.000049979,
  -0.000113940,
  -0.000122150,
  -0.000005612,
   0.000170516,
   0.000237199,
   0.000169640,
   0.000285688,
   0.000984574,
   0.002018683,
   0.002002275,
  -0.005909882,
  -0.012049081,
  -0.018222860,
  -0.022606931,
   0.002460860,
   0.035868225,
   0.084016453,
   0.135563500,
   0.175261268,
   0.220176552);

   float res = ntsc_scale;
   float OriginalSize = params.OriginalSize.x * auto_rez; 
   vec2 one_x = 0.25*params.OriginalSize.zz / res;
   vec3 signal = vec3(0.0);
   float phase = (params.ntsc_phase < 1.5) ? ((OriginalSize > 300.0) ? 2.0 : 3.0) : ((params.ntsc_phase > 2.5) ? 3.0 : 2.0);
   if (params.ntsc_phase == 4.0) { phase = 3.0; luma_filter_3_phase = luma_filter_4_phase; }

   float offset = 0.0; float tf = 0.0; int i = 0; float j = 0.0;
   vec3 wsum = 0.0.xxx;
   vec3 sums = wsum;
   vec3 tmp  = wsum;  

   if(phase < 2.5)
   {
	  vec2 dx = vec2(one_x.x, 0.0); vec2 dx1 = dx;
	  float iloop = max(params.ntsc_taps, 8.0);
	  int loopstart = int(TAPS_2_phase - iloop);
	  
	  float ltap = params.ntsc_taps + 1.0;
	  float cs_sub = iloop - iloop / params.ntsc_cscale;
	  float ctap = 0.0;
	  
      for (i = loopstart; i < 32; i++)
      {
         offset = float(i-loopstart); j = offset + 1.0; dx1 = (offset - iloop)*dx;
         sums = fetch_offset2(dx1); ctap = max(j-cs_sub, 0.0);
         tmp = vec3(luma_filter_2_phase[i], ctap.xx);
         wsum = wsum + tmp;
         signal += sums * tmp;
      }
      ctap = ltap - cs_sub;
	  tmp = vec3(luma_filter_2_phase[TAPS_2_phase], ctap.xx);
      wsum = wsum + wsum + tmp;
      signal += texture(Source, vTexCoord).xyz * tmp;
      signal = signal / wsum;
   }
   else
   {
      one_x.y = one_x.y/params.ntsc_cscale1;
	  vec3 dx = vec3(one_x.x, one_x.y, 0.0); vec3 dx1 = dx;
	  
	  float iloop = min(params.ntsc_taps, TAPS_3_phase);
      int loopstart = int(24.0 - iloop);
	  
      for (i = loopstart; i < 24; i++)
      {
         offset = float(i-loopstart); j = offset + 1.0; dx1.xy = (offset - iloop)*dx.xy;
         sums = fetch_offset3(dx1);
         tmp = vec3(luma_filter_3_phase[i], chroma_filter_3_phase[i].xx);
         wsum = wsum + tmp;
         signal += sums * tmp;
      }
      tmp = vec3(luma_filter_3_phase[TAPS_3_phase], chroma_filter_3_phase[TAPS_3_phase], chroma_filter_3_phase[TAPS_3_phase]);
      wsum = wsum + wsum + tmp;
      signal += texture(Source, vTexCoord).xyz * tmp;
      signal = signal / wsum;
   }

   if (params.ntsc_ring > 0.05)
   {
      vec2 dx = vec2(params.OriginalSize.z / min(res, 1.0), 0.0);
      float a = texture(Source, vTexCoord - 1.5*dx).a;
      float b = texture(Source, vTexCoord - 0.5*dx).a;
      float c = texture(Source, vTexCoord + 1.5*dx).a;
      float d = texture(Source, vTexCoord + 0.5*dx).a;
      float e = texture(Source, vTexCoord         ).a;	  
      signal.x = mix(signal.x, clamp(signal.x, min(min(min(a,b),min(c,d)),e), max(max(max(a,b),max(c,d)),e)), params.ntsc_ring);
   }
   
   vec3 orig = rgb2yiq(texture(PrePass0, vTexCoord).rgb);
   
   signal.x = clamp(signal.x, -1.0, 1.0);
   vec3 rgb = signal;
   
   FragColor = vec4(rgb, orig.x);
}  
