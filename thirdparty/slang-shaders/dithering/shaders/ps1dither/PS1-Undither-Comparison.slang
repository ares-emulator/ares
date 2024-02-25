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
    vec3 OriginPass  = texture(Source, vTexCoord).xyz;

    //Get blended textures by offsetting between 4 pixels.
    vec3 SquarePass1 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 0.5, 0.5)) * params.SourceSize.zw).xyz;
    vec3 SquarePass2 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2(-0.5,-0.5)) * params.SourceSize.zw).xyz;
    vec3 SquarePass3 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2(-0.5, 0.5)) * params.SourceSize.zw).xyz;
    vec3 SquarePass4 = texture(Source, (vTexCoord * params.SourceSize.xy + vec2( 0.5,-0.5)) * params.SourceSize.zw).xyz;
    
    //Which SquarePass is closest to the OriginPass?
    vec3 SquarePass1Diff = abs(SquarePass1 - OriginPass);
    vec3 SquarePass2Diff = abs(SquarePass2 - OriginPass);
    vec3 SquarePass3Diff = abs(SquarePass3 - OriginPass);
    vec3 SquarePass4Diff = abs(SquarePass4 - OriginPass);
    float SquarePass1DiffAvg = (SquarePass1Diff.x + SquarePass1Diff.y + SquarePass1Diff.z) / 3.0;
    float SquarePass2DiffAvg = (SquarePass2Diff.x + SquarePass2Diff.y + SquarePass2Diff.z) / 3.0;
    float SquarePass3DiffAvg = (SquarePass3Diff.x + SquarePass3Diff.y + SquarePass3Diff.z) / 3.0;
    float SquarePass4DiffAvg = (SquarePass4Diff.x + SquarePass4Diff.y + SquarePass4Diff.z) / 3.0;
    float SquarePassDiffAvgMin = min(SquarePass1DiffAvg,min(SquarePass2DiffAvg,min(SquarePass3DiffAvg,SquarePass4DiffAvg)));
    
    vec3 SquarePass =
    float(SquarePassDiffAvgMin == SquarePass1DiffAvg) * float(SquarePass1DiffAvg != SquarePass2DiffAvg) * float(SquarePass1DiffAvg != SquarePass3DiffAvg) * float(SquarePass1DiffAvg != SquarePass4DiffAvg) * SquarePass1 +
    float(SquarePassDiffAvgMin == SquarePass2DiffAvg) * float(SquarePass2DiffAvg != SquarePass3DiffAvg) * float(SquarePass2DiffAvg != SquarePass4DiffAvg) * SquarePass2 +
    float(SquarePassDiffAvgMin == SquarePass3DiffAvg) * float(SquarePass3DiffAvg != SquarePass4DiffAvg) * SquarePass3 +
    float(SquarePassDiffAvgMin == SquarePass4DiffAvg) * SquarePass4;
    
    
    //How different is the blended texture from the original texture?
    //The difference between the least different and most different SquarePasses returns edges.
    vec3 SquareCompareMin = min(SquarePass1Diff,min(SquarePass2Diff,min(SquarePass3Diff,SquarePass4Diff)));
    vec3 SquareCompareMax = max(SquarePass1Diff,max(SquarePass2Diff,max(SquarePass3Diff,SquarePass4Diff)));
    vec3 SquareCompareBoth = abs(SquareCompareMin - SquareCompareMax);
    
    float SquareCompare = max(SquareCompareBoth.x,max(SquareCompareBoth.y,SquareCompareBoth.z));
    
    //Turn a range of the comparison result into a mask. 2.5 just happens to work well, it's not set in stone or anything.
    float SquareMask = 1.0-pow(1.0-min(max(SquareCompare*2.5,0.0),1.0),2.0);
    
    //Blend it all together
    vec3 Result = mix(SquarePass,OriginPass,SquareMask);
    
    //Result = vec3(SquareMask);
    
    FragColor = vec4(Result,1.0);
}