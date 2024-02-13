#version 450

/*
    by Rere
	license: public domain
*/

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float thickness;
	float glow;
	float sGamma;
	float tGamma;
	float highlights;
	float boost;
} params;

#pragma parameter thickness  "Scanline thickness"        0.50 0.00 1.00 0.01
#pragma parameter glow       "Scanline glow"             0.75 0.00 1.00 0.01
#pragma parameter highlights "Scanline highlights"       0.75 0.00 1.00 0.01
#pragma parameter boost      "Luminance boost"           0.25 0.00 1.00 0.01
#pragma parameter sGamma     "Source gamma"              2.40 1.00 3.00 0.01
#pragma parameter tGamma     "Target gamma"              2.20 1.00 3.00 0.01

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define pi 3.141592654
#define luminance(c) (0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b)

vec3 gammaFn(vec3 c, float gamma) {
  return vec3(pow(c.x, gamma), pow(c.y, gamma), pow(c.z, gamma));
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out float thickness;
layout(location = 2) out float glow;
layout(location = 3) out float highlights;
layout(location = 4) out float boost;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
   thickness = 0.5 + mix(0.0, 2.0, params.thickness);
   glow = mix(-0.5, 0.5, params.glow);
   highlights = mix(0.0, 1.0, params.highlights);
   boost = mix(0.0, 5.0, params.boost);
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in float thickness;
layout(location = 2) in float glow;
layout(location = 3) in float highlights;
layout(location = 4) in float boost;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
	vec2 uv = vTexCoord.xy;
    vec3 col = texture(Source, uv).rgb;
    col = gammaFn(col, params.sGamma);
    float L = luminance(col);
    float y = fract(uv.y * params.SourceSize.y * 1.0);
	y = pow(sin(y * pi), thickness);
	y = (y + glow) / (1.0 + glow);
	float g = 1.0 + L * (1.0 - L) * boost;
	col = mix(col, col * g * y, 1.0 - L * highlights);
    FragColor = vec4(gammaFn(col, 1.0 / params.tGamma), 1.0);
}
