#version 450

// Accessibility Mods
// based on work by Karen Stevens for Electronic Arts
// adapted for slang by hunterk

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
   float Protanopia, Deuteranopia, Tritanopia, D_factor, D_contrast, D_brightness;
} params;

#pragma parameter D_factor "Accessibility Adjustment Strength" 0.5 0.0 1.0 0.05
#pragma parameter Protanopia "Protanopia Correction Toggle" 0.0 0.0 1.0 1.0
#pragma parameter Deuteranopia "Deuteranopia Correction Toggle" 0.0 0.0 1.0 1.0
#pragma parameter Tritanopia "Tritanopia Correction Toggle" 0.0 0.0 1.0 1.0
#pragma parameter D_contrast "Contrast Adjustment" 0.0 0.0 1.0 0.05
#pragma parameter D_brightness "Brightness Adjustment" 0.0 -1.0 1.0 0.05

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
} global;

#define mul(a,b) b*a

// RGB to LMS and back again
// https://ssodelta.wordpress.com/tag/rgb-to-lms/
mat3 RGBtoLMS = mat3( 17.8824,  43.5161,  4.1193,
                       3.4557,  27.1554,  3.8671,
                       0.02996,  0.18431, 1.4670);
                       
mat3 LMStoRGB = mat3( 0.0809, -0.1305,  0.1167,
                     -0.0102,  0.0540, -0.1136,
                     -0.0003, -0.0041,  0.6935);

// This function draws heavily from Karen Stevens' Daltonization work, found here:
// https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Stevens_Karen_GameAccessibilityPracticalFixes.pdf
vec3 Daltonize(vec3 input_col){
   vec3 Dalton_col = input_col;
   vec3 LMS_col = mul(input_col, RGBtoLMS);
   Dalton_col.r = mix(LMS_col.r, (2.02344 * LMS_col.g) - (2.5281 * LMS_col.b), params.Protanopia);
   Dalton_col.g = mix(LMS_col.g, (0.494207 * LMS_col.r) + (1.24827 * LMS_col.b), params.Deuteranopia);
   Dalton_col.b = mix(LMS_col.b, (-0.395913 * LMS_col.r) + (0.801109 * LMS_col.g), params.Tritanopia);
   
   vec3 diff_col = input_col - mul(Dalton_col, LMStoRGB);

   vec3 shift_col;
   shift_col.r = 0.0;
   shift_col.g = diff_col.g + diff_col.r;
   shift_col.b = diff_col.b + diff_col.r;
   
   vec3 out_col = input_col + shift_col.rgb;
   out_col = clamp(out_col, 0.0, 1.0);
   
   return out_col;
}

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

void main()
{
   vec3 frame = texture(Source, vTexCoord).rgb;
   frame = mix(frame, (((frame - 0.5) * (1.0 + params.D_contrast * 5.)) + 0.5), params.D_factor);
   frame += params.D_brightness * params.D_factor;
   vec3 Daltonized_frame = Daltonize(frame);
   FragColor = vec4(mix(frame, Daltonized_frame, params.D_factor), 1.0);
}
