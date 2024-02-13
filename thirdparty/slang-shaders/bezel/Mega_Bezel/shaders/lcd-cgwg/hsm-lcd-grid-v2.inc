
layout(push_constant) uniform Push
{
    float LCD_RSUBPIX_R;
    float LCD_RSUBPIX_G;
    float LCD_RSUBPIX_B;
    float LCD_GSUBPIX_R;
    float LCD_GSUBPIX_G;
    float LCD_GSUBPIX_B;
    float LCD_BSUBPIX_R;
    float LCD_BSUBPIX_G;
    float LCD_BSUBPIX_B;
    float LCD_GAIN;
    float LCD_GAMMA;
    float LCD_BLACK_LEVEL;
    float LCD_AMBIENT;
    float LCD_BGR;
} param;

#pragma parameter LCDGRID_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter LCD_TITLE	        "[ --- LCD GRID V2 --- ]:" 0 0 0.01 0.01
#pragma parameter LCD_SUBPIX_TITLE	"[ SUBPIXEL COLORS ]:" 0 0 0.01 0.01
#pragma parameter LCD_RSUBPIX_R     "          Colour of R subpixel: R" 0.75 0.0 1.0 0.01
#pragma parameter LCD_RSUBPIX_G     "          Colour of R subpixel: G" 0.0 0.0 1.0 0.01
#pragma parameter LCD_RSUBPIX_B     "          Colour of R subpixel: B" 0.0 0.0 1.0 0.01
#pragma parameter LCD_GSUBPIX_R     "          Colour of G subpixel: R" 0.0 0.0 1.0 0.01
#pragma parameter LCD_GSUBPIX_G     "          Colour of G subpixel: G" 0.75 0.0 1.0 0.01
#pragma parameter LCD_GSUBPIX_B     "          Colour of G subpixel: B" 0.0 0.0 1.0 0.01
#pragma parameter LCD_BSUBPIX_R     "          Colour of B subpixel: R" 0.0 0.0 1.0 0.01
#pragma parameter LCD_BSUBPIX_G     "          Colour of B subpixel: G" 0.0 0.0 1.0 0.01
#pragma parameter LCD_BSUBPIX_B     "          Colour of B subpixel: B" 0.75 0.0 1.0 0.01
#pragma parameter LCDGRID_BRIGHTNESS_EMPTY_LINE							" " 0 0 0.001 0.001
#pragma parameter LCD_BRIGHT_TITLE	"[ BRIGHTNESS & GAMMA ]:" 0 0 0.01 0.01
#pragma parameter LCD_GAIN          "          Gain"                    1.30 0.5 2.0 0.05 // This was 1.75 before for an unknown reason
#pragma parameter LCD_GAMMA         "          LCD Gamma"               2.5 0.5 5.0 0.1
#pragma parameter LCD_AMBIENT       "          Ambient"                 0.0 0.0 25 0.1
#pragma parameter LCD_BGR           "          BGR LCD Sub Pixel Order" 0 0 1 1

#define outgamma 2.2



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

layout(set = 0, binding = 2) uniform sampler2D Source;

layout(set = 0, binding = 3) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 4) uniform sampler2D AvgLumPass;

layout(location = 0) out vec4 FragColor;

vec3 fetch_offset(sampler2D in_sampler, vec2 coord, vec2 offset) 
{
    offset = vec2(0);
    vec2 texel_size = 1 / CROPPED_ROTATED_SIZE;
    vec2 pixel_coord = SAMPLE_AREA_START_PIXEL_COORD + coord * CROPPED_ROTATED_SIZE + offset;

    vec3 out_rgb = HSM_GetCroppedTexSample(in_sampler, coord + offset * texel_size).rgb;
    out_rgb = pow(vec3(param.LCD_GAIN) * out_rgb, vec3(param.LCD_GAMMA));
    out_rgb += vec3(param.LCD_AMBIENT / 100);
    return out_rgb;
}

// integral of (1 - x^2 - x^4 + x^6)^2
float coeffs_x[7] = float[](1.0, -2.0/3.0, -1.0/5.0, 4.0/7.0, -1.0/9.0, -2.0/11.0, 1.0/13.0);
// integral of (1 - 2x^4 + x^6)^2
float coeffs_y[7] = float[](1.0,      0.0, -4.0/5.0, 2.0/7.0,  4.0/9.0, -4.0/11.0, 1.0/13.0);

float intsmear_func(float z, float coeffs[7])
{
    float z2 = z*z;
    float zn = z;
    float ret = 0.0;
    for (int i = 0; i < 7; i++) {
        ret += zn*coeffs[i];
        zn *= z2;
    }
    return ret;
}

float intsmear(float x, float dx, float d, float coeffs[7])
{
    float zl = clamp((x-dx*0.5)/d,-1.0,1.0);
    float zh = clamp((x+dx*0.5)/d,-1.0,1.0);
    return d * ( intsmear_func(zh,coeffs) - intsmear_func(zl,coeffs) )/dx;
}

void main()
{
    // HSM Added
	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec2 cache_bounds_coord = SCREEN_COORD;

// If it's the potato preset render the whole frame
#ifndef IS_POTATO_PRESET
#ifndef IS_NO_REFLECT_PRESET
	// Have to get the scale of the coordinates so we can figure out the size of the onscreen rectangle of the area 
	HSM_GetBezelCoords(TUBE_DIFFUSE_COORD, 
						TUBE_DIFFUSE_SCALE, 
						TUBE_SCALE, 
						SCREEN_ASPECT, 
						false,
						BEZEL_OUTSIDE_SCALE,
						BEZEL_OUTSIDE_COORD, 
						BEZEL_OUTSIDE_CURVED_COORD, 
						FRAME_OUTSIDE_CURVED_COORD);
	cache_bounds_coord = (BEZEL_OUTSIDE_COORD - 0.5) * 0.9 + 0.5;
#endif
#endif

	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
	{
			FragColor = vec4(0);
			return;
	}

	// HSM Added
	float raster_bloom_avg_lum = texture(AvgLumPass, vec2(0.5,0.5)).a;
	vec2 screen_curved_coord = vec2(0.5);

	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetCrtShaderFinalCoord(SCREEN_COORD,
		SCREEN_SCALE,
		raster_bloom_avg_lum,
		screen_curved_coord);

	vec2 curved_coord = screen_curved_coord_with_overscan_and_mirror;
    vec2 texelSize = 1 / CROPPED_ROTATED_SIZE_WITH_RES_MULT;

	float screen_mask = HSM_GetCornerMask((screen_curved_coord - 0.5) * 1.00 + 0.5, SCREEN_ASPECT, HSM_GLOBAL_CORNER_RADIUS * HSM_SCREEN_CORNER_RADIUS_SCALE, 0.9);
    // End Addition

    vec2 range = global.OutputSize.zw / SCREEN_SCALE;

    vec3 cred   = pow(vec3(param.LCD_RSUBPIX_R, param.LCD_RSUBPIX_G, param.LCD_RSUBPIX_B), vec3(outgamma));
    vec3 cgreen = pow(vec3(param.LCD_GSUBPIX_R, param.LCD_GSUBPIX_G, param.LCD_GSUBPIX_B), vec3(outgamma));
    vec3 cblue  = pow(vec3(param.LCD_BSUBPIX_R, param.LCD_BSUBPIX_G, param.LCD_BSUBPIX_B), vec3(outgamma));

    ivec2 tli = ivec2(floor(curved_coord/texelSize-vec2(0.4999)));

    vec3 lcol, rcol;
    float subpix = (curved_coord.x/texelSize.x - 0.4999 - float(tli.x))*3.0 - 0.33;
    float rsubpix = range.x/texelSize.x * 3.0;

    lcol = vec3(intsmear(subpix+1.0, rsubpix, 1.5, coeffs_x),
                intsmear(subpix    , rsubpix, 1.5, coeffs_x),
                intsmear(subpix-1.0, rsubpix, 1.5, coeffs_x));
    rcol = vec3(intsmear(subpix-2.0, rsubpix, 1.5, coeffs_x),
                intsmear(subpix-3.0, rsubpix, 1.5, coeffs_x),
                intsmear(subpix-4.0, rsubpix, 1.5, coeffs_x));

    if (param.LCD_BGR > 0.5) {
        lcol.rgb = lcol.bgr;
        rcol.rgb = rcol.bgr;
    }

    float tcol, bcol;
    subpix = curved_coord.y/texelSize.y - 0.4999 - float(tli.y);
    rsubpix = range.y/texelSize.y;
    tcol = intsmear(subpix    ,rsubpix, 0.63, coeffs_y);
    bcol = intsmear(subpix-1.0,rsubpix, 0.63, coeffs_y);

    vec3 topLeftColor     = fetch_offset(Source, curved_coord, ivec2(-1, -1)) * lcol * vec3(tcol);
    vec3 bottomRightColor = fetch_offset(Source, curved_coord, ivec2( 1,  1)) * rcol * vec3(bcol);
    vec3 bottomLeftColor  = fetch_offset(Source, curved_coord, ivec2(-1,  1)) * lcol * vec3(bcol);
    vec3 topRightColor    = fetch_offset(Source, curved_coord, ivec2( 1, -1)) * rcol * vec3(tcol);

    vec3 averageColor = topLeftColor + bottomRightColor + bottomLeftColor + topRightColor;

    // HSM Added
    vec3 source_color = HSM_GetCroppedTexSample(Source, curved_coord).rgb;
    vec3 averageColorOutsideScreen = mix(mat3(cred, cgreen, cblue) * pow(source_color, vec3(outgamma)), pow(source_color, vec3(outgamma)), 0.75);
    averageColor = mix(averageColorOutsideScreen, averageColor, screen_mask);
    
    FragColor = vec4(averageColor, 1);
	FragColor = pow(FragColor, vec4(1 / (HSM_GAMMA_OUT_CRT / DEFAULT_SRGB_GAMMA)));
}
