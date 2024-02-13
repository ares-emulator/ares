#version 450
// 450 or 310 es are recommended

layout(set = 0, binding = 0, std140) uniform UBO
{
   mat4 MVP;
};

#pragma name StockShader
#pragma format R8G8B8A8_UNORM

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
void main()
{
   gl_Position = MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D Source;
void main()
{
   FragColor = texture(Source, vTexCoord);
}