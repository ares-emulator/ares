#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
	float noScanlines;
	float tvVerticalResolution;
	float blackLevel;
	float contrast;
	float compositeConnection;
} params;

#pragma parameter noScanlines "No Scanlines" 0.0 0.0 1.0 1.0
#pragma parameter tvVerticalResolution "TV Vertical Resolution" 250.0 20.0 1000.0 10.0
#pragma parameter blackLevel "Black Level" 0.07 -0.30 0.30 0.01
#pragma parameter contrast "Contrast" 1.0 0.0 2.0 0.1

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
} global;

////////////////////////////////////////////////////////
// GTU version 0.50
// Author: aliaspider - aliaspider@gmail.com
// License: GPLv3
////////////////////////////////////////////////////////

#include "config.h"

#define pi        3.14159265358
#define normalGauss(x) ((exp(-(x)*(x)*0.5))/sqrt(2.0*pi))

float normalGaussIntegral(float x)
{
   float a1 = 0.4361836;
   float a2 = -0.1201676;
   float a3 = 0.9372980;
   float p = 0.3326700;
   float t = 1.0 / (1.0 + p*abs(x));
   return (0.5-normalGauss(x) * (t*(a1 + t*(a2 + a3*t))))*sign(x);
}

vec3 scanlines( float x , vec3 c){
   float temp=sqrt(2*pi)*(params.tvVerticalResolution * params.SourceSize.w);

   float rrr=0.5*(params.SourceSize.y * params.OutputSize.w);
   float x1=(x+rrr)*temp;
   float x2=(x-rrr)*temp;
   c.r=(c.r*(normalGaussIntegral(x1)-normalGaussIntegral(x2)));
   c.g=(c.g*(normalGaussIntegral(x1)-normalGaussIntegral(x2)));
   c.b=(c.b*(normalGaussIntegral(x1)-normalGaussIntegral(x2)));
   c*=(params.OutputSize.y * params.SourceSize.w);
   return c;
}

#define Y(j) (offset.y-(j))
#define a(x) abs(x)
#define d(x,b) (pi*b*min(a(x)+0.5,1.0/b))
#define e(x,b) (pi*b*min(max(a(x)-0.5,-1.0/b),1.0/b))
#define STU(x,b) ((d(x,b)+sin(d(x,b))-e(x,b)-sin(e(x,b)))/(2.0*pi))

#define SOURCE(j) vec2(vTexCoord.x,vTexCoord.y - Y(j) * params.SourceSize.w)
#define C(j) (texture(Source, SOURCE(j)).xyz)


#define VAL(j) (C(j)*STU(Y(j),(params.tvVerticalResolution * params.SourceSize.w)))

#define VAL_scanlines(j) (scanlines(Y(j),C(j)))

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
	vec2	offset	=	fract((vTexCoord.xy * params.SourceSize.xy) - 0.5);
	vec3	tempColor	=	vec3(0.0);

   float range=ceil(0.5+params.SourceSize.y/params.tvVerticalResolution);

   float i;
   
   if (params.noScanlines > 0.0)
      for (i=-range;i<range+2.0;i++){
         tempColor+=VAL(i);
      }
   else
      for (i=-range;i<range+2.0;i++){
         tempColor+=VAL_scanlines(i);
      }
	tempColor-=vec3(params.blackLevel);
	tempColor*=(params.contrast/vec3(1.0-params.blackLevel));
   FragColor = vec4(tempColor, 1.0);
}