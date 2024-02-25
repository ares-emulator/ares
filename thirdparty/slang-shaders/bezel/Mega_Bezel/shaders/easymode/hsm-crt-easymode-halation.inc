/*
CRT Shader by EasyMode with addition of scaling functions from hsm-mega-screen-scale
License: GPL

Addition of hsm-mega-screen-scale by HyperspaceMadness remimcgill@hotmail.com
*/

layout(push_constant) uniform Push
{
	// HSM Removed
    // float GAMMA_OUTPUT;
	float ESM_SHARPNESS_H;
	float ESM_SHARPNESS_V;
	float ESM_MASK_TYPE;
	float ESM_MASK_STRENGTH_MIN;
	float ESM_MASK_STRENGTH_MAX;
	float ESM_MASK_SIZE;
	float ESM_SCANLINE_STRENGTH_MIN;
	float ESM_SCANLINE_STRENGTH_MAX;
	float ESM_SCANLINE_BEAM_MIN;
	float ESM_SCANLINE_BEAM_MAX;
    // HSM Removed
	// float GEOM_CURVATURE;
	// float GEOM_WARP;
	// float GEOM_CORNER_SIZE;
	// float GEOM_CORNER_SMOOTH;
	float ESM_INTERLACING_TOGGLE;
	float ESM_HALATION;
	float ESM_DIFFUSION;
	float ESM_BRIGHTNESS;
} params;

// HSM Removed
// #pragma parameter GAMMA_OUTPUT          "    Gamma Output" 2.2 0.1 5.0 0.01
#pragma parameter ESM_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter ESM_EASYMODE_TITLE        "[ --- EASYMODE--- ]:" 0 0 0 1
#pragma parameter ESM_SCANLINE_TITLE        "[ SCANLINE ]:" 0 0 0 1
// #pragma parameter ESM_INTERLACING_TOGGLE    "[EASYMODE SCANLINE]  Interlace ON" 0 0.0 1.0 1.0
#pragma parameter ESM_SCANLINE_STRENGTH_MIN "          Strength Min." 0.2 0.0 1.0 0.05
#pragma parameter ESM_SCANLINE_STRENGTH_MAX "          Strength Max." 0.4 0.0 1.0 0.05
#pragma parameter ESM_SCANLINE_BEAM_MIN     "          Beam Min." 1.0 0.25 5.0 0.05
#pragma parameter ESM_SCANLINE_BEAM_MAX     "          Beam Max." 1.0 0.25 5.0 0.05

#pragma parameter ESM_SHARPNESS_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter ESM_SHARPNESS_TITLE       "[ SHARPNESS ]:" 0 0 0 1
#pragma parameter ESM_SHARPNESS_H           "          Horizontal" 0.6 0.0 1.0 0.05
#pragma parameter ESM_SHARPNESS_V           "          Vertical" 1.0 0.0 1.0 0.05

#pragma parameter ESM_MASK_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter ESM_MASK_TITLE            "[ MASK ]:" 0 0 0 1
#pragma parameter ESM_MASK_TYPE             "          Type" 4.0 0.0 7.0 1.0
#pragma parameter ESM_MASK_SIZE             "          Size - 0:Auto, 1:2K, 2:4K" 0 0 100 1
#pragma parameter ESM_MASK_STRENGTH_MIN     "          Strength Min." 0.2 0.0 0.5 0.01
#pragma parameter ESM_MASK_STRENGTH_MAX     "          Strength Max." 0.2 0.0 0.5 0.01

#pragma parameter ESM_GLOW_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter ESM_GLOW_TITLE            "[ GLOW ]:" 0 0 0 1
#pragma parameter ESM_HALATION              "          Halation" 0.03 0.0 1.0 0.01
#pragma parameter ESM_DIFFUSION             "          Diffusion" 0.0 0.0 1.0 0.01
#pragma parameter ESM_BRIGHTNESS            "          Brightness" 0.73 0.0 2.0 0.01

// HSM Removed
// #pragma parameter GEOM_CURVATURE "Geom Curvature" 0.0 0.0 0.1 0.01
// #pragma parameter GEOM_WARP "Geom Warp" 0.0 0.0 0.1 0.01
// #pragma parameter GEOM_CORNER_SIZE "Geom Corner Size" 0.0 0.0 0.1 0.01
// #pragma parameter GEOM_CORNER_SMOOTH "Geom Corner Smoothness" 150.0 50.0 1000.0 25.0
// #pragma parameter ESM_INTERLACING_TOGGLE    "    Interlacing Toggle" 1.0 0.0 1.0 1.0

// HSM REMOVED
/* Removed the following
layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	uint FrameCount;
	vec4 OriginalSize;
	vec4 ORIG_LINEARIZEDSize;
} global;
*/

/*
    CRT Shader by EasyMode
    License: GPL
*/

#define FIX(c) max(abs(c), 1e-5)
#define PI 3.141592653589

// HSM Removed
// #define TEX2D(c) texture(tex, c)

// HSM Added
#define TEX2D(c) HSM_GetCroppedTexSample(tex, c)

float curve_distance(float x, float sharp)
{
    float x_step = step(0.5, x);
    float curve = 0.5 - sqrt(0.25 - (x - x_step) * (x - x_step)) * sign(0.5 - x);

    return mix(x, curve, sharp);
}

mat4x4 get_color_matrix(sampler2D tex, vec2 co, vec2 dx)
{
    return mat4x4(TEX2D(co - dx), TEX2D(co), TEX2D(co + dx), TEX2D(co + 2.0 * dx));
}

vec4 filter_lanczos(vec4 coeffs, mat4x4 color_matrix)
{
    vec4 col = color_matrix * coeffs;
    vec4 sample_min = min(color_matrix[1], color_matrix[2]);
    vec4 sample_max = max(color_matrix[1], color_matrix[2]);

    col = clamp(col, sample_min, sample_max);

    return col;
}

vec3 get_scanline_weight(float pos, float beam, float strength)
{
    float weight = 1.0 - pow(cos(pos * 2.0 * PI) * 0.5 + 0.5, beam);
    
    weight = weight * strength * 2.0 + (1.0 - strength);
    
    return vec3(weight);
}

/* HSM Removed
vec2 curve_coordinate(vec2 co, float curvature)
{
    vec2 curve = vec2(curvature, curvature * 0.75);
    vec2 co2 = co + co * curve - curve / 2.0;
    vec2 co_weight = vec2(co.y, co.x) * 2.0 - 1.0;

    co = mix(co, co2, co_weight * co_weight);

    return co;
}

float get_corner_weight(vec2 co, vec2 corner, float smoothfunc)
{
    float corner_weight;
    
    co = min(co, vec2(1.0) - co) * vec2(1.0, 0.75);
    co = (corner - min(co, corner));
    corner_weight = clamp((corner.x - sqrt(dot(co, co))) * smoothfunc, 0.0, 1.0);
    corner_weight = mix(1.0, corner_weight, ceil(corner.x));
    
    return corner_weight;
}
*/

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

layout(set = 0, binding = 2) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 4) uniform sampler2D LinearizePass;
layout(set = 0, binding = 5) uniform sampler2D AvgLumPass;

void main()
{
    /* HSM Removed
    vec2 tex_size = global.SourceSize.xy;
    */

    // TODO implement Cropping
    // HSM Added
	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);
    vec2 tex_size = CROPPED_ROTATED_SIZE_WITH_RES_MULT;
    // End Addition

	vec2 cache_bounds_coord = SCREEN_COORD;

// // If it's the potato preset render the whole frame
// #ifndef IS_POTATO_PRESET
// #ifndef IS_NO_REFLECT_PRESET
// 	vec2 bezel_outside_flat_coord;
// 	vec2 frame_outside_flat_coord;
// 	HSM_GetSimpleBezelCoords(TUBE_DIFFUSE_COORD, 
// 							TUBE_DIFFUSE_SCALE, 
// 							TUBE_SCALE, 
// 							SCREEN_ASPECT,
// 							bezel_outside_flat_coord, 
// 							frame_outside_flat_coord);
// 	cache_bounds_coord = (frame_outside_flat_coord - 0.5) * 0.9 + 0.5;
// #endif
// #endif

// 	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
// 	{
// 			FragColor = vec4(0);
// 			return;
// 	}

    vec2 midpoint = vec2(0.5, 0.5);
    float scan_offset = 0;
    float timer = vec2(global.FrameCount, global.FrameCount).x;

    /* HSM Removed
    if (params.ESM_INTERLACING_TOGGLE > 0.5 && global.SourceSize.y >= 400)
    {
        tex_size.y *= 0.5;

        if (mod(timer, 2.0) > 0.0)
        {
            midpoint.y = 0.75;
            scan_offset = 0.5;
        }        
        else midpoint.y = 0.25;
    }

    vec2 co = vTexCoord * tex_size * global.SourceSize.zw;
    vec2 xy = curve_coordinate(co, params.GEOM_WARP);
    float corner_weight = get_corner_weight(curve_coordinate(co, params.GEOM_CURVATURE), vec2(params.GEOM_CORNER_SIZE), params.GEOM_CORNER_SMOOTH);
    Not sure why params.GEOM_WARP was used for the main coordinate and params.GEOM_CURVATURE was used for the corner
    */

    // HSM Added


    vec2 co = SCREEN_COORD;
    vec2 screen_coord = co;

	// HSM Added
	float raster_bloom_avg_lum = texture(AvgLumPass, vec2(0.5,0.5)).a;
	vec2 screen_curved_coord = vec2(0.5);

	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetCrtShaderFinalCoord(SCREEN_COORD,
		SCREEN_SCALE,
		raster_bloom_avg_lum,
		screen_curved_coord);

	vec2 xy = screen_curved_coord_with_overscan_and_mirror;
	// End Addition

    // HSM Removed
    // float corner_weight = 1;

    vec2 dx = vec2(1.0 / tex_size.x, 0.0);
    vec2 dy = vec2(0.0, 1.0 / tex_size.y);
    vec2 pix_co = xy * tex_size - midpoint;
    vec2 tex_co = (floor(pix_co) + midpoint) / tex_size;
    vec2 dist = fract(pix_co);
    float curve_x, curve_y;
    vec3 col, col2, diff;

    curve_x = curve_distance(dist.x, params.ESM_SHARPNESS_H * params.ESM_SHARPNESS_H);
    curve_y = curve_distance(dist.y, params.ESM_SHARPNESS_V * params.ESM_SHARPNESS_V);

    vec4 coeffs_x = PI * vec4(1.0 + curve_x, curve_x, 1.0 - curve_x, 2.0 - curve_x);
    vec4 coeffs_y = PI * vec4(1.0 + curve_y, curve_y, 1.0 - curve_y, 2.0 - curve_y);

    coeffs_x = FIX(coeffs_x);
    coeffs_x = 2.0 * sin(coeffs_x) * sin(coeffs_x / 2.0) / (coeffs_x * coeffs_x);
    coeffs_x /= dot(coeffs_x, vec4(1.0));

    coeffs_y = FIX(coeffs_y);
    coeffs_y = 2.0 * sin(coeffs_y) * sin(coeffs_y / 2.0) / (coeffs_y * coeffs_y);
    coeffs_y /= dot(coeffs_y, vec4(1.0));

    mat4x4 color_matrix;

    color_matrix[0] = filter_lanczos(coeffs_x, get_color_matrix(LinearizePass, tex_co - dy, dx));
    color_matrix[1] = filter_lanczos(coeffs_x, get_color_matrix(LinearizePass, tex_co, dx));
    color_matrix[2] = filter_lanczos(coeffs_x, get_color_matrix(LinearizePass, tex_co + dy, dx));
    color_matrix[3] = filter_lanczos(coeffs_x, get_color_matrix(LinearizePass, tex_co + 2.0 * dy, dx));

    col = filter_lanczos(coeffs_y, color_matrix).rgb;
    diff = HSM_GetCroppedTexSample(Source, xy).rgb;

    float rgb_max = max(col.r, max(col.g, col.b));

    /* HSM Removed
    float sample_offset = (global.SourceSize.y * global.OutputSize.w) * 0.5;
    */

    // HSM Added the Following
    float sample_offset = (tex_size.y * global.OutputSize.w * SCREEN_SCALE.y) * 0.5;
    // End Addition

    float scan_pos = xy.y * tex_size.y + scan_offset;
    float scan_strength = mix(params.ESM_SCANLINE_STRENGTH_MAX, params.ESM_SCANLINE_STRENGTH_MIN, rgb_max);
    float scan_beam = clamp(rgb_max * params.ESM_SCANLINE_BEAM_MAX, params.ESM_SCANLINE_BEAM_MIN, params.ESM_SCANLINE_BEAM_MAX);
    vec3 scan_weight = vec3(0.0);

    float mask_colors;
    float mask_dot_width;
    float mask_dot_height;
    float mask_stagger;
    float mask_dither;
    vec4 mask_config;

    if      (params.ESM_MASK_TYPE == 1) mask_config = vec4(2.0, 1.0, 1.0, 0.0);
    else if (params.ESM_MASK_TYPE == 2) mask_config = vec4(3.0, 1.0, 1.0, 0.0);
    else if (params.ESM_MASK_TYPE == 3) mask_config = vec4(2.1, 1.0, 1.0, 0.0);
    else if (params.ESM_MASK_TYPE == 4) mask_config = vec4(3.1, 1.0, 1.0, 0.0);
    else if (params.ESM_MASK_TYPE == 5) mask_config = vec4(2.0, 1.0, 1.0, 1.0);
    else if (params.ESM_MASK_TYPE == 6) mask_config = vec4(3.0, 2.0, 1.0, 3.0);
    else if (params.ESM_MASK_TYPE == 7) mask_config = vec4(3.0, 2.0, 2.0, 3.0);

    mask_colors = floor(mask_config.x);
    mask_dot_width = mask_config.y;
    mask_dot_height = mask_config.z;
    mask_stagger = mask_config.w;
    mask_dither = fract(mask_config.x) * 10.0;

    /* HSM Removed
    vec2 mod_fac = floor(vTexCoord * global.OutputSize.xy * global.SourceSize.xy / (global.SourceSize.xy * vec2(params.ESM_MASK_SIZE, mask_dot_height * params.ESM_MASK_SIZE)));
    */

    // HSM Added
    	// HSM Added
	float final_mask_size = params.ESM_MASK_SIZE;
	// If using automatic Mask Size
	if (final_mask_size < 0.5)
	{
		final_mask_size = (global.FinalViewportSize.x + global.FinalViewportSize.y) / 3 > 1900 ? 2 : 1;
	}

    vec2 mod_fac = floor(SCREEN_COORD * global.OutputSize.xy * SCREEN_SCALE * tex_size.xy / (tex_size.xy * vec2(final_mask_size, mask_dot_height * final_mask_size)));
    // End Addition

    int dot_no = int(mod((mod_fac.x + mod(mod_fac.y, 2.0) * mask_stagger) / mask_dot_width, mask_colors));
    float dither = mod(mod_fac.y + mod(floor(mod_fac.x / mask_colors), 2.0), 2.0);

    float mask_strength = mix(params.ESM_MASK_STRENGTH_MAX, params.ESM_MASK_STRENGTH_MIN, rgb_max);
    float mask_dark, mask_bright, mask_mul;
    vec3 mask_weight;

    mask_dark = 1.0 - mask_strength;
    mask_bright = 1.0 + mask_strength * 2.0;

    if (dot_no == 0) mask_weight = mix(vec3(mask_bright, mask_bright, mask_bright), vec3(mask_bright, mask_dark, mask_dark), mask_colors - 2.0);
    else if (dot_no == 1) mask_weight = mix(vec3(mask_dark, mask_dark, mask_dark), vec3(mask_dark, mask_bright, mask_dark), mask_colors - 2.0);
    else mask_weight = vec3(mask_dark, mask_dark, mask_bright);

    if (dither > 0.9) mask_mul = mask_dark;
    else mask_mul = mask_bright;

    mask_weight *= mix(1.0, mask_mul, mask_dither);
    mask_weight = mix(vec3(1.0), mask_weight, clamp(params.ESM_MASK_TYPE, 0.0, 1.0));

    col2 = (col * mask_weight);
    col2 *= params.ESM_BRIGHTNESS;

    scan_weight = get_scanline_weight(scan_pos - sample_offset, scan_beam, scan_strength);
    col = clamp(col2 * scan_weight, 0.0, 1.0);
    scan_weight = get_scanline_weight(scan_pos, scan_beam, scan_strength);
    col += clamp(col2 * scan_weight, 0.0, 1.0);
    scan_weight = get_scanline_weight(scan_pos + sample_offset, scan_beam, scan_strength);
    col += clamp(col2 * scan_weight, 0.0, 1.0);
    col /= 3.0;

    /* HSM Removed
    col *= vec3(corner_weight);
    col += diff * mask_weight * params.ESM_HALATION * vec3(corner_weight);
    col += diff * params.ESM_DIFFUSION * vec3(corner_weight);
    col = pow(col, vec3(1.0 / params.GAMMA_OUTPUT));
    */

    // HSM Added
    col += diff * mask_weight * params.ESM_HALATION;
    col += diff * params.ESM_DIFFUSION;
    
    FragColor = vec4(col, 1);

	// Apply gamma offset mimicking the difference in incoming siglnal and CRT image output
	FragColor = pow(FragColor, vec4(1 / (HSM_GAMMA_OUT_CRT / DEFAULT_SRGB_GAMMA)));
}
