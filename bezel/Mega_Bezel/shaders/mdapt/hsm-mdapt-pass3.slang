#version 450

/*
   Merge Dithering and Pseudo Transparency Shader v2.8 - Pass 3
   by Sp00kyFox, 2014

   Backpropagation and checkerboard smoothing.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    float HSM_DEDITHER_MODE;
} params;


layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma parameter HSM_DEDITHER_MODE "Mode: OFF| STRICT | RELAXED | HYLLIAN | HYL + STRIPES | STRIPES"	0 0 5 1
#define HSM_DEDITHER_MODE  params.HSM_DEDITHER_MODE

#define TEX(dx,dy)   texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)
#define TEXt0(dx,dy) texture(PreMdaptPass, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)

bool eq(vec3 A, vec3 B){
	return (A == B);
}

float and_(float a, float b){
	return min(a,b);
}

float or_(float a, float b, float c, float d, float e, float f, float g, float h, float i){
	return max(a, max(b, max(c, max(d, max(e, max(f, max(g, max(h,i))))))));
}

vec2 and_(vec2 a, vec2 b){
	return min(a,b);
}

vec2 or_(vec2 a, vec2 b){
	return max(a,b);
}

vec2 or_(vec2 a, vec2 b, vec2 c, vec2 d){
	return max(a, max(b, max(c,d)));
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
layout(set = 0, binding = 3) uniform sampler2D PreMdaptPass;

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


	vec4 C  = TEX( 0., 0.);		vec3 c  = TEXt0( 0., 0.).xyz;
	vec2 L  = TEX(-1., 0.).xy;	vec3 l  = TEXt0(-1., 0.).xyz;
	vec2 R  = TEX( 1., 0.).xy;	vec3 r  = TEXt0( 1., 0.).xyz;
	vec2 U  = TEX( 0.,-1.).xy;	vec3 u  = TEXt0( 0.,-1.).xyz;
	vec2 D  = TEX( 0., 1.).xy;	vec3 d  = TEXt0( 0., 1.).xyz;
	float UL = TEX(-1.,-1.).y;	vec3 ul = TEXt0(-1.,-1.).xyz;
	float UR = TEX( 1.,-1.).y;	vec3 ur = TEXt0( 1.,-1.).xyz;
	float DL = TEX(-1., 1.).y;	vec3 dl = TEXt0(-1., 1.).xyz;
	float DR = TEX( 1., 1.).y;	vec3 dr = TEXt0( 1., 1.).xyz;

	// Backpropagation
	C.xy = or_(C.xy, and_(C.zw, or_(L, R, U, D)));

	// Checkerboard Smoothing
	C.y = or_(C.y, min(U.y, float(eq(c,u))), min(D.y, float(eq(c,d))), min(L.y, float(eq(c,l))), min(R.y, float(eq(c,r))), min(UL, float(eq(c,ul))), min(UR, float(eq(c,ur))), min(DL, float(eq(c,dl))), min(DR, float(eq(c,dr))));

   FragColor = vec4(C);
}