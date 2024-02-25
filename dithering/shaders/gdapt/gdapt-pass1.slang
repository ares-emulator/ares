#version 450

/*
   Genesis Dithering and Pseudo Transparency Shader v1.3 - Pass 1
   by Sp00kyFox, 2014

   Blends pixels based on detected dithering patterns.

*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float STEPS;
	float DEBUG;
	float linear_gamma;
} params;

#pragma parameter STEPS "GDAPT Error Prevention LVL"	1.0 0.0 5.0 1.0
#pragma parameter DEBUG "GDAPT Adjust View"		0.0 0.0 1.0 1.0
#pragma parameter linear_gamma "Use Linear Gamma"		0.0 0.0 1.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define TEX(dx,dy) texture(Source, vTexCoord+vec2((dx),(dy))*t1)

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
	vec4 C = TEX( 0, 0);
	vec4 L = TEX(-1, 0);
	vec4 R = TEX( 1, 0);
	
	if(params.linear_gamma > 0.5)
	{
		C.xyz = pow(C.xyz, vec3(2.2)).xyz;
		L.xyz = pow(L.xyz, vec3(2.2)).xyz;
		R.xyz = pow(R.xyz, vec3(2.2)).xyz;
	}
	
	float str = 0.0;

	if(params.STEPS == 0.0){
		str = C.w;
	}
	else if(params.STEPS == 1.0){
		str = min(max(L.w, R.w), C.w);
	}
	else if(params.STEPS == 2.0){
		str = min(max(min(max(TEX(-2,0).w, R.w), L.w), min(R.w, TEX(2,0).w)), C.w);				
	}
	else if(params.STEPS == 3.0){
		float tmp = min(R.w, TEX(2,0).w);
		str = min(max(min(max(min(max(TEX(-3,0).w, R.w), TEX(-2,0).w), tmp), L.w), min(tmp, TEX(3,0).w)), C.w);
	}
	else if(params.STEPS == 4.0){
		float tmp1 = min(R.w, TEX(2,0).w);
		float tmp2 = min(tmp1, TEX(3,0).w);
		str = min(max(min(max(min(max(min(max(TEX(-4,0).w, R.w), TEX(-3,0).w), tmp1), TEX(-2,0).w), tmp2), L.w), min(tmp2, TEX(4,0).w)), C.w);
	}
	else{
		float tmp1 = min(R.w, TEX(2,0).w);
		float tmp2 = min(tmp1, TEX(3,0).w);
		float tmp3 = min(tmp2, TEX(4,0).w);
		str = min(max(min(max(min(max(min(max(min(max(TEX(-5,0).w, R.w), TEX(-4,0).w), tmp1), TEX(-3,0).w), tmp2), TEX(-2,0).w), tmp3), L.w), min(tmp3, TEX(5,0).w)), C.w);
	}


	if(params.DEBUG > 0.5)
		FragColor = vec4(str);

	float sum  = L.w + R.w;
	float wght = max(L.w, R.w);
	      wght = (wght == 0.0) ? 1.0 : sum/wght;

   vec4 final = vec4(mix(C.xyz, (wght*C.xyz + L.w*L.xyz + R.w*R.xyz)/(wght + sum), str), 1.0);
   FragColor = final;
   if(params.linear_gamma > 0.5) FragColor = pow(final, vec4(1.0 / 2.2));
}