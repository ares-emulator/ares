#version 450

/*
   2023
   Night mode (!) by DariusG

*/


layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	float WP;
	float strength;

} params;
#pragma parameter WP "Color Temperature" 0.1 0.0 0.25 0.01
#pragma parameter strength "Night mode Strength" 0.8 0.5 1.0 0.05

#define WP params.WP
#define strength params.strength


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
	vTexCoord = TexCoord;

}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;


void main()
{	
	vec3 color = texture(Source, vTexCoord).rgb;
	
	color *= vec3(strength);
	color  = vec3(color.r+WP, color.g, color.b-WP);


	FragColor = vec4(color,1.0);
}
