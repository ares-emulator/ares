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

void main()
{
    vec3 Sample1 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 0.0, 0.0)) * params.SourceSize.zw).xyz;
    vec3 Sample2 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 1.0, 1.0)) * params.SourceSize.zw).xyz;
    vec3 Sample3 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 1.0, 0.0)) * params.SourceSize.zw).xyz;
    vec3 Sample4 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 0.0, 1.0)) * params.SourceSize.zw).xyz;
    
    vec3 Average = (Sample1+Sample2+Sample3+Sample4)*0.25;
    float Brightness = 1.0-pow(1.0-max(max(Average.x,Average.y),Average.z),2.0);
    
    vec3 Sample2Diff = abs(Sample2-Sample1);
    float Sample2DiffMax = max(max(Sample2Diff.x,Sample2Diff.y),Sample2Diff.z);
    vec3 Sample3Diff = abs(Sample3-Sample1);
    float Sample3DiffMax = max(max(Sample3Diff.x,Sample3Diff.y),Sample3Diff.z);
    vec3 Sample4Diff = abs(Sample4-Sample1);
    float Sample4DiffMax = max(max(Sample4Diff.x,Sample4Diff.y),Sample4Diff.z);
    
    float DiffLimit = (Brightness * 4.0 + 2.0) / 32.0;
    
    Sample2 = mix(Sample1,Sample2,vec3(Sample2DiffMax < DiffLimit));
    Sample3 = mix(Sample1,Sample3,vec3(Sample3DiffMax < DiffLimit));
    Sample4 = mix(Sample1,Sample4,vec3(Sample4DiffMax < DiffLimit));
    
    vec3 Mixing = Sample1 + Sample2 + Sample3 + Sample4;
    Mixing *= 0.25;
    
    vec3 Result = vec3(Mixing);
    
    FragColor = vec4(Result,1.0);
}