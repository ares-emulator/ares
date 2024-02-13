#version 450

/*
   Merge Dithering and Pseudo Transparency Shader v2.8 - Pass 4
   by Sp00kyFox, 2014

   Blends pixels based on detected dithering patterns.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float VL;
	float CB;
	float DEBUG;
	float linear_gamma;
} params;

#pragma parameter VL    "MDAPT Vertical Lines"	0.0 0.0 1.0 1.0
#pragma parameter CB    "MDAPT Checkerboard"	1.0 0.0 1.0 1.0
#pragma parameter DEBUG "MDAPT Adjust View"	0.0 0.0 1.0 1.0
#pragma parameter linear_gamma "MDAPT Linear Gamma Blend"	0.0 0.0 1.0 1.0

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
layout(set = 0, binding = 3) uniform sampler2D mdapt_refpass;

//#define TEX(dx,dy)   texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)
//#define TEXt0(dx,dy) texture(mdapt_refpass, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw)

vec4 TEX(float dx, float dy){
	if(params.linear_gamma > 0.5) return pow(texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw), vec4(2.2));
	else return texture(Source, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw);
}

vec4 TEXt0(float dx, float dy){
	if(params.linear_gamma > 0.5) return pow(texture(mdapt_refpass, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw), vec4(2.2));
	else return texture(mdapt_refpass, vTexCoord+vec2((dx),(dy))*params.SourceSize.zw);
}

bool eq(vec3 A, vec3 B){
	return (A == B);
}

float and_(float a, float b){
	return min(a,b);
}

float or_(float a, float b){
	return max(a,b);
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

void main()
{
	/*
		UL U UR
		L  C R
		DL D DR	
	*/

	vec4 C = TEX( 0., 0.);		vec3 c = TEXt0( 0., 0.).xyz;
	vec2 L = TEX(-1., 0.).xy;	vec3 l = TEXt0(-1., 0.).xyz;
	vec2 R = TEX( 1., 0.).xy;	vec3 r = TEXt0( 1., 0.).xyz;
	vec2 U = TEX( 0.,-1.).xy;
	vec2 D = TEX( 0., 1.).xy;

	float  prVL = 0.0,		prCB = 0.0;
	vec3 fVL  = vec3(0.0),	fCB  = vec3(0.0);


	// Backpropagation
	C.xy = or_(C.xy, and_(C.zw, or_(L.xy, R.xy, U.xy, D.xy)));


	if(params.VL > 0.5){
		float prSum = L.x + R.x;		

		prVL = max(L.x, R.x);
		prVL = (prVL == 0.0) ? 1.0 : prSum/prVL;
	
		fVL  = (prVL*c + L.x*l + R.x*r)/(prVL + prSum);
		prVL = C.x;
	}


	if(params.CB > 0.5){	
		vec3 u = TEXt0( 0.,-1.).xyz;
		vec3 d = TEXt0( 0., 1.).xyz;

		float eqCL = float(eq(c,l));
		float eqCR = float(eq(c,r));
		float eqCU = float(eq(c,u));
		float eqCD = float(eq(c,d));

		float prU = or_(U.y, eqCU);
		float prD = or_(D.y, eqCD);
		float prL = or_(L.y, eqCL);
		float prR = or_(R.y, eqCR);


		float prSum = prU  + prD  + prL  + prR;

		prCB = max(prL, max(prR, max(prU,prD)));
		prCB = (prCB == 0.0) ? 1.0 : prSum/prCB; 
		
		//standard formula: C/2 + (L + R + D + U)/8
		fCB = (prCB*c + prU*u + prD*d + prL*l + prR*r)/(prCB + prSum);


		float UL = TEX(-1.,-1.).y;	vec3 ul = TEXt0(-1.,-1.).xyz;
		float UR = TEX( 1.,-1.).y;	vec3 ur = TEXt0( 1.,-1.).xyz;
		float DL = TEX(-1., 1.).y;	vec3 dl = TEXt0(-1., 1.).xyz;
		float DR = TEX( 1., 1.).y;	vec3 dr = TEXt0( 1., 1.).xyz;

		// Checkerboard Smoothing
		prCB = or_(C.y, and_(L.y, eqCL), and_(R.y, eqCR), and_(U.y, eqCU), and_(D.y, eqCD), and_(UL, float(eq(c,ul))), and_(UR, float(eq(c,ur))), and_(DL, float(eq(c,dl))), and_(DR, float(eq(c,dr))));
	}

	if(params.DEBUG > 0.5)
		FragColor = vec4(prVL, prCB, 0.0, 0.0);
		
	vec4 final = (prCB >= prVL) ? vec4(mix(c, fCB, prCB), 1.0) : vec4(mix(c, fVL, prVL), 1.0);
	FragColor = (params.linear_gamma > 0.5) ? pow(final, vec4(1.0 / 2.2)) : final;
}
