#version 450

/*
	Bilateral v1.0
	by Sp00kyFox, 2014
Bilateral Filter, calculates a weighted mean of surrounding pixels based on color and spatial distance.
This can be used to smooth color transitions or blend dithering to some extent while preserving sharp edges.
Increasing the radius leads to more pixel lookups and therefore to a lower shader performance.
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float RAD;
	float CLR;
	float CWGHT;
} params;

#pragma parameter RAD "Bilateral Radius" 2.0 0.0 12.0 0.25
#define RAD params.RAD
#pragma parameter CLR "Bilateral Color Thresh" 0.15 0.01 1.0 0.01
#define CLR params.CLR
#pragma parameter CWGHT "Bilateral Central Wght" 0.25 0.0 2.0 0.05
#define CWGHT params.CWGHT

#define TEX(dx,dy) texture(Source, vTexCoord + vec2((dx),(dy)) * t1)
#define mul(a,b) (b*a)
#define saturate(c) clamp(c, 0.0, 1.0)

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const vec4 unit4  = vec4(1.0);

int   steps = int(ceil(RAD));
float clr   = -CLR * CLR;
float sigma = RAD * RAD / 2.0;
float cwght = 1.0 + CWGHT * max(1.0, 2.87029746*sigma + 0.43165242*RAD - 0.25219746);

float domain[13] = float[13](1.0, exp( -1.0/sigma), exp( -4.0/sigma), exp( -9.0/sigma), exp( -16.0/sigma), exp( -25.0/sigma), exp( -36.0/sigma),
				exp(-49.0/sigma), exp(-64.0/sigma), exp(-81.0/sigma), exp(-100.0/sigma), exp(-121.0/sigma), exp(-144.0/sigma));

float dist2(vec3 pt1, vec3 pt2)
{
	vec3 v = pt1 - pt2;
	return dot(v,v);
}

vec4 weight(int i, int j, vec3 org, mat4x3 A)
{
	return domain[i] * domain[j] * exp(vec4(dist2(org,A[0]), dist2(org,A[1]), dist2(org,A[2]), dist2(org,A[3]))/clr);
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 t1;

void main()
{
   gl_Position = global.MVP * Position;
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
	mat4x3 A, B;
	vec4 wghtA, wghtB;
	vec3 org = TEX(0.,0.).rgb, result = cwght*org;
	float  norm = cwght;
	
		for(int x = 1; x <= RAD; x++){
		
			A = mat4x3(TEX( x, 0.).rgb, TEX(-x, 0.).rgb, TEX( 0., x).rgb, TEX( 0.,-x).rgb);
			B = mat4x3(TEX( x, x).rgb, TEX( x,-x).rgb, TEX(-x, x).rgb, TEX(-x,-x).rgb);

			wghtA = weight(x, 0, org, A); wghtB = weight(x, x, org, B);	

			result += mul(wghtA, A)     + mul(wghtB, B);
			norm   += dot(wghtA, unit4) + dot(wghtB, unit4);
		
			for(int y = 1; y < x; y++){
					
				A = mat4x3(TEX( x, y).rgb, TEX( x,-y).rgb, TEX(-x, y).rgb, TEX(-x,-y).rgb);
				B = mat4x3(TEX( y, x).rgb, TEX( y,-x).rgb, TEX(-y, x).rgb, TEX(-y,-x).rgb);

				wghtA = weight(x, y, org, A); wghtB = weight(y, x, org, B);	

				result += mul(wghtA, A)     + mul(wghtB, B);
				norm   += dot(wghtA, unit4) + dot(wghtB, unit4);
			}
		}
    FragColor = vec4(result/norm, 1.0);
}