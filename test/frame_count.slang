#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   uint FrameCount;
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
layout(set = 0, binding = 1) uniform sampler2D Source;

void main()
{
   vec3 current = texture(Source, vTexCoord).rgb;
   FragColor = sqrt(0.5 + 0.4 * sin(float(global.FrameCount) * 6.28 * 0.01)) *
      vec4(current, 1.0);
}
