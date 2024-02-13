layout(push_constant) uniform Push
{
	float phosphor_power;
	float phosphor_amplitude;
	float CRTgamma;
	float width;
	float aspect_x;
	float aspect_y;
	float d;
	float R;
	float angle_x;
	float angle_y;
	float aperture_strength;
	float aperture_brightboost;
	float halation;
	float curvature;
	float cornersize;
	float cornersmooth;
	float overscan_x;
	float overscan_y;
	float monitorgamma;
	float mask_type;
	float SHARPER;
	float scanline_weight;
	float geom_lum;
	float interlace_detect;
	float rasterbloom_smooth;
	float rasterbloom;
} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
    vec4 FinalViewportSize;
    vec4 internal1Size;
	mat4 MVP;
} global;

#pragma parameter mask_type "Mask Pattern" 1.0 1.0 20.0 1.0
#pragma parameter aperture_strength "Shadow mask strength" 0.4 0.0 1.0 0.05
#pragma parameter aperture_brightboost "Shadow mask brightness boost" 0.4 0.0 1.0 0.05
#pragma parameter phosphor_power "Phosphor decay power" 1.2 0.5 3.0 0.05
#pragma parameter phosphor_amplitude "Phosphor persistence amplitude" 0.04 0.0 0.2 0.01
#pragma parameter CRTgamma "Gamma of simulated CRT" 2.4 0.7 4.0 0.05
//#pragma parameter rasterbloom_smooth "Raster bloom temporal smoothing" 0.5 0.0 0.99 0.01
#pragma parameter rasterbloom "Raster bloom amplitude" 0.1 0.0 1.0 0.01
#pragma parameter halation "Halation amplitude" 0.1 0.0 0.3 0.01
#pragma parameter width "Halation blur width" 2.0 0.1 4.0 0.1
#pragma parameter curvature "Enable Curvature" 1.0 0.0 1.0 1.0
#pragma parameter R "Radius of curvature" 3.5 0.5 10.0 0.1
#pragma parameter d "Distance to screen" 2.0 0.1 10.0 0.1
#pragma parameter angle_x "Tilt X" 0.0 -1.0 1.0 0.01
#pragma parameter angle_y "Tilt Y" 0.0 -1.0 1.0 0.01
#pragma parameter cornersize "Rounded corner size" 0.01 0.00 0.10 0.01
#pragma parameter cornersmooth "Border smoothness" 1000.0 100.0 2000.0 100.0
#pragma parameter overscan_x "Overscan X" 1.0 0.8 1.2 0.005
#pragma parameter overscan_y "Overscan Y" 1.0 0.8 1.2 0.005
#pragma parameter monitorgamma "Gamma of output display" 2.2 0.7 4.0 0.05
#pragma parameter aspect_x "Aspect ratio X" 1.0 0.3 1.0 0.01
#pragma parameter aspect_y "Aspect ratio Y" 0.75 0.3 1.0 0.01
#pragma parameter scanline_weight "CRTGeom Scanline Weight" 0.3 0.1 0.5 0.01
#pragma parameter geom_lum "CRTGeom Luminance" 0.0 0.0 1.0 0.01
#pragma parameter interlace_detect "CRTGeom Interlacing Simulation" 1.0 0.0 1.0 1.0

#define phosphor_power params.phosphor_power
#define phosphor_amplitude params.phosphor_amplitude
#define CRTgamma params.CRTgamma
#define width params.width
#define curvature params.curvature
#define d params.d
#define cornersize params.cornersize
#define cornersmooth params.cornersmooth
#define monitorgamma params.monitorgamma
#define halation params.halation
#define aperture_strength params.aperture_strength
#define aperture_brightboost params.aperture_brightboost
#define geom_lum params.geom_lum
#define scanline_weight params.scanline_weight
#define interlace_detect params.interlace_detect
//#define rasterbloom_smooth params.rasterbloom_smooth
#define rasterbloom (params.rasterbloom / 10.0)
vec2 aspect = vec2(params.aspect_x, params.aspect_y);
vec2 angle = vec2(params.angle_x, params.angle_y);
vec2 overscan = vec2(params.overscan_x, params.overscan_y);
int mask_picker = int(params.mask_type);
vec4 blurwidth = vec4(width);

const float gamma = 2.2;
