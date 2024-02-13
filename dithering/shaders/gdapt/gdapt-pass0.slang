#version 450

/*
   Genesis Dithering and Pseudo Transparency Shader v1.3 - Pass 0
   by Sp00kyFox, 2014

   Neighbor anaylsis via dot product of the difference vectors.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float MODE;
	float PWR;
} params;

#pragma parameter MODE "GDAPT Monochrome Analysis"	0.0 0.0 1.0 1.0
#pragma parameter PWR  "GDAPT Color Metric Exp"		2.0 0.0 10.0 0.1

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define dotfix(x,y) clamp(dot(x,y), 0.0, 1.0)	// NVIDIA Fix
#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*t1).xyz

// Reference: http://www.compuphase.com/cmetric.htm
float eq(vec3 A, vec3 B)
{
	vec3 diff = A-B;
	float  ravg = (A.x + B.x) * 0.5;

	diff *= diff * vec3(2.0 + ravg, 4.0, 3.0 - ravg);
	
	return pow( smoothstep(3.0, 0.0, sqrt(diff.x + diff.y + diff.z)), params.PWR );
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 t1;

void main()
{
   gl_Position = global.MVP * vec4(Position.xy, 0.0, 1.0);
   vTexCoord = TexCoord;
   t1 = params.SourceSize.zw;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 t1;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec3 C = TEX( 0, 0);
	vec3 L = TEX(-1, 0);
	vec3 R = TEX( 1, 0);

	float tag = 0.0;

	if(params.MODE > 0.5){
		tag = ((L == R) && (C != L)) ? 1.0 : 0.0;
	}
	else{
		tag = dotfix(normalize(C-L), normalize(C-R)) * eq(L,R);
	}

   FragColor = vec4(C, tag);
}
