#version 450

/*
   Merge Dithering and Pseudo Transparency Shader v2.8 - Pass 1
   by Sp00kyFox, 2014

   Preparing checkerboard patterns.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float HSM_DEDITHER_MODE;
} params;

#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

float and_(float a, float b){
	return min(a,b);
}

float and_(float a, float b, float c){
	return min(a, min(b,c));
}

float or_(float a, float b){
	return max(a,b);
}

float or_(float a, float b, float c, float d, float e){
	return max(a, max(b, max(c, max(d,e))));
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
		UL U UR
		L  C R
		DL D DR
	*/

	if (HSM_DEDITHER_MODE < 1 || HSM_DEDITHER_MODE == 3)
	{
		FragColor = texture(Source, vTexCoord);
		return;
	}

	vec3 C = TEX( 0., 0.).xyz;
	vec3 L = TEX(-1., 0.).xyz;
	vec3 R = TEX( 1., 0.).xyz;
	vec3 D = TEX( 0., 1.).xyz;
	vec3 U = TEX( 0.,-1.).xyz;
	
	float UL = TEX(-1.,-1.).z;
	float UR = TEX( 1.,-1.).z;
	float DL = TEX(-1., 1.).z;
	float DR = TEX( 1., 1.).z;

	// Checkerboard Pattern Completion
	float prCB = or_(C.z,
		and_(L.z, R.z, or_(U.x, D.x)),
		and_(U.z, D.z, or_(L.y, R.y)),
		and_(C.x, or_(and_(UL, UR), and_(DL, DR))),
		and_(C.y, or_(and_(UL, DL), and_(UR, DR))));
   FragColor = vec4(C.x, prCB, 0.0, 0.0);
}