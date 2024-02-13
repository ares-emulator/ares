#version 450

/*
   April 2023
   Fast CRT shader by DariusG.
 
*/
layout(push_constant) uniform Push
{
    uint FrameCount;
	float SCANLINE1;
	float SCANLINE2;
	float INTERLACE;
	float SCALE;
	float MSK1;
	float MSK2;
	float MSK_SIZE;
	float FADE;
	float PRESERVE;
   float SAT;
   float SIZE;
   float BLUR;
   float NOISE;
   float SEGA;
   float INGAMMA;
   float OUTGAMMA,WPR,WPG,WPB,BOOST, BOOSTD;
} params;

#pragma parameter SIZE "   Soft Pixel Size" 0.5 0.125 4.0 0.125
#pragma parameter BLUR "   Pixel Softness" 0.6 0.0 1.0 0.05
#pragma parameter SCANLINE1 "Scanline Strength Dark" 0.75 0.0 1.0 0.05
#pragma parameter SCANLINE2 "Scanline Strength Bright" 0.4 0.0 1.0 0.05
#pragma parameter INTERLACE "Interlace Mode" 1.0 0.0 1.0 1.0
#pragma parameter SCALE "Scanlines downscale"  1.0 1.0 2.0 1.0
#pragma parameter MSK1 "   Mask Brightness Dark" 0.3 0.0 1.0 0.05
#pragma parameter MSK2 "   Mask Brightness Bright" 0.6 0.0 1.0 0.05
#pragma parameter MSK_SIZE "   Mask Size" 0.5002 0.5002 1.0 0.1666
#pragma parameter FADE "   Mask/Scanlines fade" 0.2 0.0 1.0 0.05
#pragma parameter PRESERVE "Protect Bright Colors" 0.7 0.0 1.0 0.01
#pragma parameter SAT "Saturation" 1.0 0.0 2.0 0.05
#pragma parameter INGAMMA "Gamma In" 2.4 1.0 4.0 0.05
#pragma parameter OUTGAMMA "Gamma Out" 2.25 1.0 4.0 0.05
#pragma parameter SEGA "Brightness Fix 1.0:Sega, 2:Amiga/ST" 0.0 0.0 2.0 1.0
#pragma parameter NOISE "Add Noise" 0.0 0.0 1.0 0.01
#pragma parameter WPR "  Shift to Red" 0.0 -0.25 0.25 0.01
#pragma parameter WPG "  Shift to Green" 0.0 -0.25 0.25 0.01
#pragma parameter WPB "  Shift to Blue" 0.0 -0.25 0.25 0.01
#pragma parameter BOOST "  Bright Boost" 1.3 1.0 2.0 0.05
#pragma parameter BOOSTD "  Dark Boost" 1.45 1.0 2.0 0.05

#define OUTGAMMA params.OUTGAMMA
#define INGAMMA params.INGAMMA
#define SEGA params.SEGA
#define NOISE params.NOISE
#define SIZE params.SIZE
#define BLUR params.BLUR
#define SCANLINE1 params.SCANLINE1
#define SCANLINE2 params.SCANLINE2
#define INTERLACE params.INTERLACE
#define SCALE params.SCALE
#define MSK1 params.MSK1
#define MSK2 params.MSK2
#define MSK_SIZE params.MSK_SIZE 
#define FADE params.FADE 
#define PRESERVE params.PRESERVE
#define GAMMA params.GAMMA
#define SAT params.SAT
#define WPR params.WPR
#define WPG params.WPG
#define WPB params.WPB
#define BOOST params.BOOST
#define BOOSTD params.BOOSTD



layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
    vec4 SourceSize;
    vec4 OriginalSize;
    vec4 OutputSize;
} global;

#define iTimer float(params.FrameCount/60.0)
#define OutputSize global.OutputSize
#define SourceSize global.SourceSize
#define pi  3.141592654


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 omega;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord * 1.0001;
    omega = vec2(pi * OutputSize.x, pi * SourceSize.y/SCALE);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 omega;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;

float snow(vec2 pos)
{
    return fract(sin(iTimer * dot(pos.xy ,vec2(13,78.233))) * 43758.5453);
}



void main()
{

   vec2 pos = vTexCoord.xy;
/// SHARP-BILINEAR  Author: rsn8887
 	vec2 texel = pos * SourceSize.xy;
 	vec2 texel2 = pos * OutputSize.xy;
   vec2 scale = max(floor(OutputSize.xy / SourceSize.xy), vec2(1.0, 1.0));
   vec2 pixel = vec2(SIZE/OutputSize.x,0.0);
	vec2 pixely = vec2(0.0,SIZE/OutputSize.y);

   vec2 texel_floored = floor(texel);
   vec2 texel_floored2 = floor(texel2);
   vec2 s = fract(texel);
   vec2 s2 = fract(texel2);
   vec2 region_range = 0.5 - 0.5 / scale;

   // Figure out where in the texel to sample to get correct pre-scaled bilinear.
   // Uses the hardware bilinear interpolator to avoid having to sample 4 times manually.

   vec2 center_dist = s - 0.5;
   vec2 center_dist2 = s2 - 0.5;
   vec2 f = (center_dist - clamp(center_dist, -region_range, region_range)) * scale + 0.5;
   vec2 f2 = (center_dist2 - clamp(center_dist2, -region_range, region_range))/scale  + 0.5;

   vec2 mod_texel = texel_floored + f;
   vec2 mod_texel2 = texel_floored2 + f2;
   vec2 uv=mod_texel / SourceSize.xy;
   vec2 uv2=mod_texel2 / OutputSize.xy;

/// "GHOST" PIXELS LEFT/RIGHT CALCULATION
   vec3 col   = texture(Source, uv).rgb;
   vec3 colr  = texture(Source, uv2-pixel).rgb;
   vec3 coll  = texture(Source, uv2+pixel).rgb;
   vec3 cold  = texture(Source, uv2-pixely).rgb;

	vec3 color = vec3 (col.r*(1.0-BLUR) + coll.r*BLUR,
						    col.g*(1.0-BLUR)                              + cold.g*BLUR,
						    col.b*(1.0-BLUR)               + colr.b*BLUR);

  
    vec3 lumweight=vec3(0.3,0.6,0.1);
    float lum = dot(color,lumweight);

 
//APPLY MASK 
         float MSK  = mix(MSK1,MSK2,lum);       
         float mask = mix(abs(sin(vTexCoord.x*omega.x*MSK_SIZE)), 1.0, lum*PRESERVE);

         float scan = 1.0;
         float SCANLINE = mix(SCANLINE1,SCANLINE2,lum);
//INTERLACING MODE FIX SCANLINES
    if (INTERLACE > 0.0 && global.OriginalSize.y > 400.0 ) scan; else
         color *=  SCANLINE * sin(fract(vTexCoord.y*SourceSize.y/SCALE)*3.141529) +(1.0-SCANLINE);
    color = pow(color,vec3(INGAMMA));

         color =mix(color*mask, color, dot(color, vec3(FADE)));
    
    color *= mix(BOOSTD,BOOST,lum);
    color *= vec3(1.0+WPR,     1.0-WPR/2.0, 1.0-WPR/2.0);
    color *= vec3(1.0-WPG/2.0, 1.0+WPG,     1.0-WPG/2.0);
    color *= vec3(1.0-WPB/2.0, 1.0-WPB/2.0, 1.0+WPB);
    
    color *= 1.0 + snow(uv * 2.0) * (1.0-lum) *NOISE;
	 if (SEGA == 1.0) color *= 1.0625; else if (SEGA == 2.0) color *=2.0; else color;

	 color = pow(color,vec3(1.0/OUTGAMMA));

//FAST SATURATION CONTROL
        if(SAT !=1.0)
        {
        vec3 gray = vec3(lum);
        color = vec3(mix(gray, color, SAT));
        }

    FragColor = vec4(color,1.0);
}
