#version 450

layout(location = 0) out vec4 color;
layout(set = 0, binding = 1) uniform texture2D tex;
layout(set = 1, binding = 1) uniform sampler tex_sampler;

void main() {
    color = texture(sampler2D(tex, tex_sampler), vec2(0.0));
}