#version 450

// Advanced Cartoon shader I and II
// by guest(r) (guest.r@gmail.com)
// license: GNU-GPL

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float border;
   float mute_colors;
} params;

#pragma parameter border "Toon Border Thickness" 1.0 0.0 4.0 0.5
#pragma parameter mute_colors "Toon Mute Colors" 0.0 0.0 1.0 1.0

vec2 OGL2Param = vec2(params.border, params.border);
const float bb = 0.5; // effects black border sensitivity

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord[9];

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord[0] = TexCoord;
   
   float x = (params.SourceSize.z / 8.0)*OGL2Param.x;
   float y = (params.SourceSize.w / 8.0)*OGL2Param.y;
   vec2 dg1 = vec2( x,y);
   vec2 dg2 = vec2(-x,y);
   vec2 dx  = vec2(x,0.0);
   vec2 dy  = vec2(0.0,y);
   vTexCoord[1].xy = vTexCoord[0].xy - dy;
   vTexCoord[2].xy = vTexCoord[0].xy + dy;
   vTexCoord[3].xy = vTexCoord[0].xy - dx;
   vTexCoord[4].xy = vTexCoord[0].xy + dx;
   vTexCoord[5].xy = vTexCoord[0].xy - dg1;
   vTexCoord[6].xy = vTexCoord[0].xy + dg1;
   vTexCoord[7].xy = vTexCoord[0].xy - dg2;
   vTexCoord[8].xy = vTexCoord[0].xy + dg2;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord[9];
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec3 c00 = texture(Source, vTexCoord[5].xy).xyz; 
	vec3 c10 = texture(Source, vTexCoord[1].xy).xyz; 
	vec3 c20 = texture(Source, vTexCoord[8].xy).xyz; 
	vec3 c01 = texture(Source, vTexCoord[3].xy).xyz; 
	vec3 c11 = texture(Source, vTexCoord[0].xy).xyz; 
	vec3 c21 = texture(Source, vTexCoord[4].xy).xyz; 
	vec3 c02 = texture(Source, vTexCoord[7].xy).xyz; 
	vec3 c12 = texture(Source, vTexCoord[2].xy).xyz; 
	vec3 c22 = texture(Source, vTexCoord[6].xy).xyz; 
	vec3 dt = vec3(1.0,1.0,1.0); 

	float d1=dot(abs(c00-c22),dt);
	float d2=dot(abs(c20-c02),dt);
	float hl=dot(abs(c01-c21),dt);
	float vl=dot(abs(c10-c12),dt);
	float d = bb*(d1+d2+hl+vl)/(dot(c11,dt)+0.15);
   
   float lc, f;
   vec3 frct;
   
   if(params.mute_colors < 0.5)
   {
   	lc = 4.0*length(c11);
      f = fract(lc); f*=f;
      lc = 0.25*(floor(lc) + f*f)+0.05;
      c11 = 4.0*normalize(c11); 
      vec3 frct = fract(c11); frct*=frct;
      c11 = floor(c11)+ 0.05*dt + frct*frct;
      FragColor.xyz = 0.25*lc*(1.1-d*sqrt(d))*c11;
      return;
   }
   else
   {
      lc = 5.0*length(c11); 
      lc = 0.2*(floor(lc) + pow(fract(lc),4.0));
      c11 = 4.0*normalize(c11); 
      frct = fract(c11); frct*=frct;
      c11 = floor(c11) + frct*frct;
      c11 = 0.25*(c11)*lc; lc*=0.577;
      c11 = mix(c11,lc*dt,lc);
      FragColor.xyz = (1.1-pow(d,1.5))*c11;
   }
}