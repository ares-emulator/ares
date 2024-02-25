#version 450

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 SourceSize;
   vec4 OriginalHistorySize1;
   vec4 OriginalHistorySize3;
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
layout(set = 0, binding = 2) uniform sampler2D OriginalHistory1;
layout(set = 0, binding = 3) uniform sampler2D OriginalHistory3;

void main()
{
   vec2 tex = floor(global.OriginalHistorySize1.xy * vTexCoord);
   tex = (tex + 0.5) * global.OriginalHistorySize1.zw;

   vec3 current = texture(Source, tex).rgb;
   vec3 prev = texture(OriginalHistory1, tex).rgb;
   vec3 prev2 = texture(OriginalHistory3, tex).rgb;
   FragColor = vec4(abs(current - prev + prev2), 1.0);
}
