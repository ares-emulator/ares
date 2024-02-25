#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float FB_RES;
	float SIGMA_R;
	float SIGMA_D;
} params;

#pragma parameter FB_RES "Bilateral Internal Res" 2.0 1.0 8.0 1.0
#pragma parameter SIGMA_R "Bilateral Blur" 0.4 0.0 2.0 0.1
#pragma parameter SIGMA_D "Bilateral Space" 3.0 0.0 10.0 0.2

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

#define GET(M,K) (texture(Source,tc+M*dx+K*dy).xyz)

#define BIL(M,K) {\
	col=GET(M,K);\
	ds=M*M+K*K;\
	weight=exp(-ds/sd2)*exp(-(col-center)*(col-center)/si2);\
	color+=(weight*col);\
	wsum+=weight;\
	}

void main()
{
      float ds, sd2, si2;
      float sigma_d = params.SIGMA_D;
      float sigma_r = params.SIGMA_R*0.04;

      vec3 color = vec3(0.0, 0.0, 0.0);
      vec3 wsum = vec3(0.0, 0.0, 0.0);
      vec3 weight;

      vec2 dx = vec2(params.FB_RES, 0.0) * params.SourceSize.zw;
      vec2 dy = vec2(0.0, params.FB_RES) * params.SourceSize.zw;

      sd2 = 2.0 * sigma_d * sigma_d;
      si2 = 2.0 * sigma_r * sigma_r;

      vec2 tc = vTexCoord;

      vec3 col;
      vec3 center = GET(0,0);
     // center = sqrt(center);

      BIL(-2,-2)
      BIL(-1,-2)
      BIL( 0,-2)
      BIL( 1,-2)
      BIL( 2,-2)
      BIL(-2,-1)
      BIL(-1,-1)
      BIL( 0,-1)
      BIL( 1,-1)
      BIL( 2,-1)
      BIL(-2, 0)
      BIL(-1, 0)
      BIL( 0, 0)
      BIL( 1, 0)
      BIL( 2, 0)
      BIL(-2, 1)
      BIL(-1, 1)
      BIL( 0, 1)
      BIL( 1, 1)
      BIL( 2, 1)
      BIL(-2, 2)
      BIL(-1, 2)
      BIL( 0, 2)
      BIL( 1, 2)
      BIL( 2, 2)

      // Weight normalization
      color /= wsum;

      FragColor = vec4(color, 1.);
}