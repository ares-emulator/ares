#include "../config/config-static.inc"

#define eps 1e-5
#define pi  3.141592653589793
#define TAU 6.28318530717958647693
#define alpha_mark 0.1

layout(push_constant) uniform Push {
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	vec4 avglum_passFeedbackSize;
	uint FrameCount;
	uint Rotation;
	
	float HALO_SHARPNESS ;
	float HALO_NO_PREGAIN;
	float HALO_POWER ;
	float HALO_GAMMA ;
	float HALO_GAMMA_OUT;
	float HALO_VS_SCAN ;
	float DO_BLOOM ; //4
 	float BLOOM_QUALITY ; //1
	float AMBI_ADD_ON_BLACK;
	float AMBI_STRETCH;
	float AMBI_STRETCH_VERTICAL;
	float AMBI_INT_OFFSET ;
	float AMBI_SMP_INT_OFFSET ;
	
} params;

layout(std140, set = 0, binding = 0) uniform UBO {
	mat4 MVP;
	
	vec4 FinalViewportSize;
	vec4 final_passFeedbackSize;
	vec4 in_glow_passSize;
	vec4 FXAA_passSize;

	vec4 avglum_passSize;
	vec4 flick_and_noise_passSize;
	vec4 colortools_and_ntsc_passSize;
	vec4 colortools_and_ntsc_passFeedbackSize;

	float COLOR_MONO_COLORIZE;
	float COLOR_MONO_HUE1;
	float COLOR_MONO_HUE2;
	float COLOR_MONO_HUE_BIAS;
	
	float DO_IN_GLOW;

	float DO_HALO ;
	float DO_CCORRECTION;
	float IN_GLOW_BIAS ;
	float IN_GLOW_SPREAD ;

	float IN_GLOW_W;
	float IN_GLOW_H;
	float IN_GLOW_POWER;
	float IN_GLOW_GAMMA;
	
	float BLOOM_MIX;
	float BLOOM_EYE_ADPT_SRT;
	float BLOOM_EYE_INERTIA;
 	float BLOOM_SIZE;
	float BLOOM_GAMMA;
	float BLOOM_BYPASS;
	float BLOOM_POWER;
	float BLOOM_OVER_WHITE;
  	float BLOOM_GAMMA_OUT;
	
	float DO_FXAA;
	
	float DO_SAT_BLEED;
	float SAT_BLEED_SIZE;
	float SAT_BLEED_FALLOFF;
	float SAT_BLEED_STRENGTH;
	float SAT_BLEED_PAL;
	
	float DO_PPERSISTENCE;
	float PPERSISTENCE_START;
	float PPERSISTENCE_END;
	
	float GAMMA_OUT ;
	float SATURATION;
	float LUMINANCE;
	float VIBRANCE;
	float CONTRAST;
	float ADAPTIVE_BLACK;
	float BRIGHTNESS;
	float TEMPERATURE;
	
	float DO_CURVATURE;
	float GEOM_WARP_X;
	float GEOM_WARP_Y;
	float GEOM_CORNER_SIZE;
	float GEOM_CORNER_SMOOTH;

	float DO_BEZEL;
	float BEZEL_R;
	float BEZEL_G;
	float BEZEL_B;
	float BEZEL_CON;
	float BEZEL_INNER_ZOOM;
	float BEZEL_FRAME_ZOOM;
	float BEZEL_USE_STRAIGHT;
	float BEZEL_RFL_BLR_SHD;
	float BEZEL_RFL_ZOOM;
	float BEZEL_REFL_STRENGTH;
	float BEZEL_DIFFUSION_STR;
	float BEZEL_SPCL_STRENGTH;
	float BEZEL_ROUGHNESS;
	float BEZEL_CORNER_DARK;
	
	float DO_SPOT;
	float S_POSITION_X; 
	float S_POSITION_Y; 
	float S_SIZE;
	float S_POWER;

	float DO_VIGNETTE;
	float V_SIZE;
	float V_POWER;

	float DO_DYNZOOM;

	float DO_AMBILIGHT;
	float AMBI_FALLOFF;
	float AMBI_POWER;
	float AMBI_GAMMA;

	float AMBI_OVER_BEZEL;
	float AMBI_OVER_BEZEL_SIZE;
	float AMBI_STEPS;
	float AMBI_BG_IMAGE_BLEND_MODE;
	float AMBI_BG_IMAGE_FORCE;

	
	float ASPECT_X;
	float ASPECT_Y;
	
	float DO_BG_IMAGE;
	float BG_IMAGE_OVER;
	float BG_IMAGE_OFFY;
	float BG_IMAGE_OFFX;
	float BG_IMAGE_ZOOM;
	float BG_IMAGE_ROTATION;
	float BG_IMAGE_NIGHTIFY;
	float BG_IMAGE_WRAP_MODE;

	float DO_SHIFT_RGB;
	float SHIFT_R;
	float SHIFT_G;
	float SHIFT_B;
	float OFFSET_STRENGTH;
	
	float DO_GLOBAL_SHZO;
	float GLOBAL_ZOOM;
	float GLOBAL_OFFX;
	float GLOBAL_OFFY;

	float DO_RF_NOISE;
	float RF_NOISE_STRENGTH;
	
	float DO_GAME_GEOM_OVERRIDE;
	float GAME_GEOM_INT_SCALE;
	float GAME_GEOM_INT_SCALE_OVR;
	float GAME_GEOM_INT_SCALE_MAX;
	float GAME_GEOM_ASPECT;
	float GAME_GEOM_VSHIFT;
	float GAME_GEOM_HSHIFT;
	float GAME_GEOM_ZOOM;
	float GAME_GEOM_OFF_FIX; 
	
	float DO_TILT;
	float TILT_X;
	float TILT_BEZEL_K;
	float TILT_FOV;
	
	float AUTOCROP_MAX;
	float AUTOCROP_MIN;
	float AUTOCROP_SAMPLES;
	float AUTOCROP_SAMPLE_SIZE;
	float AUTOCROP_TRANSITION_SPEED;
	float AUTOCROP_STEADINESS;
	
	float DO_BACKDROP; 
	float BACKDROP_OFFX;
	float BACKDROP_OFFY;
	float BACKDROP_ZOOM;
	
	float RESSWITCH_GLITCH_SIZE;
	float TATE;

	float DO_DOT_MATRIX;
	float DOT_M_G_SHARP;
	float DOT_M_G_TRESH;
	float DOT_M_G_STR;
	float DOT_M_G_BRT;
	float DOT_M_RGB_SHARP;
	float DOT_M_RGB_STR;
	float DOT_M_G_GAIN;
	float DOT_M_MBLUR_STR;
	float DOT_M_MBLUR_MODE;
	float DOT_M_MULTIPLY;
	float DOT_M_SHADOW_STR;
	float DOT_M_SHADOW_OFF;
	
	float MIN_LINES_INTERLACED;
	float DO_PIXELGRID;
	float PIXELGRID_OVERMASK;
	float DO_PIXELGRID_W;
	float DO_PIXELGRID_H;
	float PIXELGRID_H_ANTIMOIRE;
	float PIXELGRID_COREY_FAKE_SCAN;
	float PIXELGRID_DOUBLESCAN;
	float PIXELGRID_MIN_H;
	float PIXELGRID_MIN_W;
	float PIXELGRID_MAX_H;
	float PIXELGRID_MAX_W;
	float PIXELGRID_SIZE_W;
	float PIXELGRID_INFLATION;
	
	float BEZEL_LIGHT_NO_FADEOUT;
	float IN_GLOW_SPREAD_SHARPNESS;

	float PIXELGRID_MUL_X;
													
	float PIXELGRID_OFFSET_CORE;
	float PIXELGRID_NO_INTERBLEED_W;
	float PIXELGRID_NO_INTERBLEED_H;
	float PIXELGRID_H_DEDOT;
	
	float PIXELGRID_INTR_FLICK_MODE;
	float PIXELGRID_INTR_FLICK_POWR;
	float PIXELGRID_INTR_DISABLE_Y;
	
	
	float PIXELGRID_DECON_R_H;
	float PIXELGRID_DECON_G_H;
	float PIXELGRID_DECON_B_H;
	
	float PIXELGRID_Y_MASK;
	float PIXELGRID_Y_MASK_ON_WHITE;
	float PIXELGRID_Y_MASK_HEIGHT;
	float PIXELGRID_Y_MASK_OFFSET;
	float PIXELGRID_Y_MASK_STEEP;
	float PIXELGRID_Y_MASK_COORDS;
	float PIXELGRID_Y_MASK_SHIFT;
	float PIXELGRID_Y_SPARK;
	float PIXELGRID_R_SHIFT;
	float PIXELGRID_G_SHIFT;
	float PIXELGRID_B_SHIFT;
	float PIXELGRID_H_COUNT;
	float PIXELGRID_H_PRST;
	float PIXELGRID_GAMMA_H;
	float PIXELGRID_GAMMA_W;
	float PIXELGRID_DO_SHADOWMASK;
	float PIXELGRID_SHADOWMASK_H;
	float PIXELGRID_SHADOWMASK_SHIFT;
	//float PIXELGRID_HMASK_NO_BRIGHT;
	//float MOIRE_SWEET_SPOT;
	
	float PIXELGRID_BASAL_GRID;
	float DELTA_RENDER_FORCE_REFRESH;
	float DELTA_RENDER_CHECK_AREA;
	
	float DO_DEDITHER;
	float DEDITHER_STRENGTH;
	float DEDITHER_STRENGTH2;
	float DEDITHER_TRESHMAX;
	
	float DO_NTSC_ARTIFACTS;
	float NTSC_FILTER_WIDTH;
	float NTSC_FILTER_SCF;
	float NTSC_FILTER_FC;
	float NTSC_MIX;
	float NTSC_PHASE_SHIFT;

	float NTSC_ARTF_NOBLEED;
	float NTSC_ARTF_NOBLUR;
	float NTSC_ARTF_TRSH;
	float NTSC_SHOW_ARTF_MASK;
	float NTSC_ARTF_ZAP;
	
	float SERVICE1;
	

	
} global;


#pragma parameter LABEL0000 "Read the documentation in docs-ng.md text file" 		0.0 0.0 0.0 0.0
#pragma parameter LABEL0001 "==================================================" 		0.0 0.0 0.0 0.0
#pragma parameter LABEL0002 " " 		0.0 0.0 0.0 0.0

// Color correction
#pragma parameter DO_CCORRECTION "★ Color corrections enable? (co) ==>" 		0.0 	0.0 1.0 1.0
#pragma parameter IN_GLOW_POWER  "       Input signal gain"					1.0 	0.0 5.0 0.05
#pragma parameter LUMINANCE      "       Luminance push (>1.0=clip)"		0.0	 0.0 1.5 0.05
#pragma parameter BRIGHTNESS     "       Brightness in   (0.0=off)"		0.0	-1.0 1.0 0.01
#pragma parameter CONTRAST       "       Contrast in     (0.0=off)"		0.0	-1.0 1.0 0.01
#pragma parameter ADAPTIVE_BLACK "       Adaptive black level range"	0.0 0.0  1.0 0.01
#pragma parameter TEMPERATURE    "       Temperature in (6500=off)"		6500.0	3000 14000.0 50.0
#pragma parameter IN_GLOW_GAMMA  "       Gamma In  (applied at the beginning)"		1.0 	1.0 	10.0 0.1
#pragma parameter GAMMA_OUT             "       Gamma out (applied at the end)"			0.69	0.1 	4.0  0.01
#pragma parameter COLOR_MONO_COLORIZE	"       Monochrome screen colorization"	0.0  0.0 1.0 0.1
#pragma parameter COLOR_MONO_HUE1       "          . Hue bright"					0.25 0.0 1.0 0.01
#pragma parameter COLOR_MONO_HUE2       "          . Hue dark"						0.3  0.0 1.0 0.01
#pragma parameter COLOR_MONO_HUE_BIAS   "          . Hue bright-dark bias"		0.0	-2.0 2.0 0.01
#pragma parameter SATURATION            "       Saturation in   (1.0=off)"		1.0	0.0  2.0 0.01
#pragma parameter VIBRANCE              "       Vibrance in   (0.0=off)"		0.0	0.0  5.0 0.1
#pragma parameter LABEL0003 " " 0.0 0.0 0.0 0.0


// FXAA
// Apply an antialiasing filter via FXAA from Nvidia.
#pragma parameter DO_FXAA "★ FXAA enable? (*f) ==>"	0.0 	0.0 1.0 1.0
#pragma parameter LABEL0004 " " 0.0 0.0 0.0 0.0


// RF Noise
#pragma parameter DO_RF_NOISE	 "★ RF Noise enable? (rf) ==>"			0.0 	0.0  1.0     1.0
#pragma parameter RF_NOISE_STRENGTH "        Strength"				0.05	0.0 1.0 0.005
#pragma parameter LABEL0005 " " 0.0 0.0 0.0 0.0

// Dedithering
#pragma parameter DO_DEDITHER         "★ Dedither enable? (not with NTSC artifacts) ==>"	0.0 	0.0 1.0 1.0
#pragma parameter DEDITHER_TRESHMAX   "        Sensitivity"				1.2	0.0 3.0  0.1
#pragma parameter DEDITHER_STRENGTH   "        Basic search strength"				0.8	0.0 1.0  0.1
#pragma parameter DEDITHER_STRENGTH2  "        Extensive search strength"			0.0	0.0 1.0  0.1

#pragma parameter LABEL0006 " " 0.0 0.0 0.0 0.0

// NTSC color artifacting
#pragma parameter DO_NTSC_ARTIFACTS "★ CVBS: NTSC color artifacts enable? (nt) ==>"	0.0 	0.0 1.0 1.0
#pragma parameter NTSC_MIX          "        Strength"				0.5	0.0 3.0  0.1
#pragma parameter NTSC_FILTER_WIDTH "        Filter width"			17.0	5.0 21.0 4.0
#pragma parameter NTSC_FILTER_SCF   "        Subcarrier Frequency mul"		4.57	1.0 10.0 0.01
#pragma parameter NTSC_FILTER_FC    "        Frequency cutoff"			2.5	0.0 10.0 0.1
#pragma parameter NTSC_PHASE_SHIFT  "        Phase shift"			0.0	0.0 1.0  1.0
#pragma parameter label_blut_ntsc   "        1 and 2 need Glow or Bandwidth Limited chroma" 0.0 0.0 0.0 0.0
#pragma parameter NTSC_ARTF_TRSH      "      . Consider artifacts above this treshold"			0.1	0.0 1.0 0.01
#pragma parameter NTSC_SHOW_ARTF_MASK "         . Show the mask of selected artifacts (debug)"				0.0	0.0 1.0 1.0
#pragma parameter NTSC_ARTF_NOBLUR    "         . 1* Under treshold: Cancel blur (Glow)"				0.0	0.0 8.0 0.01
#pragma parameter NTSC_ARTF_NOBLEED   "         . 2* Under treshold: Cancel Bandwidth limited chroma"	0.0	0.0 1.0 0.01
#pragma parameter NTSC_ARTF_ZAP       "         . 3* Under treshold: Cancel artifacts"					0.0	0.0 1.0 0.01

#pragma parameter LABEL0007 " " 0.0 0.0 0.0 0.0


// YIQ/YUV bandwidth limited chroma bleeding.
#pragma parameter DO_SAT_BLEED "★ CVBS: Bandwidth limited chroma enable? (cv) ==>"	0.0 	0.0 1.0 1.0
#pragma parameter SAT_BLEED_PAL           "        Use PAL colorspace, not NTSC"	0.0 	 0.0 1.0 1.0
#pragma parameter SAT_BLEED_STRENGTH      "        Strength"						1.0	 0.0 5.0  0.01
#pragma parameter SAT_BLEED_SIZE          "        Size (capped to 5.0 in d3d)"		5.0  1.0 20.0  1.0
#pragma parameter SAT_BLEED_FALLOFF  	  "        Falloff"				1.7 	 1.0 2.0 0.01
#pragma parameter LABEL0008 " " 0.0 0.0 0.0 0.0


//Phosphor persistence
#pragma parameter DO_PPERSISTENCE       "★ Persistence of phosphors enable? (ppe) ==>"	0.0	 0.0 1.0 1.0
#pragma parameter PPERSISTENCE_START    "        Early decay"			0.9 	 0.0 1.0 0.05
#pragma parameter PPERSISTENCE_END      "        Late Persistence"		0.5 	 0.0 2.0 0.05
#pragma parameter LABEL0009 " " 0.0 0.0 0.0 0.0

//Deconvergence
#pragma parameter DO_SHIFT_RGB       "★ Deconvergence enable? (de) ==>"	0.0	 0.0 1.0 1.0
#pragma parameter OFFSET_STRENGTH "        Strength"			0.6 	 0.0 1.0 0.05
#pragma parameter SHIFT_R         "        Red offset"			-40.0 	-210.0 189.0 1
#pragma parameter SHIFT_G         "        Green offset"		2.0 	-210.0 189.0 1
#pragma parameter SHIFT_B         "        Blue offset"			40.0 	-210.0 189.0 1
#pragma parameter LABEL0010 " " 0.0 0.0 0.0 0.0



// Input Blur / light glowing
#pragma parameter DO_IN_GLOW               "★ Glow/Blur enable? (gl) ==>"	 					0.0	0.0 1.0 1.0
#pragma parameter IN_GLOW_BIAS             "        Glow to blur bias"							1.0     0.0 1.0 0.1
#pragma parameter IN_GLOW_SPREAD           "        Glow light gain"		0.6 	0.0 4.0 0.05
#pragma parameter IN_GLOW_SPREAD_SHARPNESS "        Glow light sharpness"	2.0 	0.01 2.0 0.01
#pragma parameter IN_GLOW_W                "        Sharp X (neg=use BOX unsharp)"		2.5	-2.0 7.0 0.05
#pragma parameter IN_GLOW_H                "        Sharp Y (neg=use BOX unsharp)"		2.5	-2.0 7.0 0.05
#pragma parameter LABEL0011 " " 0.0 0.0 0.0 0.0


#pragma parameter TATE                     "★ Tate mode (0:disabled 1:auto 2:forced)"		1.0 	0.0 2.0 1.0
#pragma parameter LABEL0012 " " 0.0 0.0 0.0 0.0

#pragma parameter RESSWITCH_GLITCH_SIZE		"★ Glitch if vertical resolution changes"		0.1		0.0 0.5 0.05
#pragma parameter LABEL0013 " " 0.0 0.0 0.0 0.0

#pragma parameter LABEL_INTERLACE           "★ Hi-resolution scanlines handling" 1.0 1.0 1.0 0.0
#pragma parameter MIN_LINES_INTERLACED      "    . Consider Hi-Resolution above # lines (0=never):"		380.0 	0.0 1024 1.0
#pragma parameter PIXELGRID_INTR_DISABLE_Y  "    . Hi-Res scanlines type (read docs):"   -1.0 -2.0 2.0 1.0
#pragma parameter LABEL0036        "      (-2: Faker-Interlaced, -1: Fake-Interlaced)"          0.0 0.0 0.0 0.0
#pragma parameter LABEL0037        "      ( 0: Interlaced, 1: No scanlines, 2: Don't interlace)" 0.0 0.0 0.0 0.0
#pragma parameter PIXELGRID_INTR_FLICK_MODE "      . Scanlines flicker (0=off,1=on,2=if Hi-Res)"			2.0 	0.0 2.0 1.0
#pragma parameter PIXELGRID_INTR_FLICK_POWR "          . Flicker power"		  		0.25 	0.0 8.0 0.01

#pragma parameter LABEL0014 " " 0.0 0.0 0.0 0.0


// pixel Grid
#pragma parameter DO_PIXELGRID		 "★  Low level Phosphor grid enable? (ph) ==>"	0.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_OVERMASK "      Overmask (1.0 = neutral)"	1.0 0.0 2.0 0.02
#pragma parameter LABEL0015 " " 0.0 0.0 0.0 0.0

#pragma parameter DO_PIXELGRID_H            "      ★ Scanlines (0 to disable)"    	1.0 0.0 1.25 0.05
#pragma parameter PIXELGRID_DOUBLESCAN      "         . Double-scan low input resolutions"  0.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_COREY_FAKE_SCAN "         . Fake integer scanlines (-1=auto)"  -1.0 -1.0 1.0 1.0
#pragma parameter PIXELGRID_MIN_H	        "         . Phosphors height Min"           0.2 0.00 1.0 0.05
#pragma parameter PIXELGRID_MAX_H           "         . Phosphors height Max"           0.6 0.05 1.0 0.05
#pragma parameter PIXELGRID_INFLATION       "         . Inflation Strength"              0.75  0.0 2.0 0.01

#pragma parameter PIXELGRID_GAMMA_H         "         . Phosphors height min->max gamma:"	4.2 1.0 8.00 0.1
#pragma parameter PIXELGRID_NO_INTERBLEED_H	"         . Inter line extra steepness (1.0 to disable)"	1.0 1.0 11.0 0.1
#pragma parameter PIXELGRID_H_ANTIMOIRE     "         . Anti-moire sweet spot (disables Y deconvergence)"  0.0 0.0 1.0 0.01
#pragma parameter PIXELGRID_OFFSET_CORE     "         . Slotmask(fake) offset (not with anti-moire) " 0.0 0.0 1.0 0.01
#pragma parameter PIXELGRID_DECON_R_H       "         . Deconvergence Y: R phosphor" 0.0 -1.6 1.6 0.1
#pragma parameter PIXELGRID_DECON_G_H       "         . Deconvergence Y: G phosphor" 0.0 -1.6 1.6 0.1
#pragma parameter PIXELGRID_DECON_B_H       "         . Deconvergence Y: B phosphor" 0.0   -1.6 1.6 0.1
#pragma parameter PIXELGRID_H_DEDOT			"         . Dedot mask between scanlines (use only if needed!)"	0.0 0.0 2.0 0.1

#pragma parameter LABEL0016 " " 0.0 0.0 0.0 0.0


#pragma parameter DO_PIXELGRID_W     "      ★ Horizontal mask (0 to disable)"    	1.0 0.0 1.0 0.05
#pragma parameter PIXELGRID_SIZE_W   "           . Resolution: (core or screen)"	1.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_MUL_X    "           . Cell size multiplier x (neg=divider):"	1.0 -8.0 8.0 0.125
#pragma parameter PIXELGRID_H_PRST   "           . Mask type preset (0: for manual)"  3.0 0.0 7.0 1.0
#pragma parameter LABEL0038          "             (1:gm 2:gmx 3:rgb 4:rgbx 5:rbg 6:rbgx 7:wx)"  0.0 0.0 0.0 0.0
#pragma parameter PIXELGRID_H_COUNT  "                . 0: Phosphors+gap count (mask size)"	3.0 1.0 4.0  1.0
#pragma parameter PIXELGRID_R_SHIFT  "                . 0: R phospor position"      0.0  0.0 4.0 0.1
#pragma parameter PIXELGRID_G_SHIFT  "                . 0: G phospor position"      1.0  0.0 4.0 0.1
#pragma parameter PIXELGRID_B_SHIFT  "                . 0: B phospor position"      2.0  0.0 4.0 0.1
#pragma parameter PIXELGRID_MIN_W    "           . Phosphors width Min"			0.25 0.05 1.0 0.01
#pragma parameter PIXELGRID_MAX_W    "           . Phosphors width Max"			0.25 0.05 1.0 0.01
#pragma parameter PIXELGRID_GAMMA_W  "           . Phosphors width min->max gamma:"	4.2 1.0 8.00 0.1

#pragma parameter PIXELGRID_BASAL_GRID  "           . Black level of the unexcided phosphor grid"	0.0 0.0 10.0 0.01
#pragma parameter LABEL0017 " " 0.0 0.0 0.0 0.0

#pragma parameter PIXELGRID_Y_MASK           "      ★ Vertical cell Mask (aperturegrille/slotmask)"	0.5 0.0 1.0 0.05
#pragma parameter PIXELGRID_Y_MASK_COORDS    "           . Resolution: (core or screen)"	1.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_Y_MASK_HEIGHT    "           . Height divider (neg = multiplier):"	0.5 -8.0 8.0 0.125
#pragma parameter PIXELGRID_Y_MASK_OFFSET    "           . Even/odd Y offset (slotmask)"	1.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_Y_MASK_SHIFT     "           . Vertical shift (neg = auto)"	-0.01 -0.01 1.0 0.01
#pragma parameter PIXELGRID_Y_MASK_STEEP     "           . Steepness"	2.0 1.0 30.0 1.0
#pragma parameter PIXELGRID_Y_MASK_ON_WHITE  "           . Fadeout under light"	0.5 0.0 2.0 0.01
#pragma parameter PIXELGRID_Y_SPARK          "           . Sparkling look punch"	0.0 0.0 8.0 0.1

#pragma parameter LABEL0018 " " 0.0 0.0 0.0 0.0

#pragma parameter PIXELGRID_DO_SHADOWMASK    "      ★ Shadowmask "	0.0 0.0 1.0 1.0
#pragma parameter PIXELGRID_SHADOWMASK_SHIFT "           . X staggering    "	1.0 0.5 4.0 0.5
#pragma parameter PIXELGRID_SHADOWMASK_H     "           . Phosphor height "	1.0 1.0 10.0 1.0


#pragma parameter LABEL0019 " " 0.0 0.0 0.0 0.0


// Dot matrix emulation
#pragma parameter DO_DOT_MATRIX		"★  Dot matrix emulation enable? (NOT with scanlines!) ==>"	0.0 0.0 1.0 1.0
#pragma parameter DOT_M_G_STR		"        Grid strength"					0.0 0.0 1.0 0.01
#pragma parameter DOT_M_G_TRESH		"           . Paint on bright (<0 on black)"	1.1 -1.1 1.1 0.01
#pragma parameter DOT_M_G_BRT		"           . Grid brightness"					0.0 0.0 1.0 0.01
#pragma parameter DOT_M_G_SHARP		"           . Grid sharpness (0.0 for auto)"	0.0 0.0 20.0 0.25
#pragma parameter DOT_M_MBLUR_STR	"        Refresh inertia"				0.45  0.0 0.9 0.05
#pragma parameter DOT_M_MBLUR_MODE  "           . Inertia on: both,white,black"	0.0  0.0 2.0 1.0
#pragma parameter DOT_M_SHADOW_STR  "        Shadow strength (disables deconvergence)"	0.0  0.0 1.0 0.01
#pragma parameter DOT_M_SHADOW_OFF  "           . Shadow offset"								0.0  -1.0 1.0 0.01
#pragma parameter DOT_M_MULTIPLY	"        Multiply factor (debug)"		1.0  1.0 50.0 1.0
#pragma parameter LABEL0020 " " 0.0 0.0 0.0 0.0




// Halo
#pragma parameter DO_HALO         "★ Halo enable? (ha) ==>" 						0.0	0.0 1.0 1.0
#pragma parameter HALO_NO_PREGAIN "        Pre-attenuate input signal gain to 1x"	0.0		0.0	1.0	1.0
#pragma parameter HALO_POWER      "        Strength (negative = 10x precision)"	0.9 	-50.0  5.0 0.01
#pragma parameter HALO_SHARPNESS  "        Sharpness"					3.00	0.05   7.0 0.05
#pragma parameter HALO_GAMMA      "        Gamma in, the higher, the less the halo on dark colors"	2.0 	1.0  10.0 0.05
#pragma parameter HALO_GAMMA_OUT  "        Gamma out"	 1.0	0.25  4.0 0.1
#pragma parameter HALO_VS_SCAN    "        Light up scanline gaps and dot grid gaps too"				0.0	0.0  1.0  0.01
#pragma parameter LABEL0021 " " 0.0 0.0 0.0 0.0



// Bloom
#pragma parameter DO_BLOOM           "★ Bloom enable? (bl) ==>" 						0.0  0.0  1.0  1.0
#pragma parameter BLOOM_MIX          "        Final mix, (0.0=off)"					0.45 0.0 1.0  0.01
#pragma parameter BLOOM_SIZE         "        Radius"							4.0  0.25 30.0 0.25
#pragma parameter BLOOM_QUALITY      "        Quality: more is better but slower"			1.0  1.0 8.0 1.0
#pragma parameter BLOOM_GAMMA        "        Gamma in (threshold)"					8.0  1.0 10.0 0.5
#pragma parameter BLOOM_GAMMA_OUT    "        Gamma out (contour smoothness)"				1.0  0.1 10.0 0.1
#pragma parameter BLOOM_POWER        "        Power multiplier"						1.20  1.0 20.0 0.05  
#pragma parameter BLOOM_EYE_ADPT_SRT "        Modulate: Local exposure eye adaption strength"		0.45   0.0 10.0 0.05
#pragma parameter BLOOM_EYE_INERTIA  "        Modulate: Time before eye adaption starts"		1500  10.0 10000 50.0
#pragma parameter BLOOM_OVER_WHITE   "        Modulate: Strength on bright areas (0 = aura)"		0.5  0.0 1.0  0.05
#pragma parameter BLOOM_BYPASS       "        Bypass/Solo (1=Unmodulated, 2=Modulated)"			0.0  0.0 2.0  1.0
#pragma parameter LABEL0022 " " 0.0 0.0 0.0 0.0



//Curvature parameters:
#pragma parameter DO_CURVATURE	     "★ Curvature enable? (cu) ==>"			0.0   0.0 1.0 1.0
#pragma parameter GEOM_WARP_X        "        Warp X"				0.28   0.0 6.0 0.01
#pragma parameter GEOM_WARP_Y        "        Warp Y"				0.32   0.0 6.0 0.01
#pragma parameter LABEL0054 " " 0.0 0.0 0.0 0.0


#pragma parameter LABEL_0055   "★ Corners/Edges"			1.0   1.0 1.0 0.0
#pragma parameter GEOM_CORNER_SIZE   "        Corners radius"			0.005  0.002 0.1 0.001
#pragma parameter GEOM_CORNER_SMOOTH "        Edges sharpness"			200.0 50.0 1000.0 10.0
#pragma parameter LABEL0023 " " 0.0 0.0 0.0 0.0


//Bezel related
#pragma parameter DO_BEZEL	      "★ Bezel enable? (be) ==>"				0.0   0.0 1.0 1.0
#pragma parameter BEZEL_USE_STRAIGHT  "        Straight"				0.0	0.0 1.0  1.0 
#pragma parameter BEZEL_INNER_ZOOM    "        Inner zoom (ignored if integer scaled)"	-0.18   -1.5 0.5 0.001
#pragma parameter BEZEL_FRAME_ZOOM    "        Frame zoom"				0.0   -1.5 0.5 0.001
#pragma parameter BEZEL_R             "        Color: Red"				-0.35   -1.0 1.0  0.005
#pragma parameter BEZEL_G             "        Color: Green"				-0.35  -1.0 1.0  0.005
#pragma parameter BEZEL_B             "        Color: Blue"				-0.35   -1.0 1.0  0.005
#pragma parameter BEZEL_CON           "        Contrast"				1.3   0.0  10.0 0.01
#pragma parameter BEZEL_RFL_ZOOM      "        Reflections zoom"			0.05   -1.5 0.5 0.01
#pragma parameter BEZEL_REFL_STRENGTH "        Reflections strength"			0.5    0.0 2.0 0.01
#pragma parameter BEZEL_RFL_BLR_SHD   "        Reflections sharpness"			-0.2  -1.5 0.8 0.01
#pragma parameter BEZEL_ROUGHNESS     "        Reflections roughness"			1.0    0.0 5.0 0.1
#pragma parameter BEZEL_DIFFUSION_STR "        Diffusion strength"             0.2     0.0 1.0 0.01
#pragma parameter BEZEL_SPCL_STRENGTH "        Specularity strength"			0.5    0.0 3.0 0.05
#pragma parameter BEZEL_LIGHT_NO_FADEOUT "        Light fadeout distance"		0.5    0.0 1.0 0.01
#pragma parameter BEZEL_CORNER_DARK   "        Darken corners"					0.8    0.0 2.0 0.05


#pragma parameter LABEL0024 " " 0.0 0.0 0.0 0.0



//Background Image
#pragma parameter DO_BG_IMAGE	     "★ Back/Foreground image enable? (ba) ==>"				 0.0  0.0  1.0     1.0
#pragma parameter LABEL0039     "   !! RetroArch aspect needs to be set to Full"	 1.0  1.0  1.0     1.0
#pragma parameter BG_IMAGE_OVER	     "      Image over content (alpha channel driven)?"	 0.0  0.0  1.0     1.0
#pragma parameter BG_IMAGE_OFFX	     "      Shift Image over X axis"				 0.0  -1.0  1.0    0.0005
#pragma parameter BG_IMAGE_OFFY	     "      Shift Image over Y axis"				 0.0  -1.0  1.0    0.0005
#pragma parameter BG_IMAGE_ZOOM	     "      Zoom Image"					 1.0  -1.0  3.0    0.0005
#pragma parameter BG_IMAGE_ROTATION  "      Rotate/flip (0=auto,1/-1=no change,<0 for rotated games)"  0.0  -8.0  8.0    1.0
#pragma parameter BG_IMAGE_NIGHTIFY  "      Nightify image"                                 0.0   0.0  1.0    0.1
#pragma parameter BG_IMAGE_WRAP_MODE "      Wrap mode: default, clamp to border, edge, repeat" 0.0  0.0    3.0    1.0
#pragma parameter LABEL0025 " " 0.0 0.0 0.0 0.0

//Backdrop image
#pragma parameter DO_BACKDROP		"★ Backdrop mirrored image enable? ==>"		 0.0  0.0  1.0     1.0
#pragma parameter BACKDROP_OFFX		"        Shift backdrop over X axis"				 0.0  -1.0  1.0    0.0005
#pragma parameter BACKDROP_OFFY 	"        Shift backdrop over X axis"				 0.0  -1.0  1.0    0.0005
#pragma parameter BACKDROP_ZOOM 	"        Zoom backdrop"					 1.0  -1.0  3.0    0.0005
#pragma parameter LABEL0026 " " 0.0 0.0 0.0 0.0


//Back leds
//Emulates leds under the monitor frame that slowly reacts to image contents
#pragma parameter DO_AMBILIGHT	            "★ Ambient light leds enable? (am) ==> "				1.0  0.0  1.0     1.0
#pragma parameter LABEL0040            "    !! RetroArch aspect needs to be set to Full"		1.0  1.0  1.0     1.0
#pragma parameter AMBI_STEPS	            "        Slowness"						60.0 5.0  1000.0  5.0
#pragma parameter AMBI_FALLOFF              "        Led power/falloff"			0.4  0.1  3.0     0.01
#pragma parameter AMBI_POWER                "        Led Saturation"						1.8  0.0  100.0     0.05
#pragma parameter AMBI_INT_OFFSET           "        Internalness (Led position)"      0.07 -1.0 1.0 0.01
#pragma parameter AMBI_SMP_INT_OFFSET       "        Internalness (Sampling point) "   0.07 0.07 1.0 0.01 
#pragma parameter AMBI_STRETCH              "        Widen lights (horizontal games)"	0.0  0.0  1.0     0.01
#pragma parameter AMBI_STRETCH_VERTICAL     "        Widen lights (vertical games)"		0.35  0.0  1.0     0.01
#pragma parameter AMBI_GAMMA                "        Output Gamma"						1.3  0.1  8.0     0.1
#pragma parameter AMBI_OVER_BEZEL           "        Bezel Colorization intensity"		0.2  0.0  5.0     0.05
#pragma parameter AMBI_OVER_BEZEL_SIZE      "        Bezel Colorization size"			0.15 0.001  0.5   0.001 ///hacky, but keep min to 0.001 plz.
#pragma parameter AMBI_BG_IMAGE_BLEND_MODE  "        Back/Foreground image alpha blend (0=mix, 1=add)"	0.0   0.0  1.0    1.0
#pragma parameter AMBI_ADD_ON_BLACK         "        Foggy (power on dark colors, add mode only)    "		1.0   0.0  1.0    0.05
#pragma parameter AMBI_BG_IMAGE_FORCE       "        Always colorize Back/Foreground image (add mode only) "		0.0   0.0  1.0    0.1
#pragma parameter LABEL0027 " " 0.0 0.0 0.0 0.0



//Vignette and spot
#pragma parameter DO_VIGNETTE	"★ Vignette enable? (vi) ==>"	0.0  0.0  1.0  1.0
#pragma parameter V_SIZE	"        Size"			0.7  0.3  2.0  0.02
#pragma parameter V_POWER	"        Power"			1.0  0.05 1.0  0.01

#pragma parameter DO_SPOT	"★ Spot enable? (sp) ==>"		0.0   0.0   1.0 1.0
#pragma parameter S_POSITION_X 	"        X Position"	   -0.5   -2.0 2.0 0.05
#pragma parameter S_POSITION_Y 	"        Y Position"		0.5   -2.0 2.0 0.05
#pragma parameter S_SIZE	"        Size"			0.36    0.0   3.0 0.01
#pragma parameter S_POWER	"        Power" 		0.11   0.01  1.0 0.01
#pragma parameter LABEL0028 " " 0.0 0.0 1.0 1.0



//Global Zoom/Offset
#pragma parameter DO_GLOBAL_SHZO	 "★ Global shift/zoom enable? (zo) ==>"	0.0   0.0  1.0 	  1.0
#pragma parameter GLOBAL_OFFX	 "        X axis shift "			0.0  -1.0  1.0    0.0005
#pragma parameter GLOBAL_OFFY	 "        Y axis shift "			0.0  -1.0  1.0    0.0005
#pragma parameter GLOBAL_ZOOM	 "        Zoom"					2.0   0.0  3.0    0.0005
#pragma parameter LABEL0029 " " 0.0 0.0 1.0 1.0



//Aspect Ratio
#pragma parameter label_ar	 "★ Aspect Ratio (as) "							1.0  1.0  1.0     0.0
#pragma parameter ASPECT_X       "        Aspect Ratio Numerator (<=0 for a preset)"   		0.0  -6.0 256. 1.0
#pragma parameter ASPECT_Y       "        Aspect Ratio Denominator"	  				3.0   0.0 256. 1.0
#pragma parameter LABEL0042 " " 0.0 0.0 0.0 0.0
#pragma parameter LABEL0043 "        Presets reference list:" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0044 "          (0 =  MAME 1.33)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0045 "          (-1 = NTSC 1.5)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0046 "          (-2 = PAL 1.25)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0047 "          (-3 = Snes 8/7)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0048 "          (-4 = Megadrive 10/7)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0049 "          (-5 = Uncorrected)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0050 "          (-6 = MAME rotated/TATE 0.75)" 0.0 0.0 0.0 1.0
#pragma parameter LABEL0051 " " 0.0 0.0 0.0 0.0

//Modulate the image zooming depending on the image luminosity.
#pragma parameter DO_DYNZOOM	     "★ Luminosity tied zoom (!!OBSTRUCTS integer scale!!) ==>"		0.015   0.0   1.0  0.005
#pragma parameter LABEL0032 " " 0.0 0.0 0.0 0.0

//Autocrop
#pragma parameter AUTOCROP_MAX	     "★ Autocrop: maximum amount (!!OBSTRUCTS integer scale!!) ==>"   0.0   0.0   0.5  0.01
#pragma parameter AUTOCROP_MIN	            "        Number of mandatory lines to crop"   1.0   0.0   10.0  1.0
#pragma parameter AUTOCROP_SAMPLES          "        Samples per frame (faster response, higher gpu use)"     20.0   0.0   300.0 1.0
#pragma parameter AUTOCROP_SAMPLE_SIZE      "        Sample size (big speedup, less accurate)" 2.0 0.0 5.0 1.0  
#pragma parameter AUTOCROP_STEADINESS       "        Scene change treshold (0 = continuous cropping)"     0.2   0.0   0.5 0.01
#pragma parameter AUTOCROP_TRANSITION_SPEED "        Transition speed"    		 0.1   0.05   1.0 0.05
#pragma parameter LABEL0033 " " 0.0 0.0 0.0 0.0

//Content geomerty overrides
#pragma parameter DO_GAME_GEOM_OVERRIDE	  "★ Override content geometry enable? (ov)==>"	0.0   0.0  1.0 	  1.0
#pragma parameter GAME_GEOM_INT_SCALE     "        Integer scale (disables inner zoom) 2=No keep aspect"	0.0   0.0  2.0    1.0
#pragma parameter GAME_GEOM_INT_SCALE_MAX "        .    Maximum integer scale" 			10.0  1.0  100.0  1.0
#pragma parameter GAME_GEOM_INT_SCALE_OVR "        .    Permit integer overscale by" 			1.04   1.00  1.1  0.01
#pragma parameter LABEL0052           "    !! The following will override integer scale" 1.0  1.0  1.0     1.0
#pragma parameter GAME_GEOM_ASPECT	  "        Aspect (0 = unchanged)"		0.0    0.0  3.0	  0.005
#pragma parameter GAME_GEOM_VSHIFT	  "        Vertical position"			0.0   -10.0 10.0  0.01
#pragma parameter GAME_GEOM_HSHIFT	  "        Horizontal position"			0.0   -10.0 10.0  0.01
#pragma parameter GAME_GEOM_ZOOM	  "        Zoom"					1.0    0.0  2.0  0.01
#pragma parameter GAME_GEOM_OFF_FIX	  "        Sharp hack through offset (0.0=disable)"	0.0    0.0  100.0  1
#pragma parameter LABEL0034 " " 0.0 0.0 0.0 0.0


//Tilt
#pragma parameter DO_TILT	  	"★ Tilt enable? (ti) ==>"				0.0   0.0  1.0 	  1.0
#pragma parameter TILT_X	  	"        Tilt along X axis"		0.0    -0.5  0.5  0.01
#pragma parameter TILT_FOV	  	"        Fov"					0.7    0.1	1.5   0.05
#pragma parameter TILT_BEZEL_K	"        Bezel multiplier"		1.0    0.0  1.5  0.01
#pragma parameter LABEL0035 " " 0.0 0.0 0.0 0.0

//Delta render
#pragma parameter LABEL0053    "  !THE FOLLOWING FEATURES ARE STATIC, SEE docs-ng.md TO ENABLE"    1.0 1.0 1.0 1.0
#pragma parameter DELTA_RENDER_FORCE_REFRESH    "★ Delta render force refresh interval ? ==>"	4.0 1.0 3600.0 1.0
#pragma parameter DELTA_RENDER_CHECK_AREA       "        Delta render area size"			1.0 0.0 10.0 1.0
#pragma parameter LABEL0041 " " 0.0 0.0 1.0 1.0

#pragma parameter SERVICE1 "Don't use me, for debug purposes only" 	0.5 0.0 1.0 0.01

// This is for a patched retroarch version I'm testing (24/03/04)
#pragma inject_preset_code DEFINES_FROM_PRESET

//User parameters goes here, they overrides almost everything:
#include "../config/config-user.txt"

// The following translates global.PARAMETER to PARAMETER
#include "config.globals.inc"


// 2014-Jan-25
// Since we moved some mandatory defines from config-static.inc to config-user.txt
// It is possible that users overriding config-user.txt will run into errors
// if they don't check for updated config-user.
// Brave users don't deserve any headaches, so put some safety checks here.

#ifndef DELTA_RENDER
	#define DELTA_RENDER 0.0 
#endif

#ifndef HALVE_BORDER_UPDATE
	#define HALVE_BORDER_UPDATE 0.0 
#endif

#ifndef SKIP_RANDOM
	#define SKIP_RANDOM 0.0 
#endif

