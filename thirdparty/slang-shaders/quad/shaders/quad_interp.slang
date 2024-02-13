#version 450

/*
   Author: Themaister
   License: Public domain
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float QUAD_INTERP_RESOLUTION_X;
	float QUAD_INTERP_RESOLUTION_Y;
	float QUAD_INTERP_SHARPNESS;
} params;

#pragma parameter QUAD_INTERP_RESOLUTION_X "QuadInterp Input Resolution X" 0.0 0.0 1920.0 1.0
#pragma parameter QUAD_INTERP_RESOLUTION_Y "QuadInterp Input Resolution Y" 0.0 0.0 1920.0 1.0
#pragma parameter QUAD_INTERP_SHARPNESS     "QuadInterp Sharpness"           2.01 0.0 10.0 0.01

#define QUAD_INTERP_RESOLUTION_X params.QUAD_INTERP_RESOLUTION_X
#define QUAD_INTERP_RESOLUTION_Y params.QUAD_INTERP_RESOLUTION_Y
#define QUAD_INTERP_SHARPNESS params.QUAD_INTERP_SHARPNESS

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define saturate(c) clamp(c, 0.0, 1.0)
#define lerp(c) mix(c)
#define mul(a,b) (b*a)
#define fmod(c) mod(c)
#define frac(c) fract(c)
#define tex2D(c,d) texture(c,d)
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4
#define float2x2 mat2x2
#define float3x3 mat3x3
#define float4x4 mat4x4

#define s0 Source
#define tex vTexCoord

#define QUAD_INTERP_RESOLUTION_X_DEF params.SourceSize.x
#define QUAD_INTERP_RESOLUTION_Y_DEF params.SourceSize.y

float3 quad_inter(float3 x0, float3 x1, float3 x2, float x)
{
   float3 poly[3];
   poly[2] =  0.5 * x0 - x1 + 0.5*x2;
   poly[1] = -1.5 * x0 + 2.0 * x1 - 0.5*x2;
   poly[0] = x0;
   return poly[2] * x * x + poly[1] * x + poly[0];
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
   /* messy I know but we need to make it possible to have it default to input resolution x/y in case RESOLUTION_X is 0.0 */
	float2 texsize = float2(1.0/((QUAD_INTERP_RESOLUTION_X == 0) ? QUAD_INTERP_RESOLUTION_X_DEF : QUAD_INTERP_RESOLUTION_X), 1.0/((QUAD_INTERP_RESOLUTION_Y == 0) ? QUAD_INTERP_RESOLUTION_Y_DEF : QUAD_INTERP_RESOLUTION_Y));
   float dx = float(pow(QUAD_INTERP_SHARPNESS * texsize.x, -1.0));
   float dy = float(pow(QUAD_INTERP_SHARPNESS * texsize.y, -1.0));

   float3 c00 = tex2D(s0, tex + float2(-dx, -dy)).xyz;
   float3 c01 = tex2D(s0, tex + float2(-dx, 0)).xyz;
   float3 c02 = tex2D(s0, tex + float2(-dx, dy)).xyz;
   float3 c10 = tex2D(s0, tex + float2(0, -dy)).xyz;
   float3 c11 = tex2D(s0, tex + float2(0, 0)).xyz;
   float3 c12 = tex2D(s0, tex + float2(0, dy)).xyz;
   float3 c20 = tex2D(s0, tex + float2(dx, -dy)).xyz;
   float3 c21 = tex2D(s0, tex + float2(dx, 0)).xyz;
   float3 c22 = tex2D(s0, tex + float2(dx, dy)).xyz;

   float frac_amt_x = frac(tex.x * texsize.x);
   float frac_amt_y = frac(tex.y * texsize.y);
   float3 loval = quad_inter(c00, c10, c20, frac_amt_x + 0.5);
   float3 midval = quad_inter(c01, c11, c21, frac_amt_x + 0.5);
   float3 hival = quad_inter(c02, c12, c22, frac_amt_x + 0.5);
   float3 res = quad_inter(loval, midval, hival, frac_amt_y + 0.5);

// Bilinear!
//   float3 first = lerp(c00, c20, frac(tex.x * texsize.x + 0.5));
//   float3 second = lerp(c02, c22, frac(tex.x * texsize.x + 0.5));
//   float3 res = lerp(first, second, frac(tex.y * texsize.y + 0.5));
//   OUT.color = float4(res, 1.0);

   FragColor = vec4(res, 1.0);
}