#version 450

/*
   Merge Dithering and Pseudo Transparency Shader v2.8 - Pass 2
   by Sp00kyFox, 2014

   Eliminating isolated detections.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float VL_LO;
	float VL_HI;
	float CB_LO;
	float CB_HI;
    float HSM_DEDITHER_MODE;
} params;

// #pragma parameter VL_LO "MDAPT VL LO Thresh" 1.25 0.0 10.0 0.05
// #pragma parameter VL_HI "MDAPT VL HI Thresh" 1.75 0.0 10.0 0.05
// #pragma parameter CB_LO "MDAPT CB LO Thresh" 5.25 0.0 25.0 0.05
// #pragma parameter CB_HI "MDAPT CB HI Thresh" 5.75 0.0 25.0 0.05

const float VL_LO = 1.25;
const float VL_HI = 1.75;
const float CB_LO = 5.25;
const float CB_HI = 5.75;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)
#define and(x,y) min(x,y)
#define or(x,y)  max(x,y)

vec2 sigmoid(vec2 signal){
	return smoothstep(vec2(VL_LO, CB_LO), vec2(VL_HI, CB_HI), signal);
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
		NW  UUL U2 UUR NE
		ULL UL  U1 UR  URR
		L2  L1  C  R1  R2
		DLL DL  D1 DR  DRR	
		SW  DDL D2 DDR SE
	*/

	if (HSM_DEDITHER_MODE < 1 || HSM_DEDITHER_MODE == 3)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}


	vec2 C = TEX( 0., 0.).xy;
	vec2 hits = vec2(0.0);
	
	//phase 1
	vec2 L1 = TEX(-1., 0.).xy;
	vec2 R1 = TEX( 1., 0.).xy;
	vec2 U1 = TEX( 0.,-1.).xy;
	vec2 D1 = TEX( 0., 1.).xy;

	//phase 2
	vec2 L2 = and(TEX(-2., 0.).xy, L1);
	vec2 R2 = and(TEX( 2., 0.).xy, R1);
	vec2 U2 = and(TEX( 0.,-2.).xy, U1);
	vec2 D2 = and(TEX( 0., 2.).xy, D1);
	vec2 UL = and(TEX(-1.,-1.).xy, or(L1, U1));
	vec2 UR = and(TEX( 1.,-1.).xy, or(R1, U1));
	vec2 DL = and(TEX(-1., 1.).xy, or(L1, D1));
	vec2 DR = and(TEX( 1., 1.).xy, or(R1, D1));

	//phase 3
	vec2 ULL = and(TEX(-2.,-1.).xy, or(L2, UL));
	vec2 URR = and(TEX( 2.,-1.).xy, or(R2, UR));
	vec2 DRR = and(TEX( 2., 1.).xy, or(R2, DR));
	vec2 DLL = and(TEX(-2., 1.).xy, or(L2, DL));
	vec2 UUL = and(TEX(-1.,-2.).xy, or(U2, UL));
	vec2 UUR = and(TEX( 1.,-2.).xy, or(U2, UR));
	vec2 DDR = and(TEX( 1., 2.).xy, or(D2, DR));
	vec2 DDL = and(TEX(-1., 2.).xy, or(D2, DL));

	//phase 4
	hits += and(TEX(-2.,-2.).xy, or(UUL, ULL));
	hits += and(TEX( 2.,-2.).xy, or(UUR, URR));
	hits += and(TEX(-2., 2.).xy, or(DDL, DLL));
	hits += and(TEX( 2., 2.).xy, or(DDR, DRR));

	hits += (ULL + URR + DRR + DLL + L2 + R2) + vec2(0.0, 1.0) * (C + U1 + U2 + D1 + D2 + L1 + R1 + UL + UR + DL + DR + UUL + UUR + DDR + DDL);

   FragColor = vec4(C * sigmoid(hits), C);
}