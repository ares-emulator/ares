#version 450

/*
   Merge Dithering and Pseudo Transparency Shader v2.8 - Pass 0
   by Sp00kyFox, 2014

   Neighbor analysis via color metric and dot product of the difference vectors.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float MODE;
	float PWR;
    float HSM_DEDITHER_MODE;
} params;


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter HSM_DEDITHERING_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter HSM_DEDITHERING_TITLE "[ --- MDAPT & HYLLIAN DE-DITHERING --- ]:" 0 0 0.01 0.01

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

// OFF | STRICT | RELAXED | HYLLIAN | HYL + Stripes | Stripes
// #pragma parameter MODE "MDAPT Monochrome Analysis"	0.0 0.0 1.0 1.0
// #pragma parameter PWR  "MDAPT Color Metric Exp"		2.0 0.0 10.0 0.1

const float PWR = 2;

#define dotfix(x,y) clamp(dot(x,y), 0.0, 1.0)	// NVIDIA Fix
#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)

// Reference: http://www.compuphase.com/cmetric.htm
float eq(vec3 A, vec3 B)
{
	vec3 diff = A-B;
	float  ravg = (A.x + B.x) * 0.5;

	diff *= diff * vec3(2.0 + ravg, 4.0, 3.0 - ravg);
	
	return pow( smoothstep(3.0, 0.0, sqrt(diff.x + diff.y + diff.z)), PWR );
}

float and_(float a, float b, float c, float d, float e, float f){
	return min(a, min(b, min(c, min(d, min(e,f)))));
}

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
	/*
		  U
		L C R
		  D	
	*/

	if (HSM_DEDITHER_MODE < 1 || HSM_DEDITHER_MODE == 3)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

	vec3 C = TEX( 0., 0.).xyz;
	vec3 L = TEX(-1., 0.).xyz;
	vec3 R = TEX( 1., 0.).xyz;
	vec3 U = TEX( 0.,-1.).xyz;
	vec3 D = TEX( 0., 1.).xyz;


	vec3 res = vec3(0.0);

	if(HSM_DEDITHER_MODE == 1){
		res.x = float((L == R) && (C != L));
		res.y = float((U == D) && (C != U));
		res.z = float(bool(res.x) && bool(res.y) && (L == U));
	}
	else{
		vec3 dCL = normalize(C-L), dCR = normalize(C-R), dCD = normalize(C-D), dCU = normalize(C-U);

		res.x = dotfix(dCL, dCR) * eq(L,R);
		res.y = dotfix(dCU, dCD) * eq(U,D);
		res.z = and_(res.x, res.y, dotfix(dCL, dCU) * eq(L,U), dotfix(dCL, dCD) * eq(L,D), dotfix(dCR, dCU) * eq(R,U), dotfix(dCR, dCD) * eq(R,D));
	}

   FragColor = vec4(res, 1.0);
}
