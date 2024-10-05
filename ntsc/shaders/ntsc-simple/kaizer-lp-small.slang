#version 450

layout(push_constant) uniform Push
{
  float something;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;

} global;

#define OutputSize global.OutputSize
#define SourceSize global.SourceSize
#define OriginalSize global.OriginalSize
#define FrameCount global.FrameCount

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;


void main()
{
    vec2 ps = vec2(SourceSize.z,0.0);
    vec3 res = texture(Source,vTexCoord).rgb*0.62;
     res += texture(Source,vTexCoord+ps).rgb*0.39;
     res += texture(Source,vTexCoord+3.0*ps).rgb*-0.1;
     res += texture(Source,vTexCoord+5.0*ps).rgb*0.04;

    FragColor.rgb = res;
}
