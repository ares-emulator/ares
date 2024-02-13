#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float Displacement;
} params;

#pragma parameter Displacement "Displacement Factor" 20.0 0.0 60.0 1.0

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
layout(set = 0, binding = 3) uniform sampler2D displacementMap;

vec2 DisplacementLevel()
{
	return (params.OutputSize.zw*params.Displacement);//Displacement's value is measured in half of a pixel. Don't need to waste resources on multiplying by two.
}

vec2 CoordDisplacement(vec2 coord)
{
	return (coord-((texture(displacementMap, coord).xy-0.5)*DisplacementLevel()))*1.1-0.05;
//This reads the displacement texture and sets 0.5 to be 0, 1 to be 1, and 0 to be -1,
//and then it scales it to the displacement level previously defined.
//The multiplication and subtraction at the end is to scale it down so it doesn't exceed the window.
//I think it's slightly faster if I do it here and not at the end in the fragColor = line in void main().
}

void main()
{
   FragColor = vec4(texture(Source, CoordDisplacement(vTexCoord)).rgb, 1.0);
}
