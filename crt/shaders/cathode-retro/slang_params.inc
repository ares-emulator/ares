layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
	vec4 FinalViewportSize;
	float warpX, warpY, corner, noise_seed;
} params;

#pragma parameter div0 "--------Screen Settings--------" 0.0 0.0 0.0 0.0
#pragma parameter scan_intens "Scanline Intensity" 0.4 0.0 1.0 0.01
#pragma parameter cat_mask_picker "Mask (0=none, 1=aperture, 2=slot, 3=shadow)" 1.0 0.0 3.0 1.0
#pragma parameter mask_intens "Mask Strength" 0.4 0.0 1.0 0.01
#pragma parameter mask_scale "Mask Scale (2 or 3 for 4K)" 1.0 1.0 200.0 1.0
#pragma parameter mask_depth "Mask Background Darkness" 0.3 0.0 1.0 0.01
#pragma parameter warpX "Barrel Distortion X" 0.2 0.0 1.0 0.01
#pragma parameter warpY "Barrel Distortion Y" 0.1 0.0 1.0 0.01
#pragma parameter anim_noise "Animate Anti-Moire Noise" 0.0 0.0 1.0 1.0
#pragma parameter corner "Rounded Corner Size" 0.03 0.0 1.0 0.01
#pragma parameter persistence "Phosphor Persistence" 0.25 0.0 1.0 0.01
#pragma parameter diffusion "Diffusion Strength" 0.5 0.0 1.0 0.01
#pragma parameter div1 "---------TV Knob Settings---------" 0.0 0.0 0.0 0.0
#pragma parameter cat_sat "Saturation" 1.0 0.0 2.0 0.01
#pragma parameter cat_bright "Brightness" 1.0 0.0 2.0 0.01
#pragma parameter cat_white_lvl "White Level" 1.0 0.0 2.0 0.01
#pragma parameter cat_black_lvl "Black Level" 0.0 0.0 2.0 0.01
#pragma parameter tint "Tint Knob Adjustment" 0.0 -1.0 1.0 0.01
#pragma parameter blurStrength "Sharpness" -0.15 -1.0 1.0 0.01
#pragma parameter div2 "---------Signal Parameters---------" 0.0 0.0 0.0 0.0
#pragma parameter composite "Blend Chrome/Luma (aka Composite)" 1.0 0.0 1.0 1.0
#pragma parameter sig_pad "Signal Padding at Edges" 0.0 0.0 10.0 1.0
#pragma parameter minlum "Minimum Luminance" 1.0 0.0 1.0 0.01
#pragma parameter colorpower "Color Power" 1.0 0.0 2.0 0.01
#pragma parameter noise_seed "Noise Seed" 247.0 179.0 313.0 1.0
#pragma parameter cb_samples "Samples Per Color Burst Cyle" 2.0 1.0 100.0 1.0
#pragma parameter cb_first_start "Color Burst Phase First Scanline" 0.0 0.0 100.0 1.0
#pragma parameter cb_last_start "CB Phase Prev Frame First Scanline" 1.0 0.0 100.0 1.0
#pragma parameter cb_phase_inc "Color Burst Phase Increment" 1.66666 0.0 3.0 0.01
#pragma parameter stepSize "Texels Between Each Sample" 1.0 1.0 100.0 1.0
#pragma parameter div3 "-------Artifact Settings-------" 0.0 0.0 0.0 0.0
#pragma parameter horz_track_scale "Horizontal Tracking Instability Scale" 1.0 0.0 3.0 0.05
#pragma parameter ghost_vis "Ghost Visibility" 0.15 0.0 1.0 0.01
#pragma parameter ghost_dist "Ghost Delay Cycles" 1.0 0.0 100.0 1.0
#pragma parameter ghost_spread "Ghost Spread Cycles" 1.0 0.0 100.0 1.0
#pragma parameter noise_strength "Artifact Noise Strength" 0.15 0.0 1.0 0.01
#pragma parameter temp_artifact_blend "Temporal Artifact Blending (Toggle)" 0.0 0.0 1.0 1.0

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define ddx dFdx
#define ddy dFdy
#define frac fract
#define lerp mix

    float saturate(float a)
    {
      return clamp(a, 0.0, 1.0);
    }

    vec2 saturate(vec2 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    vec3 saturate(vec3 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    vec4 saturate(vec4 a)
    {
      return clamp(a, 0.0, 1.0);
    }

    void sincos(float angle, out float s, out float c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(vec2 angle, out vec2 s, out vec2 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(vec3 angle, out vec3 s, out vec3 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

    void sincos(vec4 angle, out vec4 s, out vec4 c)
    {
      s = sin(angle);
      c = cos(angle);
    }

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 OriginalSize;
	float cb_first_start, cb_last_start, cb_phase_inc, cb_samples, horz_track_scale,
	   minlum, colorpower, composite, sig_pad, ghost_vis, ghost_dist, ghost_spread,
	   noise_strength, tint, cat_sat, cat_bright, cat_white_lvl, cat_black_lvl,
	   temp_artifact_blend, blurStrength, stepSize, persistence, scan_intens,
	   diffusion, mask_intens, mask_depth, cat_mask_picker, mask_scale, anim_noise,
	   div0, div1, div2, div3;
} global;

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;//vec4(Position.x * 2.0 - 1.0, Position.y * 2.0 - 1.0, 0.0, 1.0);//
   vTexCoord = vec2(TexCoord.x, TexCoord.y) * 1.0001;//vec2(Position.x, 1.0 - Position.y);//
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
