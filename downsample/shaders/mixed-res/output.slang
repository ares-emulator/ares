#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float B_VIEW;
	float B_TRESH;
} params;

#pragma parameter B_VIEW "SHOW HIRES DETECTION" 0.0 0.0 1.0 1.0
#pragma parameter B_TRESH "BLUR THRESHOLD" 0.2 0.0 1.0 0.01

#define B_VIEW params.B_VIEW
#define B_TRESH params.B_TRESH

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

const vec3 coder = vec3(0.299, 0.587, 0.114);

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
layout(set = 0, binding = 3) uniform sampler2D HiresSource;

void main()
{
    vec4 c_a  = texture(HiresSource, vTexCoord);

    float tag = c_a.a;

    vec3 c_hires  = c_a.rgb;
    vec3 c_lowres = texture(Source, vTexCoord).rgb;

    vec3 color = mix(c_lowres, mix(c_hires, vec3(0.0, 0.0, dot(c_hires, coder)), B_VIEW), smoothstep(0.0, B_TRESH, tag));

    FragColor = vec4(color, 1.0);
}
