#version 450

/*
   April 2023
   Adjustable Bilinear by DariusG

*/
#pragma parameter sharp1 "Horizontal Sharpness" 1.0 1.0 10.0 0.1
#pragma parameter sharp2 "Vertical Sharpness" 1.0 1.0 10.0 0.1


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	float sharp1;
	float sharp2;

} params;

#define sharp1 params.sharp1
#define sharp2 params.sharp2
#define SourceSize params.SourceSize

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 FinalViewportSize;
	vec4 OutputSize;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord*1.0001;

}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;


void main()
{

vec2 OneTexel = SourceSize.zw;

vec2 OGL2Pos = vTexCoord * SourceSize.xy;
vec2 pC4 = floor(OGL2Pos) * OneTexel + 0.5*OneTexel;

vec2 coord4 = vec2 (0.0);
vec2 coord2 = vec2 (OneTexel.x, 0.0 );
vec2 coord1 = vec2 (0.0, OneTexel.y);
vec2 coord3 = vec2 (OneTexel.x, OneTexel.y);


vec4 s4 = vec4(texture(Source, pC4 + coord4));
vec4 s2 = vec4(texture(Source, pC4 + coord2));
vec4 s1 = vec4(texture(Source, pC4 + coord1));
vec4 s3 = vec4(texture(Source, pC4 + coord3));

float fu = pow(fract(OGL2Pos.x),sharp1);
float fv = pow(fract(OGL2Pos.y),sharp2);

vec4 tmp1 = mix(s4, s2, fu);
vec4 tmp2 = mix(s1, s3, fu);

vec4 t0 = mix(tmp1, tmp2, fv);

	FragColor = t0;
}
