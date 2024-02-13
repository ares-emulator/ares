#version 450

// PS1 Undither AntiBayer
// by torridgristle

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

// PlayStation dithering pattern. The offset is selected based on the
// pixel position in VRAM, by blocks of 4x4 pixels. The value is added
// to the 8bit color components before they're truncated to 5 bits.
//"const int dither_pattern[16] ="
//"  int[16](-4,  0, -3,  1,"
//"           2, -2,  3, -1,"
//"          -3,  1, -4,  0,"
//"           3, -1,  2, -2);" "\n"

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
    vec3 OriginPass  = texture(Source, vTexCoord).xyz;

    float PatternLUT[16] = float[](-4.0,  0.0, -3.0,  1.0,
                                    2.0, -2.0,  3.0, -1.0,
                                   -3.0,  1.0, -4.0,  0.0,
                                    3.0, -1.0,  2.0, -2.0);
    
    float PatternX = ceil(((mod(vTexCoord.x * params.SourceSize.x,4.00001)-0.5)/3.0)*3.0);
    float PatternY = ceil(((mod(vTexCoord.y * params.SourceSize.y,4.00001)-0.5)/3.0)*3.0);
    float Pattern = PatternLUT[int(PatternX+(PatternY*4))];
    
    vec3 Result = OriginPass;
    //Result = vec3(Pattern)/8.0 + 0.5;
    Result = (Result * 255.0 - Pattern - round(Pattern*0.25)*2.0) / 255.0;

    FragColor = vec4(Result,1.0);
}