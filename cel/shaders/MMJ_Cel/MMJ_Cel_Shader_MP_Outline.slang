#version 450

#pragma name MMJ_OutlinePass

/*
----------------------------------------------------------------
MMJ's Cel Shader v2.0 - Multi-Pass 
----------------------------------------------------------------
Parameters:
-----------
Outline Weight = Adjusts darkness of the outlines.

At lower internal resolutions, smaller values work better to
reduce the appearance of lines around individual areas of some
textures. At higher internal resolutions, setting both a higher 
outline weight value plus increased blur factors will work 
together to thicken the appearance of the lines.
----------------------------------------------------------------
*/

layout(push_constant) uniform Push
{
  vec4 MMJ_BlurPass_VSize;
  float OutlineWeight;
} params;

#pragma parameter OutlineWeight "Outline Weight" 1.0 0.0 10.0 0.1

#define MMJ_BlurPass_VSize params.MMJ_BlurPass_VSize
#define OutlineWeight params.OutlineWeight

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;


#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec4 TEX0;
layout(location = 2) out vec4 TEX1;
layout(location = 3) out vec4 TEX2;
layout(location = 4) out vec4 TEX3;
layout(location = 5) out vec4 TEX4;
layout(location = 6) out vec4 TEX5;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;

  TEX0 = vTexCoord.xyxy;
	
  vec4 offset;

  offset.xy = -(offset.zw = vec2(MMJ_BlurPass_VSize.z, 0.0));
  TEX1 = TEX0 + offset;
  TEX5 = TEX1 + offset;

  offset.xy = -(offset.zw = vec2(0.0, MMJ_BlurPass_VSize.w));
  TEX2 = TEX0 + offset;
  TEX3 = TEX1 + offset;
  TEX4 = TEX2 + offset;
}


#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec4 TEX0;
layout(location = 2) in vec4 TEX1;
layout(location = 3) in vec4 TEX2;
layout(location = 4) in vec4 TEX3;
layout(location = 5) in vec4 TEX4;
layout(location = 6) in vec4 TEX5;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D MMJ_BlurPass_V;

void main()
{
	vec3 c00 = texture(MMJ_BlurPass_V, TEX3.xy).rgb;
	vec3 c01 = texture(MMJ_BlurPass_V, TEX2.xy).rgb;
	vec3 c02 = texture(MMJ_BlurPass_V, TEX3.zy).rgb;
	vec3 c03 = texture(MMJ_BlurPass_V, TEX1.xy).rgb;
	vec3 c04 = texture(MMJ_BlurPass_V, TEX0.xy).rgb;
	vec3 c05 = texture(MMJ_BlurPass_V, TEX1.zw).rgb;
	vec3 c06 = texture(MMJ_BlurPass_V, TEX3.xw).rgb;
	vec3 c07 = texture(MMJ_BlurPass_V, TEX2.zw).rgb;
	vec3 c08 = texture(MMJ_BlurPass_V, TEX3.zw).rgb;	
  vec3 c09 = texture(MMJ_BlurPass_V, TEX4.xy).rgb;
  vec3 c10 = texture(MMJ_BlurPass_V, TEX4.zw).rgb;
  vec3 c11 = texture(MMJ_BlurPass_V, TEX5.xy).rgb;
  vec3 c12 = texture(MMJ_BlurPass_V, TEX5.zw).rgb;

  vec3 cNew = (c00 + c01 + c02 + c03 + c04 + c05 + c06 + c07 + c08 + c09 + c10 + c11 + c12) / 13.0;
  
	vec3 o = vec3(1.0), h = vec3(0.05), hz = h; 
	float k = 0.005, kz = 0.007, i = 0.0;

	vec3 cz = (cNew + h) / (dot(o, cNew) + k);

	hz = (cz - ((c00 + h) / (dot(o, c00) + k))); i  = kz / (dot(hz, hz) + kz);
	hz = (cz - ((c01 + h) / (dot(o, c01) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c02 + h) / (dot(o, c02) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c03 + h) / (dot(o, c03) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c05 + h) / (dot(o, c05) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c06 + h) / (dot(o, c06) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c07 + h) / (dot(o, c07) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c08 + h) / (dot(o, c08) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c09 + h) / (dot(o, c09) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c10 + h) / (dot(o, c10) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c11 + h) / (dot(o, c11) + k))); i += kz / (dot(hz, hz) + kz);
	hz = (cz - ((c12 + h) / (dot(o, c12) + k))); i += kz / (dot(hz, hz) + kz);

	i /= 12.0; 
	i = pow(i, OutlineWeight);

	FragColor.rgb = vec3(i);
}