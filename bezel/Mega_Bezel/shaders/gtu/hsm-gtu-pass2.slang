#version 450

layout(push_constant) uniform Push
{
	vec4 OutputSize;
	vec4 OriginalSize;
	vec4 SourceSize;
   float GTU_ON;
	float signalResolution;
	float signalResolutionI;
	float signalResolutionQ;
	float compositeConnection;
} params;

#pragma parameter signalResolution     "          Signal Resolution Y" 256.0 16.0 1024.0 16.0
#pragma parameter signalResolutionI    "          Signal Resolution I" 83.0 1.0 350.0 2.0
#pragma parameter signalResolutionQ    "          Signal Resolution Q" 25.0 1.0 350.0 2.0

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

#define YIQ_to_RGB 	mat3x3( 1.0   , 1.0      , 1.0      ,	0.9563   , -0.2721   , -1.1070   ,		0.6210   , -0.6474   , 1.7046   )
#define pi        3.14159265358
#define a(x) abs(x)
#define d(x,b) (pi*b*min(a(x)+0.5,1.0/b))
#define e(x,b) (pi*b*min(max(a(x)-0.5,-1.0/b),1.0/b))
#define STU(x,b) ((d(x,b)+sin(d(x,b))-e(x,b)-sin(e(x,b)))/(2.0*pi))
//#define X(i) (offset-(i))
#define GETC (texture(Source, vec2(vTexCoord.x - X * params.SourceSize.z, vTexCoord.y)).rgb)

#define VAL_composite vec3((c.x*STU(X,(params.signalResolution * params.SourceSize.z))),(c.y*STU(X,(params.signalResolutionI * params.SourceSize.z))),(c.z*STU(X,(params.signalResolutionQ * params.SourceSize.z))))
#define VAL (c*STU(X,(params.signalResolution * params.SourceSize.z)))

#define PROCESS(i) X=(offset-(i));c=GETC;tempColor+=VAL;
#define PROCESS_composite(i) X=(offset-(i));c=GETC;tempColor+=VAL_composite;

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
   // If GTU is off return the color from the last pass
	if (params.GTU_ON < 1)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

	float offset   = fract((vTexCoord.x * params.SourceSize.x) - 0.5);
	vec3	tempColor = vec3(0.0);
	float	X;
	vec3	c;
	float range;
	if (params.compositeConnection > 0.0)
      range=ceil(0.5+params.SourceSize.x/min(min(params.signalResolution,params.signalResolutionI),params.signalResolutionQ));
   else
      range=ceil(0.5+params.SourceSize.x/params.signalResolution);
	  
	float i;
   if(params.compositeConnection > 0.0){
      for (i=-range;i<range+2.0;i++){
         PROCESS_composite((offset-(i)))
      }
   }
   else{
      for (i=-range;i<range+2.0;i++){
         PROCESS((offset-(i)))
      }
   }
   if(params.compositeConnection > 0.0)
      tempColor=clamp(tempColor * YIQ_to_RGB,0.0,1.0);
   else
      tempColor=clamp(tempColor,0.0,1.0);

   // tempColor=clamp(tempColor,0.0,1.0);
   FragColor = vec4(tempColor, 1.0);
}