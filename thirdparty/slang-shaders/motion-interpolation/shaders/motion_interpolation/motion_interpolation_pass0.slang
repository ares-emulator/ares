#version 450

// Video Motion Interpolation
// based on Drudgerist's shadertoy:
// https://www.shadertoy.com/view/MtVfRz
// which is, in turn, based on bodhid's V+
// https://github.com/bodhid/Vplus
// This pass generates motion vectors based on the current and previous frames

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
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory1;

#define iChannel0 OriginalHistory1
#define iChannel1 Source
#pragma name Pass0

float PixV(sampler2D Target, vec2 Shift, vec2 iuv, vec2 texelSize)
{
    vec4 c = texture(Target,iuv);
    c += texture(Target, iuv + Shift+ (vec2(1, 0) * texelSize));
    c += texture(Target, iuv + Shift + (vec2(0, 1) * texelSize));
    c += texture(Target, iuv + Shift + (vec2(-1, 0) * texelSize));
    c += texture(Target, iuv + Shift + (vec2(0, -1) * texelSize));
    return c.r + c.g + c.b;
}

void main()
{
    const float magic = 6.28;
    vec2 uv = vTexCoord.xy;
    vec2 texelSize = params.SourceSize.zw;
    float CheckValue, FirstValue, PrevValue, LastMatch;
	vec2 FinalUvShift,CheckUvShift;

    PrevValue = PixV(iChannel0, vec2(0,0), uv, texelSize);
    LastMatch = abs(PrevValue - PixV(iChannel1, vec2(0,0), uv, texelSize));

    for(int Circle = 1; Circle < 4; ++Circle)
    {
        for(int j = 0; j < 4 * Circle; ++j)
        {
            float circleF = float(Circle);
            float jF = float(j);

            FirstValue = magic / (4.0 * circleF);
			CheckUvShift = vec2(sin(FirstValue * jF), cos(FirstValue * jF));
            float currentValue = PixV(iChannel1, CheckUvShift * texelSize * circleF, uv, texelSize);
            if ((CheckValue = abs(PrevValue - currentValue)) < LastMatch)
            {
                LastMatch = CheckValue;
                FinalUvShift = CheckUvShift;
            }
        }
    }
    FragColor = vec4(FinalUvShift.xy, 0, 1);
}