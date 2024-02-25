#version 450

// Super Sleuth Gamma Ramp
// based on Overload's ramp as implemented in bsnes v073
// ported by hunterk
// license: GPLv2

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	float mixer;
} params;

#pragma parameter mixer "Gamma Boost (%)" 150.0 100.0 200.0 1.0

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

// Overload's gamma ramp from Super Sleuth
// Apparently not really based on anything but it looks nice
const uint gammaRamp[32] = {
      0x00, 0x01, 0x03, 0x06, 0x0a, 0x0f, 0x15, 0x1c,
      0x24, 0x2d, 0x37, 0x42, 0x4e, 0x5b, 0x69, 0x78,
      0x88, 0x90, 0x98, 0xa0, 0xa8, 0xb0, 0xb8, 0xc0,
      0xc8, 0xd0, 0xd8, 0xe0, 0xe8, 0xf0, 0xf8, 0xff,
    };

#define conv(f) ((f >= 1.0) ? 255 : (f <= 0.0 ? 0 : int(floor(f * 256.0))))

void main()
{
	vec4 img = texture(Source, vTexCoord);

	// convert standard vec4 to uint color values
	uvec4 int_img = uvec4(conv(img.r), conv(img.g), conv(img.b), conv(img.a));

	uint r = (int_img.r) & 0xff;
	uint g = (int_img.g) & 0xff;
	uint b = (int_img.b) & 0xff;

	// apply the ramp
	uint R = gammaRamp[r >> 3];
	uint G = gammaRamp[g >> 3];
	uint B = gammaRamp[b >> 3];

	uvec3 output_i = uvec3(R, G, B);
	vec3  output_f = vec3(output_i) * vec3(1./255.);

	// mix between corrected and uncorrected output
	FragColor.rgb = mix(img.rgb, output_f, (params.mixer / 100.0) - 1.0);
}
