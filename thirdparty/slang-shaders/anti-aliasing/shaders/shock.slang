#version 450

// based on Nvidia's GPU Gems article:
// https://developer.nvidia.com/gpugems/GPUGems2/gpugems2_chapter27.html
// slang shader by torridgristle and hunterk
// This shader needs several iterations for a noticeable deblur effect
// Nvidia's example uses 5 passes at 0.05 intensity, which seems good

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float shockMagnitude;
} params;

#pragma parameter shockMagnitude "Shock Magnitude" 0.05 0.0 4.0 0.01

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

#define Euler 2.718281828459045

const vec3 ones = vec3(1.0, 1.0, 1.0);

void main()
{
    vec3 inc = vec3(params.OutputSize.zw,0.0);
    
    vec3   curCol = texture(Source, vTexCoord).xyz;
    vec3    upCol = texture(Source, vTexCoord + inc.zy).xyz;
    vec3  downCol = texture(Source, vTexCoord - inc.zy).xyz;
    vec3 rightCol = texture(Source, vTexCoord + inc.xz).xyz;
    vec3  leftCol = texture(Source, vTexCoord - inc.xz).xyz;
    
    vec3 Convexity = 4.0 * curCol - rightCol - leftCol - upCol - downCol;

    vec2 Diffusion = vec2(dot((rightCol - leftCol) * Convexity, ones),
                             dot((upCol - downCol) * Convexity, ones));

    Diffusion *= params.shockMagnitude/(length(Diffusion) + 0.00001);

    curCol += (Diffusion.x > 0 ? Diffusion.x * rightCol :

             -Diffusion.x*leftCol) +

            (Diffusion.y > 0 ? Diffusion.y * upCol :

             -Diffusion.y * downCol);

    FragColor = vec4(curCol/(1 + dot(abs(Diffusion), ones.xy)),1.0);
}
