#version 450

/*
convergence pass DariusG 2023. 
Run in Linear, BEFORE actual shader pass
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	float C_STR,Rx,Ry,Gx,Gy,Bx,By;
} params;

#pragma parameter C_STR "Convergence Overall Strength" 0.5 0.0 0.5 0.05
#pragma parameter Rx "Convergence Red Horiz." 0.9 -5.0 5.0 0.05
#pragma parameter Ry "Convergence Red Vert." 0.05 -5.0 5.0 0.05
#pragma parameter Gx "Convergence Green Horiz." -0.95 -5.0 5.0 0.05
#pragma parameter Gy "Convergence Green Vert." -0.05 -5.0 5.0 0.05
#pragma parameter Bx "Convergence Blue Horiz." 0.9 -5.0 5.0 0.05
#pragma parameter By "Convergence Blue Vert." 0.05 -5.0 5.0 0.05

#define C_STR params.C_STR
#define Rx params.Rx
#define Ry params.Ry
#define Gx params.Gx
#define Gy params.Gy
#define Bx params.Bx
#define By params.By

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 FinalViewportSize;
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
layout(set = 0, binding = 1) uniform sampler2D Source;


void main()
{
vec2 dx = vec2(params.SourceSize.z,0.0);
vec2 dy = vec2(0.0,params.SourceSize.w);
vec2 pos = vTexCoord;
vec3 res0 = texture(Source,pos).rgb;
float resr = texture(Source,pos + dx*Rx + dy*Ry).r;
float resg = texture(Source,pos + dx*Gx + dy*Gy).g;
float resb = texture(Source,pos + dx*Bx + dy*By).b;

vec3 res = vec3(  res0.r*(1.0-C_STR) +  resr*C_STR,
                  res0.g*(1.0-C_STR) +  resg*C_STR,
                  res0.b*(1.0-C_STR) +  resb*C_STR 
                   );
FragColor.rgb = res;    
}
