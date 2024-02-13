#version 450

// NES PAL composite signal simulation for RetroArch
// shader by r57shell
// thanks to feos & HardWareMan & NewRisingSun

// also TV subpixels and scanlines

// LICENSE: PUBLIC DOMAIN

// NOTE: for nice TV subpixels and scanlines I recommend to
// disable this features here and apply CRT-specialized shader.

// Quality considerations

// there are three main options:
// USE_RAW (R), USE_DELAY_LINE (D), USE_COLORIMETRY (C)
// here is table of quality in decreasing order:
// RDC, RD, RC, DC, D, C

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float Gamma;
   float Brightness;
   float Contrast;
   float Saturation;
   float HueShift;
   float HueRotation;
   float Ywidth;
   float Uwidth;
   float Vwidth;
   float TV_Pixels;
   float SizeX;
   float SizeY;
   float dark_scanline;
   float Phase_Y;
   float Phase_One;
   float Phase_Two;
   float USE_RAW_param;
   float USE_LUT_param;
} params;

#pragma parameter USE_RAW_param "(NES ONLY) Decode RAW Colors" 0.0 0.0 1.0 1.0
#pragma parameter USE_LUT_param "(NES ONLY) Use RAW LUT For Speed" 0.0 0.0 1.0 1.0

bool USE_RAW = bool(params.USE_RAW_param);
bool USE_LUT = bool(params.USE_LUT_param);

#pragma parameter Gamma "PAL Gamma" 2.5 0.0 10.0 0.03125
#pragma parameter Brightness "PAL Brightness" 0.0 -1.0 2.0 0.03125
#pragma parameter Contrast "PAL Contrast" 1.0 -1.0 2.0 0.03125
#pragma parameter Saturation "PAL Saturation" 1.0 -1.0 2.0 0.03125
#pragma parameter HueShift "PAL Hue Shift" -2.5 -6.0 6.0 0.015625
#pragma parameter HueRotation "PAL Hue Rotation" 2.0 -5.0 5.0 0.015625
#pragma parameter Ywidth "PAL Y Width" 12.0 1.0 32.0 1.0
#pragma parameter Uwidth "PAL U Width" 23.0 1.0 32.0 1.0
#pragma parameter Vwidth "PAL V Width" 23.0 1.0 32.0 1.0
#pragma parameter SizeX "Active Width" 256.0 1.0 4096.0 1.0
#pragma parameter SizeY "Active Height" 240.0 1.0 4096.0 1.0
#pragma parameter TV_Pixels "PAL TV Pixels" 200.0 1.0 2400.0 1.0
#pragma parameter dark_scanline "PAL Scanline" 0.5 0.0 1.0 0.025
#pragma parameter Phase_Y "PAL Phase Y" 2.0 0.0 12.0 0.025
#pragma parameter Phase_One "PAL Phase One" 0.0 0.0 12.0 0.025
#pragma parameter Phase_Two "PAL Phase Two" 8.0 0.0 12.0 0.025

// compatibility macros
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define frac(c) fract(c)
#define saturate(c) clamp(c, 0.0, 1.0)
#define fmod(x,y) mod(x,y)
#define mul(x,y) (y*x)
#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

// TWEAKS start

// comment this to disable dynamic settings, and use static.
// if you unable to compile shader with dynamic settings,
// and you want to tune parameters in menu, then
// try to reduce somewhere below Mwidth from 32 to lower,
// or disable USE_DELAY_LINE or USE_RAW, or all at once.
#define PARAMETER_UNIFORM

// use delay line technique
// without delay line technique, color would interleave
// to avoid this, set HueRotation to zero.
#define USE_DELAY_LINE

// use this if you need to swap even/odd V sign.
// sign of V changes each scanline
// so if some scanline is positive, then next is negative
// and if you want to match picture
// to actual running PAL NES on TV
// you may want to have this option, to change signs
// if they don't match
//#define SWAP_VSIGN

// phase shift from frame to frame as NTSC NES does.
// but PAL NES doesn't
//#define ANIMATE_PHASE

// rough simulation of scanlines
// better if you use additional shader instead
// if you still use it, make sure that SizeY
// is at least twice lower than output height
//#define USE_SCANLINES

// this option changes active visible fields.
// this is not how actual NES works
// it does not alter fields.
//#define ANIMATE_SCANLINE

// simulate CRT TV subpixels
// better if you use CRT-specialized shader instead
//#define USE_SUBPIXELS

// to change gamma of virtual TV from 2.2 to something else
//#define USE_GAMMA

// use core size. for NES use this, for other cores turn off
// for other cores use "size" tweak.
//#define USE_CORE_SIZE

// use raw palette, turn it on if you
// have nestopia and having using raw palette
//#define USE_RAW

// use lookup texture, faster but less accuracy
// it's working only if USE_RAW enabled.
//#define USE_LUT

// compensate filter width
// it will make width of picture shorter
// to make picture right border visible
#define COMPENSATE_WIDTH

// use sampled version. it's much more slower version of shader.
// because it is computing x4 more values. NOT RECOMMENDED.
//#define USE_SAMPLED

/*#ifndef PARAMETER_UNIFORM
// commented because parameters are always available in slang

// NTSC standard gamma = 2.2
// PAL standard gamma = 2.8
// according to many sources, very unlikely gamma of TV is 2.8
// most likely gamma of PAL TV is in range 2.4-2.5
const float Gamma_static = 2.5; // gamma of virtual TV

const float Brightness_static = 0.0;
const float Contrast_static = 1.0;
const float Saturation_static = 1.0;

const int
	Ywidth_static = 12,
	Uwidth_static = 23,
	Vwidth_static = 23;

// correct one is -2.5
// works only with USE_RAW
const float HueShift = -2.5;

// rotation of hue due to luma level.
const float HueRotation = 2.;

// touch this only if you know what you doing
const float Phase_Y = 2.; // fmod(341*10,12)
const float Phase_One = 0.; // alternating phases.
const float Phase_Two = 8.;

// screen size, scanlines = y*2; y one field, and y other field.
const int SizeX = 256;
const int SizeY = 240;

// count of pixels of virtual TV.
// value close to 1000 produce small artifacts
const int TV_Pixels = 400;

const float dark_scanline = 0.5; // half
#endif*/

// this is using following matrixes.
// it provides more scientific approach
// by conversion into linear XYZ space
// and back to sRGB.
// it's using Gamma setting too.
// define USE_GAMMA is not required.
#define USE_COLORIMETRY

const float3x3 RGB_to_XYZ =
mat3(
	0.4306190, 0.3415419, 0.1783091,
	0.2220379, 0.7066384, 0.0713236,
	0.0201853, 0.1295504, 0.9390944
);

const float3x3 XYZ_to_sRGB =
mat3(
	 3.2406, -1.5372, -0.4986,
	-0.9689,  1.8758,  0.0415,
	 0.0557, -0.2040,  1.0570
);

// TWEAKS end

//#ifdef PARAMETER_UNIFORM

const float Mwidth = 24;

const int Ywidth_static = 1;
const int Uwidth_static = 1;
const int Vwidth_static = 1;

const float Contrast_static = 1.;
const float Saturation_static = 1.;

/*#else

#define Brightness Brightness_static
#define Gamma Gamma_static

#define Ywidth Ywidth_static
#define Uwidth Uwidth_static
#define Vwidth Vwidth_static

const int Mwidth = max(float(Ywidth), max(float(Uwidth), float(Vwidth)));

#ifdef USE_CORE_SIZE
// just use core output size.
#define size (params.SourceSize.xy)
#else
float2 size = float2(SizeX,SizeY);
#endif

#endif*/

const float YUV_u = 0.492;
const float YUV_v = 0.877;

const mat3 RGB_to_YUV =
mat3(
	float3( 0.299, 0.587, 0.114), //Y
	float3(-0.299,-0.587, 0.886)*YUV_u, //B-Y
	float3( 0.701,-0.587,-0.114)*YUV_v //R-Y
);

#ifdef USE_DELAY_LINE
const float comb_line = 1.;
#else
const float comb_line = 2.;
#endif

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float DeltaV;
layout(location = 2) out float Voltage_0;
layout(location = 3) out float Voltage_1;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;

	if(USE_RAW)
	{
		Voltage_0 = (!USE_LUT) ? 0.518 : 0.15103768593097774;
		Voltage_1 = (!USE_LUT) ? 1.962 : 1.;
		DeltaV = (Voltage_1-Voltage_0);
	}
	else DeltaV = 1.;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float DeltaV;
layout(location = 2) in float Voltage_0;
layout(location = 3) in float Voltage_1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D nes_lut;

float RGB_y = Contrast_static/Ywidth_static/DeltaV;
float RGB_u = comb_line*Contrast_static*Saturation_static/YUV_u/Uwidth_static/DeltaV;
float RGB_v = comb_line*Contrast_static*Saturation_static/YUV_v/Vwidth_static/DeltaV;

mat3 YUV_to_RGB =
mat3(
	float3(1., 1., 1.)*RGB_y,
	float3(0., -0.114/0.587, 1.)*RGB_u,
	float3(1., -0.299/0.587, 0.)*RGB_v
);

const float pi = 3.1415926535897932384626433832795;

bool InColorPhase(int color, float phase)
{
	return fmod((color*2. + phase),24.) < 12.;
}

// from nesdev wiki page NTSC_video
float NTSCsignal(float3 pixel, float phase)
{
	float NTSC_out;
	
	// use LUT for RAW palette decoding for speed vs quality
	NTSC_out = texture(nes_lut,float2(dot(pixel,float3(
	15.*(8.)/512.,
	3.*(16.*8.)/512.,
	7./512.)
	) + 0.5/(4.*16.*8.), frac(phase/24.))).r;
	
	// return early to avoid costly decoding in software
	if(USE_LUT) return NTSC_out;
		
	// Voltage levels, relative to synch voltage
	const float black=.518, white=1.962, attenuation=.746,
		levels[8] = {.350, .518, .962,1.550,  // Signal low
				1.094,1.506,1.962,1.962}; // Signal high

	// Decode the NES color.
	int color = int(pixel.r*15);	// 0..15 "cccc"
	int level = int(pixel.g*3);	// 0..3  "ll"
	int emphasis = int(pixel.b*7+0.1);	// 0..7  "eee"
	if (color > 13) { level = 1; }	// For colors 14..15, level 1 is forced.

	// The square wave for this color alternates between these two voltages:
	float low = levels[0], high = levels[4];
	if (level == 1) { low = levels[1], high = levels[5]; }
	if (level == 2) { low = levels[2], high = levels[6]; }
	if (level == 3) { low = levels[3], high = levels[7]; }
	if(color == 0) { low = high; } // For color 0, only high level is emitted
	if(color > 12) { high = low; } // For colors 13..15, only low level is emitted


	// Generate the square wave
	// When de-emphasis bits are set, some parts of the signal are attenuated:
	float2 e = fmod(float2(emphasis), float2(2.,4.));
	float signal = InColorPhase(color,phase) ? high : low;

	if( ((e.x != 0) && InColorPhase(0,phase))
	||  ((e.y-e.x != 0) && InColorPhase(4,phase))
	||  ((emphasis-e.y != 0) && InColorPhase(8,phase)) )
		NTSC_out = signal * attenuation;
	else
		NTSC_out = signal;
	
	return NTSC_out;
}

float sinn(float x)
{
	return sin(/*fmod(x,24)*/x*(pi*2./24.));
}

float coss(float x)
{
	return cos(/*fmod(x,24)*/x*(pi*2./24.));
}

float3 monitor(sampler2D tex, float2 p)
{
//#ifdef PARAMETER_UNIFORM
	const float2 size = float2(params.SizeX,params.SizeY);
//#endif
	// align vertical coord to center of texel
	float2 uv = float2(
#ifdef COMPENSATE_WIDTH
		p.x+p.x*(params.Ywidth/8.)/size.x,
#else
		p.x,
#endif
		(floor(p.y*params.SourceSize.y)+0.5)/params.SourceSize.y);
#ifdef USE_DELAY_LINE
	float2 sh = (params.SourceSize.xy/params.SourceSize.xy/size)*float2(14./10.,-1.0);
#endif
	float2 pc = uv*params.SourceSize.xy/params.SourceSize.xy*size*float2(10.,1.);
	float alpha = dot(floor(float2(pc.x,pc.y)),float2(2.,params.Phase_Y*2.));
	alpha += params.Phase_One*2.;
#ifdef ANIMATE_PHASE
	if (fmod(IN.frame_count,2) > 1.)
		alpha += (params.Phase_Two-params.Phase_One)*2.;
#endif

	// 1/size.x of screen in uv coords = params.SourceSize.x/params.SourceSize.x/size.x;
	// then 1/10*size.x of screen:
	float ustep = params.SourceSize.x/params.SourceSize.x/size.x/10.;

	float border = params.SourceSize.x/params.SourceSize.x;
	float ss = 2.0;
#ifdef SWAP_VSIGN
#define PAL_SWITCH(A) A < 1.
#else
#define PAL_SWITCH(A) A > 1.
#endif
	if (PAL_SWITCH(fmod(uv.y*params.SourceSize.y/params.SourceSize.y*size.y,2.0)))
	{
		// cos(pi-alpha) = -cos(alpha)
		// sin(pi-alpha) = sin(alpha)
		// pi - alpha
		alpha = -alpha+12012.0;
		ss = -2.0;
	}

	float ysum = 0., usum = 0., vsum = 0., sig = 0., sig1 = 0.;
	for (int i=0; i<Mwidth; ++i)
	{

		float4 res = texture(tex, uv);
		
		if(USE_RAW)
		{
			sig = NTSCsignal(res.xyz,params.HueShift*2.+alpha-res.g*ss*params.HueRotation)-Voltage_0;
			// outside of texture is 0,0,0 which is white instead of black
			if (uv.x <= 0.0 || uv.x >= border)
				sig = 0;
#ifdef USE_DELAY_LINE
			float4 res1 = texture(tex, uv+sh);
			sig1 = NTSCsignal(res1.xyz,params.HueShift*2.+12012.0-alpha+res.g*ss*params.HueRotation)-Voltage_0;
			if (uv.x + sh.x <= 0.0 || uv.x + sh.x >= border)
				sig1 = 0;
#endif
		}
		else
		{
			float3 yuv = mul(RGB_to_YUV, res.xyz);
			const float a1 = alpha+(params.HueShift+2.5)*2.-yuv.x*ss*params.HueRotation;
			sig = yuv.x+dot(yuv.yz,sign(float2(sinn(a1),coss(a1))));
#ifdef USE_DELAY_LINE
			float4 res1 = texture(tex, uv+sh);
			float3 yuv1 = mul(RGB_to_YUV, res1.xyz);
			const float a2 = (params.HueShift+2.5)*2.+12012.0-alpha+yuv.x*ss*params.HueRotation;
			sig1 = yuv1.x+dot(yuv1.yz,sign(float2(sinn(a2),coss(a2))));
#endif
		}
		
		if (i < params.Ywidth)
			ysum += sig;

#ifdef USE_DELAY_LINE
		if (i < params.Uwidth)
			usum += (sig+sig1)*sinn(alpha);
		if (i < params.Vwidth)
			vsum += (sig-sig1)*coss(alpha);
#else
		if (i < params.Uwidth)
			usum += sig*sinn(alpha);
		if (i < params.Vwidth)
			vsum += sig*coss(alpha);
#endif
		alpha -= ss;
		uv.x -= ustep;
	}

//#ifdef PARAMETER_UNIFORM
	ysum *= params.Contrast/params.Ywidth;
	usum *= params.Contrast*params.Saturation/params.Uwidth;
	vsum *= params.Contrast*params.Saturation/params.Vwidth;
//#endif

	float3 rgb = mul(float3(ysum+params.Brightness*Ywidth_static,usum,vsum), YUV_to_RGB);
#if defined(USE_GAMMA) && !defined(USE_COLORIMETRY)
	float3 rgb1 = saturate(rgb);
	rgb = pow(rgb1, params.Gamma/2.2);
#endif

#ifdef USE_COLORIMETRY
	float3 rgb1 = saturate(rgb);
	rgb = pow(rgb1, float3(params.Gamma, params.Gamma, params.Gamma));
#endif

#if (defined(USE_SUBPIXELS) || defined(USE_SCANLINES))
	float2 q = (p*params.SourceSize.xy/params.SourceSize.xy)*float2(params.TV_Pixels*3.,size.y*2.);
#endif

#ifdef USE_SCANLINES
	float scanlines = size.y * params.OutputSize.z;
	float top = fmod(q.y-0.5*scanlines*2.,2.);
	float bottom = top+frac(scanlines)*2.;
	float2 sw = saturate(min(float2(1.,2.),float2(bottom, bottom))
		-max(float2(0.,1.),float2(top)))
		+saturate(min(float2(3.,4.),float2(bottom, bottom))
		-max(float2(2.,3.),float2(top)))
		+floor(scanlines);
#ifdef ANIMATE_SCANLINE
#define SCANLINE_MUL (fmod(int(params.FrameCount),2)<1 \
		? sw.x*params.dark_scanline+sw.y \
		: sw.x+sw.y*params.dark_scanline)
#else
#define SCANLINE_MUL (sw.x*params.dark_scanline+sw.y)
#endif
	rgb = rgb*SCANLINE_MUL/(sw.x+sw.y);

/*
	//old stupid method
	float z =
#ifdef ANIMATE_SCANLINE
	fmod(params.FrameCount,2.0)+
#endif
		0.5;

	if (abs(fmod(q.y+0.5,2)-z)<0.5)
		rgb *= params.dark_scanline;
*/
#endif

	// size of pixel screen in texture coords:
	//float output_pixel_size = params.SourceSize.x/(params.OutputSize.x*params.SourceSize.x);

	// correctness check
	//if (fmod(p.x*output_pixel_size,2.0) < 1.0)
	//	rgb = float3(0.,0.,0.);

#ifdef USE_SUBPIXELS
	float pixels = params.TV_Pixels * params.OutputSize.z;
	float left = fmod(q.x-0.5*pixels*3.,3.);
	float right = left+frac(pixels)*3.;
	float3 w = saturate(min(float3(1.,2.,3.),float3(right,right,right))
		-max(float3(0.,1.,2.),float3(left,left,left)))
		+saturate(min(float3(4.,5.,6.),float3(right,right,right))
		-max(float3(3.,4.,5.),float3(left,left,left)))
		+floor(pixels);
	rgb = rgb*3.*w/(w.x+w.y+w.z);
#endif

#ifdef USE_COLORIMETRY
	float3 xyz1 = mul(RGB_to_XYZ,rgb);
	float3 srgb = saturate(mul(XYZ_to_sRGB,xyz1));
	float3 a1 = 12.92*srgb;
	float3 a2 = 1.055*pow(srgb,float3(1./2.4))-0.055;
	float3 ssrgb;
   ssrgb.x = (srgb.x<0.0031308?a1.x:a2.x);
   ssrgb.y = (srgb.y<0.0031308?a1.y:a2.y);
   ssrgb.z = (srgb.z<0.0031308?a1.z:a2.z);
	return ssrgb;
#else
	return rgb;
#endif
}

// pos (left corner, sample size)
float4 monitor_sample(sampler2D tex, float2 p, float2 sample_)
{
	// linear interpolation was...
	// now other thing.
	// http://imgur.com/m8Z8trV
	// AT LAST IT WORKS!!!!
	// going to check in retroarch...
	float2 size = params.SourceSize.xy;
	float2 next = float2(.25,1.)/size;
	float2 f = frac(float2(4.,1.)*size*p);
	sample_ *= float2(4.,1.)*size;
	float2 l;
	float2 r;
	if (f.x+sample_.x < 1.)
	{
		l.x = f.x+sample_.x;
		r.x = 0.;
	}
	else
	{
		l.x = 1.-f.x;
		r.x = min(1.,f.x+sample_.x-1.);
	}
	if (f.y+sample_.y < 1.)
	{
		l.y = f.y+sample_.y;
		r.y = 0.;
	}
	else
	{
		l.y = 1.-f.y;
		r.y = min(1.,f.y+sample_.y-1.);
	}
	float3 top = mix(monitor(tex, p), monitor(tex, p+float2(next.x,0.)), r.x/(l.x+r.x));
	float3 bottom = mix(monitor(tex, p+float2(0.,next.y)), monitor(tex, p+next), r.x/(l.x+r.x));
	return float4(mix(top,bottom, r.y/(l.y+r.y)),1.0);
}

void main()
{
#ifdef USE_SAMPLED
	FragColor = vec4(monitor_sample(Source, vTexCoord, params.OutputSize.zw).rgb, 1.0);
#else
	FragColor = float4(monitor(Source, vTexCoord), 1.);
#endif
   
}
