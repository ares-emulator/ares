#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float trail_bright;
} params;

#pragma parameter trail_bright "Phos. Trail Brightness" 0.25 0.0 1.0 0.01

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

mat3 yiq_mat = mat3(
      0.2989, 0.5959, 0.2115,
      0.5870, -0.2744, -0.5229,
      0.1140, -0.3216, 0.3114
);

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
layout(set = 0, binding = 3) uniform sampler2D glow_trails_refpass;

void main()
{
	vec4 trails = texture(Source, vTexCoord).rgba;
	vec4 current = pow(texture(glow_trails_refpass, vTexCoord).rgba, vec4(2.2));
   FragColor = vec4(pow(current + vec4(clamp(trails.r - current.r, 0.0, 1.0) * params.trail_bright), vec4(1.0 / 2.2)));
}
