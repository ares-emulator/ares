#version 450


// An OSSC-like shader
// By DariusG @May/2023

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float OVERSCAN,ASPECTx,ASPECTy, LINES, L1, L2,L3,L4,L5,TATE, HYBRID;
} params;

#pragma parameter LINES "Lines" 4.0 2.0 5.0 1.0
#define LINES params.LINES

#pragma parameter TATE "TATE mode"  0.0 0.0 1.0 1.0
#define TATE params.TATE

#pragma parameter L1 "Line 1 Scanline Strength %" 0.25 0.0 1.0 0.01
#define L1 params.L1

#pragma parameter L2 "Line 2 Scanline Strength %" 0.0 0.0 1.0 0.01
#define L2 params.L2

#pragma parameter L3 "Line 3 Scanline Strength %" 0.0 0.0 1.0 0.01
#define L3 params.L3

#pragma parameter L4 "Line 4 Scanline Strength %" 0.4 0.0 1.0 0.01
#define L4 params.L4

#pragma parameter L5 "Line 5 Scanline Strength %" 0.5 0.0 1.0 0.01
#define L5 params.L5

#pragma parameter HYBRID "Scanline Hybrid Blend % (depend on pixel overlayed)" 0.0 0.0 1.0 0.01
#define HYBRID params.HYBRID

#pragma parameter OVERSCAN "Overscan" 0.0 0.0 1.0 1.0 
#define OVERSCAN params.OVERSCAN

#pragma parameter something "--Set Retroarch Aspect to FULL--" 0.0 0.0 0.0 0.0

#pragma parameter ASPECTx "Aspect Ratio X" 4.0 1.0 32.0 1.0 
#define ASPECTx params.ASPECTx

#pragma parameter ASPECTy "Aspect Ratio Y" 3.0 1.0 32.0 1.0 
#define ASPECTy params.ASPECTy

#define OutputSize params.OutputSize
#define SourceSize params.SourceSize
#define OriginalSize params.OriginalSize

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

vec2 Overscan(vec2 pos, float dx, float dy){
	pos=pos*2.0-1.0;    
	pos*=vec2(dx/(ASPECTx/ASPECTy/OutputSize.x*OutputSize.y),dy);
	return pos*0.5 + 0.5;
} 

float scanline5(float y, float lm)
{
	float l = 1.0/LINES;
	float m = fract(y*l);
	if (m<l) return 1.0-L1*(1.0-lm*HYBRID);
	else if (m<2.0*l) return 1.0-L2*(1.0-lm*HYBRID);
	else if (m<3.0*l) return 1.0-L3*(1.0-lm*HYBRID);
	else if (m<4.0*l) return 1.0-L4*(1.0-lm*HYBRID);
	else return 1.0-L5*(1.0-lm*HYBRID);
}

float scanline4(float y, float lm)
{
	float l = 1.0/LINES;
	float m = fract(y*l);
	if (m<l) return 1.0-L1*(1.0-lm*HYBRID);
	else if (m<2.0*l) return 1.0-L2*(1.0-lm*HYBRID);
	else if (m<3.0*l) return 1.0-L3*(1.0-lm*HYBRID);
	else return   1.0-L4*(1.0-lm*HYBRID);
	
}
float scanline3(float y, float lm)
{
	float l = 1.0/LINES;
	float m = fract(y*l);
	if (m<l) return 1.0-L1*(1.0-lm*HYBRID);
	else if (m<2.0*l) return 1.0-L2*(1.0-lm*HYBRID);
	else return   1.0-L3*(1.0-lm*HYBRID);
	
}

float scanline2(float y, float lm)
{
	float l = 1.0/LINES;
	float m = fract(y*l);
	if (m<l) return 1.0-L1*(1.0-lm*HYBRID);
	else return 1.0-L2*(1.0-lm*HYBRID);
		
}

void main()
{
	vec2 texcoord = vTexCoord.xy;
	vec2 ofactor = OutputSize.xy/OriginalSize.xy;
	vec2 intfactor;
	if (OVERSCAN == 0.0) intfactor = floor(ofactor); else intfactor = ceil(ofactor);
	vec2 diff = ofactor/intfactor;
	float scan = diff.y;
	texcoord = Overscan(texcoord, scan, scan);

   vec2 OGL2Pos = vTexCoord.xy*OutputSize.xy;
   if (TATE == 1.0) OGL2Pos = vec2(OGL2Pos.y,OGL2Pos.x);
	
	vec3 res = texture(Source, texcoord).rgb;
	vec3 lumweight = vec3 (0.2126,0.7152,0.0722);
	float lum = dot(res, lumweight);

	if (LINES == 5.0) {res *= scanline5(OGL2Pos.y, lum);}
	else if (LINES == 4.0){res *= scanline4(OGL2Pos.y,lum);}
	else if (LINES == 3.0){res *= scanline3(OGL2Pos.y,lum);}
	else if (LINES == 2.0){res *= scanline2(OGL2Pos.y,lum);}

   FragColor = vec4(res,1.0);
}
