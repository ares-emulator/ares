#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float mixfactor;
	float threshold;
} params;

#pragma parameter mixfactor "Motionblur Fadeout" 0.5 0.0 1.0 0.01
#pragma parameter threshold "Brightness Threshold" 0.9 0.0 1.0 0.01

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

float key(float avg)
{
	float guess = 1.5 - (1.5 / (avg * 0.1 + 1.0));
	return max(0.0, guess) + 0.1;
}

mat3 yiq2rgb_mat = mat3(
   1.0, 1.0, 1.0,
   0.956, -0.2720, -1.1060,
   0.6210, -0.6474, 1.7046
);

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
layout(set = 0, binding = 3) uniform sampler2D Trails0Feedback;

void main()
{
	vec3 frame   = texture(Source, vTexCoord).rgb;
	float luma   = (frame.rrr * yiq_mat).r;
    float trails = clamp(luma - params.threshold, 0.0, 1.0);
    vec4 fdback  = pow(texture(Trails0Feedback, vTexCoord), vec4(2.2));
    vec4 mixed   = clamp((1.0 - params.mixfactor) * vec4(trails) - params.mixfactor * fdback, 0.0, 1.0) + params.mixfactor * fdback;
//	vec4 current = pow(texture(Source, vTexCoord), vec4(2.2));
    
    FragColor = pow(mixed, vec4(1.0 / 2.2));
}
