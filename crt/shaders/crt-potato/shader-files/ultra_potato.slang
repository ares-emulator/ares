#version 450

// crt-potato mod by DariusG

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

// Largest integer scale of input video that will fit in the current output (y axis would typically be limiting on widescreens)
#define video_scale         floor(params.OutputSize.y * params.SourceSize.w)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 dot_size;
layout(location = 2) out vec2 one_texel;

void main()
{
	gl_Position = global.MVP * Position;
	vTexCoord = TexCoord;
	dot_size = params.SourceSize.zw;
	one_texel = 1.0 / (params.SourceSize.xy * video_scale);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 dot_size;
layout(location = 2) in vec2 one_texel;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D MASK;


#define maskSize vec2(2., floor(params.OutputSize.y / params.SourceSize.y + 0.000001))

void main()
{
   vec4 res =texture(Source, vTexCoord);
   float lum = max((res.r,res.g),res.b); 
   lum = mix(1.0,0.3,lum);
   vec4 mask = texture(MASK, fract((vTexCoord.xy * params.OutputSize.xy) / maskSize));
   mask = pow(mask,vec4(lum));
   res *= mix(1.0,1.2,lum);
   vec4 color = mask * res; 
   FragColor = color;
}