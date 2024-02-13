#version 450

// GBC Pokemon Modernizer
// by torridgristle
// license: Public Domain

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

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

precision lowp float;
 
const mat3 GBCMatrix = mat3( 0.924, 0.021, 0.013, 0.048, 0.787, 0.249, 0.104, 0.09, 0.733 );

void main()
{
    vec3 Picture = texture(Source, vTexCoord).xyz;
   
    Picture *= Picture;
    Picture *= GBCMatrix;
    Picture = sqrt(Picture);
 
    FragColor = vec4(Picture,1.0);
}
