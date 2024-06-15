#version 450

layout(push_constant) uniform Push
{
	float debug_toggle;
} params;

#pragma parameter debug_toggle "Debug (timer, cadence, bfi, luma)" 0.0 0.0 4.0 1.0
int debug = int(params.debug_toggle);

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	vec4 FinalViewportSize;
	uint FrameCount;
	uint FrameDirection;
	uint CurrentSubFrame;
	uint TotalSubFrames;
	uint Rotation;
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
layout(set = 0, binding = 2) uniform sampler2D bfiRefPass;
layout(set = 0, binding = 3) uniform sampler2D calcPass;

void main()
{
	vec3 img = texture(bfiRefPass, vTexCoord).rgb;
	vec4 calc = (global.TotalSubFrames < 2) ? vec4(1.0) : texture(calcPass, vec2(0.5,0.5));
	FragColor = vec4(img * calc.z, 1.0);
	if(debug == 1) FragColor.rgb = calc.xxx / 3600.0;
	if(debug == 2) FragColor.rgb = calc.yyy;
	if(debug == 3) FragColor.rgb = calc.zzz;
	if(debug == 4) FragColor.rgb = calc.www;
}