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
    vec3 OriginPass  = texture(Source, vTexCoord).xyz;//No blending

    //Blend 4 pixels together by sampling between them with linear interpolation
    vec2 SquareBlend = (vTexCoord * params.SourceSize.xy - 0.5) * params.SourceSize.zw;
    //Blend 2 horizontal pixels together the same way as before
    vec2 HorizoBlend = (vTexCoord * params.SourceSize.xy - vec2(0.5,0.0)) * params.SourceSize.zw;
    vec3 SquarePass  = texture(Source, SquareBlend).xyz;
    vec3 HorizoPass  = texture(Source, HorizoBlend).xyz;
    
    //Edge Detection for SquareBlend
    vec3 SquareEdge = texture(Source, (SquareBlend * params.SourceSize.xy + vec2( 0.0, 1.0)) * params.SourceSize.zw).xyz + 
                      texture(Source, (SquareBlend * params.SourceSize.xy + vec2( 1.0, 0.0)) * params.SourceSize.zw).xyz +
                      texture(Source, (SquareBlend * params.SourceSize.xy + vec2( 1.0, 1.0)) * params.SourceSize.zw).xyz;
    SquareEdge      = abs((SquareEdge / 3.0) - SquarePass);
    //Try to adjust white / black range so that edges are black and non-edges are white
    float SquareEdgeMask = 1.0-pow(1.0-pow(1.0-max(SquareEdge.x,max(SquareEdge.y,SquareEdge.z)),30.0),2.0);
    
    //Edge Detection for HorizoBlend
    vec3 HorizoEdge = texture(Source, (HorizoBlend * params.SourceSize.xy + vec2( 0.0, 1.0)) * params.SourceSize.zw).xyz + 
                      texture(Source, (HorizoBlend * params.SourceSize.xy + vec2( 1.0, 0.0)) * params.SourceSize.zw).xyz +
                      texture(Source, (HorizoBlend * params.SourceSize.xy + vec2( 1.0, 1.0)) * params.SourceSize.zw).xyz;
    HorizoEdge      = abs((HorizoEdge / 3.0) - HorizoPass);
    //Try to adjust white / black range so that edges are black and non-edges are white
    float HorizoEdgeMask = 1.0-pow(1.0-pow(1.0-max(HorizoEdge.x,max(HorizoEdge.y,HorizoEdge.z)),10.0),2.0);
    
    //If SquarePass has a detected edge, use HorizoPass
    vec3 Result = mix(HorizoPass,SquarePass,SquareEdgeMask);
    //If HorizoPass has a detected edge, use OriginPass
    Result      = mix(OriginPass,Result,HorizoEdgeMask);

    //It's complete
    FragColor = vec4(Result,1.0);
}