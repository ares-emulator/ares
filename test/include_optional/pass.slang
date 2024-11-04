#version 450

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

// green
#define COLOR_OUT (vec3(0,1,0))	
// Optionally turns into purple if includes/myinclude.inc exists.
#pragma include_optional "includes/myinclude.inc"
// This one should fail
#pragma include_optional "includes/myinclude.doesntexist.inc"

void main()
{	
	FragColor = vec4(COLOR_OUT, 1.0);
}