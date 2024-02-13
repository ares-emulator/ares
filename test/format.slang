#version 450

#pragma format R32_UINT

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
layout(location = 0) out uint FragColor;
layout(set = 0, binding = 1) uniform sampler2D Source;

void main()
{
   vec3 current = texture(Source, vTexCoord).rgb;
   FragColor = packUnorm4x8(0.3 * vec4(current, 1.0));
}
