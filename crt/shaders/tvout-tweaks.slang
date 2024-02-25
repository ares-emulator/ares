#version 450

///////////////
//	TV-out tweaks	
//	Author: aliaspider - aliaspider@gmail.com
//	License: GPLv3      
////////////////////////////////////////////////////////


// this shader is meant to be used when running
// an emulator on a real CRT-TV @240p or @480i
////////////////////////////////////////////////////////

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float TVOUT_RESOLUTION;
	float TVOUT_COMPOSITE_CONNECTION;
	float TVOUT_TV_COLOR_LEVELS;
	float TVOUT_RESOLUTION_Y;
	float TVOUT_RESOLUTION_I;
	float TVOUT_RESOLUTION_Q;
} params;

// Basic settings:

// signal resolution
// higher = sharper 
#pragma parameter TVOUT_RESOLUTION "TVOut Signal Resolution" 256.0 0.0 1024.0 32.0 // default, minimum, maximum, optional step

// simulate a composite connection instead of RGB
#pragma parameter TVOUT_COMPOSITE_CONNECTION "TVOut Composite Enable" 0.0 0.0 1.0 1.0

// use TV video color range (16-235) 
// instead of PC full range (0-255)
#pragma parameter TVOUT_TV_COLOR_LEVELS "TVOut TV Color Levels Enable" 0.0 0.0 1.0 1.0
////////////////////////////////////////////////////////

////////////////////////////////////////////////////////
// Advanced settings:
//
// these values will be used instead 
// if COMPOSITE_CONNECTION is defined
// to simulate different signal resolutions(bandwidth)
// for luma (Y) and chroma ( I and Q )
// this is just an approximation
// and will only simulate the low bandwidth anspect of
// composite signal, not the crosstalk between luma and chroma
// Y = 4MHz I=1.3MHz Q=0.4MHz
#pragma parameter TVOUT_RESOLUTION_Y "TVOut Luma (Y) Resolution" 256.0 0.0 1024.0 32.0
#pragma parameter TVOUT_RESOLUTION_I "TVOut Chroma (I) Resolution" 83.2 0.0 256.0 8.0
#pragma parameter TVOUT_RESOLUTION_Q "TVOut Chroma (Q) Resolution" 25.6 0.0 256.0 8.0

// formula is MHz=resolution*15750Hz
// 15750Hz being the horizontal Frequency of NTSC
// (=262.5*60Hz)
////////////////////////////////////////////////////////

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

mat3x3 RGB_to_YIQ = mat3x3(
         0.299,0.587,0.114, 
		 0.595716,-0.274453,-0.321263,
		 0.211456,-0.522591, 0.311135);
mat3x3 YIQ_to_RGB = mat3x3(
         1.0,0.9563,0.6210, 
		 1.0,-0.2721,-0.6474,
		 1.0,-1.1070, 1.7046);
		 
#define pi			3.14159265358
#define a(x) abs(x)
#define d(x,b) (pi*b*min(a(x)+0.5,1.0/b))
#define e(x,b) (pi*b*min(max(a(x)-0.5,-1.0/b),1.0/b))
#define STU(x,b) ((d(x,b)+sin(d(x,b))-e(x,b)-sin(e(x,b)))/(2.0*pi))
//#define X(i) (offset-(i))
#define L(C) clamp((C -16.5/ 256.0)*256.0/(236.0-16.0),0.0,1.0)
#define LCHR(C) clamp((C -16.5/ 256.0)*256.0/(240.0-16.0),0.0,1.0)

#define mul(a,b) (b*a)

vec3 LEVELS(vec3 c0)
{
   if (params.TVOUT_TV_COLOR_LEVELS > 0.5)
   {
      if (params.TVOUT_COMPOSITE_CONNECTION > 0.5)
         return vec3(L(c0.x),LCHR(c0.y),LCHR(c0.z));
      else
         return L(c0);
   }
   else
      return c0;
}

#define GETC(c) \
   if (params.TVOUT_COMPOSITE_CONNECTION > 0.5) \
      c = mul(RGB_to_YIQ,LEVELS(texture(Source, vec2(vTexCoord.x - X*oneT,vTexCoord.y)).xyz)); \
   else \
      c = (LEVELS(texture(Source, vec2(vTexCoord.x - X*oneT,vTexCoord.y)).xyz))

#define VAL(tempColor) \
   if (params.TVOUT_COMPOSITE_CONNECTION > 0.5) \
      tempColor += vec3((c.x*STU(X,(params.TVOUT_RESOLUTION_Y*oneI))),(c.y*STU(X,(params.TVOUT_RESOLUTION_I*oneI))),(c.z*STU(X,(params.TVOUT_RESOLUTION_Q*oneI)))); \
   else \
      tempColor += (c*STU(X,(params.TVOUT_RESOLUTION*oneI)))

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

void main()
{
   vec3	tempColor	=	vec3(0.0,0.0,0.0);
   float	offset	=	fract((vTexCoord.x * params.SourceSize.x) - 0.5);
   float	oneT	=	params.SourceSize.z;
   float	oneI	=	params.SourceSize.z;

   float X;
   vec3 c;

   X = (offset-(-1));
   GETC(c);
   VAL(tempColor);

   X = (offset-(0));
   GETC(c);
   VAL(tempColor);

   X = (offset-(1));
   GETC(c);
   VAL(tempColor);

   X = (offset-(2));
   GETC(c);
   VAL(tempColor);

   if (params.TVOUT_COMPOSITE_CONNECTION > 0.5)
      tempColor=mul(YIQ_to_RGB,tempColor);
   FragColor = vec4(tempColor, 1.0);
}