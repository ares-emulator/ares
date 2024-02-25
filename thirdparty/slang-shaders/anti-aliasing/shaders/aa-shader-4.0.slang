#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float INTERNAL_RES;
} params;

#pragma parameter INTERNAL_RES "Internal Resolution" 1.0 1.0 8.0 1.0

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const vec3 dt = vec3(1.0,1.0,1.0);

vec3 texture2d_(sampler2D tex, vec2 coord, vec4 yx) {

	vec3 s00 = texture(tex, coord + yx.zw).xyz; 
	vec3 s20 = texture(tex, coord + yx.xw).xyz; 
	vec3 s22 = texture(tex, coord + yx.xy).xyz; 
	vec3 s02 = texture(tex, coord + yx.zy).xyz; 

	float m1=dot(abs(s00-s22),dt)+0.001;
	float m2=dot(abs(s02-s20),dt)+0.001;

	return 0.5*(m2*(s00+s22)+m1*(s02+s20))/(m1+m2);
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
	// Calculating texel coordinates
	vec2 size     = 4.0 * params.SourceSize.xy / params.INTERNAL_RES;
//	vec2 size     = X * (params.OutputSize.xy * params.SourceSize.zw) * params.SourceSize.xy;
	vec2 inv_size = 1.0 / size;

	vec4 yx = vec4(inv_size, -inv_size);
	
	vec2 OGL2Pos = vTexCoord * size;

	vec2 fp = fract(OGL2Pos);
	vec2 dx = vec2(inv_size.x,0.0);
	vec2 dy = vec2(0.0, inv_size.y);
	vec2 g1 = vec2(inv_size.x,inv_size.y);
	vec2 g2 = vec2(-inv_size.x,inv_size.y);
	
	vec2 pC4 = floor(OGL2Pos) * 1.0001 * inv_size;	
	
	// Reading the texels
	vec3 C1 = texture2d_(Source, pC4 - dy, yx);
	vec3 C0 = texture2d_(Source, pC4 - g1, yx); 
	vec3 C2 = texture2d_(Source, pC4 - g2, yx);
	vec3 C3 = texture2d_(Source, pC4 - dx, yx);
	vec3 C4 = texture2d_(Source, pC4     , yx);
	vec3 C5 = texture2d_(Source, pC4 + dx, yx);
	vec3 C6 = texture2d_(Source, pC4 + g2, yx);
	vec3 C7 = texture2d_(Source, pC4 + dy, yx);
	vec3 C8 = texture2d_(Source, pC4 + g1, yx);
	
	vec3 ul, ur, dl, dr;
	float m1, m2;
	
	m1 = dot(abs(C0-C4),dt)+0.001;
	m2 = dot(abs(C1-C3),dt)+0.001;
	ul = (m2*(C0+C4)+m1*(C1+C3))/(m1+m2);  
	
	m1 = dot(abs(C1-C5),dt)+0.001;
	m2 = dot(abs(C2-C4),dt)+0.001;
	ur = (m2*(C1+C5)+m1*(C2+C4))/(m1+m2);
	
	m1 = dot(abs(C3-C7),dt)+0.001;
	m2 = dot(abs(C6-C4),dt)+0.001;
	dl = (m2*(C3+C7)+m1*(C6+C4))/(m1+m2);
	
	m1 = dot(abs(C4-C8),dt)+0.001;
	m2 = dot(abs(C5-C7),dt)+0.001;
	dr = (m2*(C4+C8)+m1*(C5+C7))/(m1+m2);
	
	vec3 c11 = 0.5*((dr*fp.x+dl*(1-fp.x))*fp.y+(ur*fp.x+ul*(1-fp.x))*(1-fp.y) );
	
   FragColor = vec4(c11, 1.0);
}
