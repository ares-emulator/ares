//Wrap any translation into an ifndef so that we can override them with a static include.


#ifndef DO_CCORRECTION
    #define DO_CCORRECTION	global.DO_CCORRECTION
#endif

#ifndef GAMMA_OUT
    #define GAMMA_OUT	global.GAMMA_OUT
#endif

#ifndef SATURATION
    #define SATURATION	global.SATURATION
#endif

#ifndef VIBRANCE
    #define VIBRANCE	global.VIBRANCE
#endif

#ifndef LUMINANCE
    #define LUMINANCE	global.LUMINANCE
#endif

#ifndef CONTRAST
    #define CONTRAST	global.CONTRAST
#endif

#ifndef ADAPTIVE_BLACK
    #define ADAPTIVE_BLACK  global.ADAPTIVE_BLACK
#endif

#ifndef BRIGHTNESS
    #define BRIGHTNESS	global.BRIGHTNESS
#endif

#ifndef TEMPERATURE
    #define TEMPERATURE	global.TEMPERATURE
#endif

#ifndef COLOR_MONO_COLORIZE
    #define COLOR_MONO_COLORIZE 	global.COLOR_MONO_COLORIZE
#endif

#ifndef COLOR_MONO_HUE1
    #define COLOR_MONO_HUE1		global.COLOR_MONO_HUE1
#endif

#ifndef COLOR_MONO_HUE2
    #define COLOR_MONO_HUE2		global.COLOR_MONO_HUE2
#endif

#ifndef COLOR_MONO_HUE_BIAS
    #define COLOR_MONO_HUE_BIAS global.COLOR_MONO_HUE_BIAS
#endif

#ifndef DO_FXAA
    #define DO_FXAA	global.DO_FXAA
#endif

#ifndef DO_DEDITHER
    #define DO_DEDITHER global.DO_DEDITHER
#endif

#ifndef DEDITHER_STRENGTH
    #define DEDITHER_STRENGTH global.DEDITHER_STRENGTH
#endif

#ifndef DEDITHER_STRENGTH2
    #define DEDITHER_STRENGTH2 global.DEDITHER_STRENGTH2
#endif

#ifndef DEDITHER_TRESHMAX
    #define DEDITHER_TRESHMAX global.DEDITHER_TRESHMAX
#endif

#ifndef DO_NTSC_ARTIFACTS
    #define DO_NTSC_ARTIFACTS	global.DO_NTSC_ARTIFACTS
#endif

#ifndef NTSC_FILTER_WIDTH
    #define NTSC_FILTER_WIDTH		global.NTSC_FILTER_WIDTH
#endif

#ifndef NTSC_FILTER_SCF
    #define NTSC_FILTER_SCF			global.NTSC_FILTER_SCF
#endif

#ifndef NTSC_FILTER_FC
    #define NTSC_FILTER_FC			global.NTSC_FILTER_FC
#endif

#ifndef NTSC_MIX
    #define NTSC_MIX				global.NTSC_MIX
#endif

#ifndef NTSC_PHASE_SHIFT
    #define NTSC_PHASE_SHIFT		global.NTSC_PHASE_SHIFT
#endif

#ifndef NTSC_ARTF_NOBLUR
    #define NTSC_ARTF_NOBLUR 		global.NTSC_ARTF_NOBLUR
#endif

#ifndef NTSC_ARTF_NOBLEED
    #define NTSC_ARTF_NOBLEED 		global.NTSC_ARTF_NOBLEED
#endif

#ifndef NTSC_ARTF_TRSH
    #define NTSC_ARTF_TRSH 			global.NTSC_ARTF_TRSH
#endif

#ifndef NTSC_SHOW_ARTF_MASK
    #define NTSC_SHOW_ARTF_MASK 	global.NTSC_SHOW_ARTF_MASK
#endif

#ifndef NTSC_ARTF_ZAP
    #define NTSC_ARTF_ZAP			global.NTSC_ARTF_ZAP
#endif

#ifndef DO_RF_NOISE
    #define DO_RF_NOISE		global.DO_RF_NOISE
#endif

#ifndef RF_NOISE_STRENGTH
    #define RF_NOISE_STRENGTH	global.RF_NOISE_STRENGTH
#endif

#ifndef DO_SAT_BLEED
    #define DO_SAT_BLEED		global.DO_SAT_BLEED
#endif

#ifndef SAT_BLEED_SIZE
    #define SAT_BLEED_SIZE	global.SAT_BLEED_SIZE
#endif

#ifndef SAT_BLEED_FALLOFF
    #define SAT_BLEED_FALLOFF	global.SAT_BLEED_FALLOFF
#endif

#ifndef SAT_BLEED_STRENGTH
    #define SAT_BLEED_STRENGTH	global.SAT_BLEED_STRENGTH
#endif

#ifndef SAT_BLEED_PAL
    #define SAT_BLEED_PAL		global.SAT_BLEED_PAL
#endif

#ifndef DO_SHIFT_RGB
    #define DO_SHIFT_RGB		global.DO_SHIFT_RGB
#endif

#ifndef SHIFT_R
    #define SHIFT_R			global.SHIFT_R
#endif

#ifndef SHIFT_G
    #define SHIFT_G			global.SHIFT_G
#endif

#ifndef SHIFT_B
    #define SHIFT_B			global.SHIFT_B
#endif

#ifndef OFFSET_STRENGTH
    #define OFFSET_STRENGTH	global.OFFSET_STRENGTH
#endif

#ifndef DO_IN_GLOW
    #define DO_IN_GLOW		global.DO_IN_GLOW
#endif

#ifndef IN_GLOW_BIAS
    #define IN_GLOW_BIAS			global.IN_GLOW_BIAS
#endif

#ifndef IN_GLOW_SPREAD
    #define IN_GLOW_SPREAD			global.IN_GLOW_SPREAD
    
#endif

#ifndef IN_GLOW_SPREAD_SHARPNESS
    #define IN_GLOW_SPREAD_SHARPNESS    global.IN_GLOW_SPREAD_SHARPNESS
#endif

#ifndef IN_GLOW_W
    #define IN_GLOW_W				global.IN_GLOW_W
#endif

#ifndef IN_GLOW_H
    #define IN_GLOW_H				global.IN_GLOW_H
#endif

#ifndef IN_GLOW_POWER
    #define IN_GLOW_POWER			global.IN_GLOW_POWER
#endif

#ifndef IN_GLOW_GAMMA
    #define IN_GLOW_GAMMA			global.IN_GLOW_GAMMA
#endif

#ifndef IN_GLOW_NTSC_ARTF_MULT
    #define IN_GLOW_NTSC_ARTF_MULT	global.IN_GLOW_NTSC_ARTF_MULT
#endif

#ifndef IN_GLOW_NTSC_ARTF_TRSH
    #define IN_GLOW_NTSC_ARTF_TRSH	global.IN_GLOW_NTSC_ARTF_TRSH
#endif

#ifndef IN_GLOW_SHOW_ARTF_MASK
    #define IN_GLOW_SHOW_ARTF_MASK  global.IN_GLOW_SHOW_ARTF_MASK
#endif

#ifndef DO_HALO
    #define DO_HALO		global.DO_HALO
#endif

#ifndef HALO_SHARPNESS
    #define HALO_SHARPNESS	params.HALO_SHARPNESS
#endif

#ifndef HALO_NO_PREGAIN
    #define HALO_NO_PREGAIN params.HALO_NO_PREGAIN
#endif

#ifndef HALO_POWER
    #define HALO_POWER	params.HALO_POWER
#endif

#ifndef HALO_GAMMA
    #define HALO_GAMMA	params.HALO_GAMMA
#endif

#ifndef HALO_VS_SCAN
    #define HALO_VS_SCAN	params.HALO_VS_SCAN
#endif

#ifndef HALO_GAMMA_OUT
    #define HALO_GAMMA_OUT	params.HALO_GAMMA_OUT
#endif

#ifndef DO_BLOOM
    #define DO_BLOOM		params.DO_BLOOM
#endif

#ifndef BLOOM_MIX
    #define BLOOM_MIX		global.BLOOM_MIX
#endif

#ifndef BLOOM_EYE_ADPT_SRT
    #define BLOOM_EYE_ADPT_SRT	global.BLOOM_EYE_ADPT_SRT
#endif

#ifndef BLOOM_EYE_INERTIA
    #define BLOOM_EYE_INERTIA	global.BLOOM_EYE_INERTIA
#endif

#ifndef BLOOM_QUALITY
    #define BLOOM_QUALITY		params.BLOOM_QUALITY
#endif

#ifndef BLOOM_SIZE
    #define BLOOM_SIZE		global.BLOOM_SIZE
#endif

#ifndef BLOOM_GAMMA
    #define BLOOM_GAMMA		global.BLOOM_GAMMA
#endif

#ifndef BLOOM_POWER
    #define BLOOM_POWER		global.BLOOM_POWER
#endif

#ifndef BLOOM_GAMMA_OUT
    #define BLOOM_GAMMA_OUT		global.BLOOM_GAMMA_OUT
#endif

#ifndef BLOOM_OVER_WHITE
    #define BLOOM_OVER_WHITE	global.BLOOM_OVER_WHITE
#endif

#ifndef BLOOM_BYPASS
    #define BLOOM_BYPASS		global.BLOOM_BYPASS
#endif

#ifndef DO_CURVATURE
    #define DO_CURVATURE		global.DO_CURVATURE
#endif

#ifndef GEOM_WARP_X
    #define GEOM_WARP_X		global.GEOM_WARP_X
#endif

#ifndef GEOM_WARP_Y
    #define GEOM_WARP_Y		global.GEOM_WARP_Y
#endif

#ifndef GEOM_CORNER_SIZE
    #define GEOM_CORNER_SIZE	global.GEOM_CORNER_SIZE
#endif

#ifndef GEOM_CORNER_SMOOTH
    #define GEOM_CORNER_SMOOTH	global.GEOM_CORNER_SMOOTH
#endif

#ifndef DO_BEZEL
    #define DO_BEZEL		global.DO_BEZEL
#endif

#ifndef BEZEL_R
    #define BEZEL_R			global.BEZEL_R
#endif

#ifndef BEZEL_G
    #define BEZEL_G			global.BEZEL_G
#endif

#ifndef BEZEL_B
    #define BEZEL_B			global.BEZEL_B
#endif

#ifndef BEZEL_CON
    #define BEZEL_CON		global.BEZEL_CON
#endif

#ifndef BEZEL_INNER_ZOOM
    #define BEZEL_INNER_ZOOM	global.BEZEL_INNER_ZOOM
#endif

#ifndef BEZEL_FRAME_ZOOM
    #define BEZEL_FRAME_ZOOM	global.BEZEL_FRAME_ZOOM
#endif

#ifndef BEZEL_USE_STRAIGHT
    #define BEZEL_USE_STRAIGHT	global.BEZEL_USE_STRAIGHT
#endif

#ifndef BEZEL_RFL_BLR_SHD
    #define BEZEL_RFL_BLR_SHD	global.BEZEL_RFL_BLR_SHD
#endif

#ifndef BEZEL_RFL_ZOOM
    #define BEZEL_RFL_ZOOM		global.BEZEL_RFL_ZOOM
#endif

#ifndef BEZEL_REFL_STRENGTH
    #define BEZEL_REFL_STRENGTH	global.BEZEL_REFL_STRENGTH
#endif

#ifndef BEZEL_SPCL_STRENGTH
    #define BEZEL_SPCL_STRENGTH	global.BEZEL_SPCL_STRENGTH
#endif

#ifndef BEZEL_DIFFUSION_STR
    #define BEZEL_DIFFUSION_STR global.BEZEL_DIFFUSION_STR
#endif

#ifndef BEZEL_LIGHT_NO_FADEOUT
    #define BEZEL_LIGHT_NO_FADEOUT global.BEZEL_LIGHT_NO_FADEOUT
#endif

#ifndef BEZEL_CORNER_DARK
    #define BEZEL_CORNER_DARK   global.BEZEL_CORNER_DARK
#endif

#ifndef BEZEL_ROUGHNESS
    #define BEZEL_ROUGHNESS		global.BEZEL_ROUGHNESS
#endif

#ifndef DO_SPOT
    #define DO_SPOT		global.DO_SPOT
#endif

#ifndef S_POSITION_X
    #define S_POSITION_X	global.S_POSITION_X
#endif

#ifndef S_POSITION_Y
    #define S_POSITION_Y	global.S_POSITION_Y
#endif

#ifndef S_SIZE
    #define S_SIZE		global.S_SIZE
#endif

#ifndef S_POWER
    #define S_POWER		global.S_POWER
#endif

#ifndef DO_VIGNETTE
    #define DO_VIGNETTE	global.DO_VIGNETTE
#endif

#ifndef V_SIZE
    #define V_SIZE		global.V_SIZE
#endif

#ifndef V_POWER
    #define V_POWER		global.V_POWER
#endif

#ifndef DO_BG_IMAGE
    #define DO_BG_IMAGE		global.DO_BG_IMAGE
#endif

#ifndef BG_IMAGE_OVER
    #define BG_IMAGE_OVER		global.BG_IMAGE_OVER
#endif

#ifndef BG_IMAGE_OFFY
    #define BG_IMAGE_OFFY		global.BG_IMAGE_OFFY
#endif

#ifndef BG_IMAGE_OFFX
    #define BG_IMAGE_OFFX		global.BG_IMAGE_OFFX
#endif

#ifndef BG_IMAGE_ZOOM
    #define BG_IMAGE_ZOOM		global.BG_IMAGE_ZOOM
#endif

#ifndef BG_IMAGE_ROTATION
    #define BG_IMAGE_ROTATION	global.BG_IMAGE_ROTATION
#endif

#ifndef BG_IMAGE_WRAP_MODE
    #define BG_IMAGE_WRAP_MODE	global.BG_IMAGE_WRAP_MODE
#endif

#ifndef BG_IMAGE_NIGHTIFY
    #define BG_IMAGE_NIGHTIFY   global.BG_IMAGE_NIGHTIFY
#endif

#ifndef DO_BACKDROP
    #define DO_BACKDROP		global.DO_BACKDROP
#endif

#ifndef BACKDROP_OFFX
    #define BACKDROP_OFFX		global.BACKDROP_OFFX
#endif

#ifndef BACKDROP_OFFY
    #define BACKDROP_OFFY		global.BACKDROP_OFFY
#endif

#ifndef BACKDROP_ZOOM
    #define BACKDROP_ZOOM		global.BACKDROP_ZOOM
#endif

#ifndef DO_AMBILIGHT
    #define DO_AMBILIGHT		global.DO_AMBILIGHT
#endif

#ifndef AMBI_STEPS
    #define AMBI_STEPS			global.AMBI_STEPS
#endif

#ifndef AMBI_FALLOFF
    #define AMBI_FALLOFF		global.AMBI_FALLOFF
#endif

#ifndef AMBI_POWER
    #define AMBI_POWER			global.AMBI_POWER
#endif

#ifndef AMBI_GAMMA
    #define AMBI_GAMMA			global.AMBI_GAMMA
#endif

#ifndef AMBI_STRETCH
    #define AMBI_STRETCH		params.AMBI_STRETCH
#endif

#ifndef AMBI_STRETCH_VERTICAL
    #define AMBI_STRETCH_VERTICAL 	params.AMBI_STRETCH_VERTICAL
#endif

#ifndef AMBI_OVER_BEZEL
    #define AMBI_OVER_BEZEL			global.AMBI_OVER_BEZEL
#endif

#ifndef AMBI_OVER_BEZEL_SIZE
    #define AMBI_OVER_BEZEL_SIZE		global.AMBI_OVER_BEZEL_SIZE
#endif

#ifndef AMBI_BG_IMAGE_BLEND_MODE
    #define AMBI_BG_IMAGE_BLEND_MODE	global.AMBI_BG_IMAGE_BLEND_MODE
#endif

#ifndef AMBI_BG_IMAGE_FORCE
    #define AMBI_BG_IMAGE_FORCE		global.AMBI_BG_IMAGE_FORCE
#endif

#ifndef AMBI_ADD_ON_BLACK
    #define AMBI_ADD_ON_BLACK		params.AMBI_ADD_ON_BLACK
#endif

#ifndef AMBI_INT_OFFSET
    #define AMBI_INT_OFFSET			params.AMBI_INT_OFFSET
#endif

#ifndef AMBI_SMP_INT_OFFSET
    #define AMBI_SMP_INT_OFFSET params.AMBI_SMP_INT_OFFSET
#endif

#ifndef ASPECT_X
    #define ASPECT_X		global.ASPECT_X
#endif

#ifndef ASPECT_Y
    #define ASPECT_Y		global.ASPECT_Y
#endif

#ifndef DO_DYNZOOM
    #define DO_DYNZOOM		global.DO_DYNZOOM
#endif

#ifndef DO_GLOBAL_SHZO
    #define DO_GLOBAL_SHZO	global.DO_GLOBAL_SHZO
#endif

#ifndef GLOBAL_ZOOM
    #define GLOBAL_ZOOM	global.GLOBAL_ZOOM
#endif

#ifndef GLOBAL_OFFX
    #define GLOBAL_OFFX	global.GLOBAL_OFFX
#endif

#ifndef GLOBAL_OFFY
    #define GLOBAL_OFFY	global.GLOBAL_OFFY
#endif

#ifndef DO_GAME_GEOM_OVERRIDE
    #define DO_GAME_GEOM_OVERRIDE	global.DO_GAME_GEOM_OVERRIDE
#endif

#ifndef GAME_GEOM_INT_SCALE
    #define GAME_GEOM_INT_SCALE	global.GAME_GEOM_INT_SCALE
#endif

#ifndef GAME_GEOM_INT_SCALE_OVR
    #define GAME_GEOM_INT_SCALE_OVR global.GAME_GEOM_INT_SCALE_OVR
#endif

#ifndef GAME_GEOM_INT_SCALE_MAX
    #define GAME_GEOM_INT_SCALE_MAX global.GAME_GEOM_INT_SCALE_MAX
#endif

#ifndef GAME_GEOM_ASPECT
    #define GAME_GEOM_ASPECT 	global.GAME_GEOM_ASPECT
#endif

#ifndef GAME_GEOM_VSHIFT
    #define GAME_GEOM_VSHIFT 	global.GAME_GEOM_VSHIFT
#endif

#ifndef GAME_GEOM_HSHIFT
    #define GAME_GEOM_HSHIFT 	global.GAME_GEOM_HSHIFT
#endif

#ifndef GAME_GEOM_ZOOM
    #define GAME_GEOM_ZOOM		global.GAME_GEOM_ZOOM
#endif

#ifndef GAME_GEOM_OFF_FIX
    #define GAME_GEOM_OFF_FIX   global.GAME_GEOM_OFF_FIX
#endif

#ifndef DO_TILT
    #define DO_TILT			global.DO_TILT
#endif

#ifndef TILT_X
    #define TILT_X			global.TILT_X
#endif

#ifndef TILT_FOV
    #define TILT_FOV		global.TILT_FOV
#endif

#ifndef TILT_BEZEL_K
    #define TILT_BEZEL_K	global.TILT_BEZEL_K
#endif

#ifndef AUTOCROP_MAX
    #define AUTOCROP_MAX global.AUTOCROP_MAX
#endif

#ifndef AUTOCROP_MIN
    #define AUTOCROP_MIN global.AUTOCROP_MIN
#endif

#ifndef AUTOCROP_SAMPLES
    #define AUTOCROP_SAMPLES global.AUTOCROP_SAMPLES
#endif

#ifndef AUTOCROP_SAMPLE_SIZE
    #define AUTOCROP_SAMPLE_SIZE global.AUTOCROP_SAMPLE_SIZE
#endif


#ifndef AUTOCROP_TRANSITION_SPEED
	#define AUTOCROP_TRANSITION_SPEED global.AUTOCROP_TRANSITION_SPEED
#endif

#ifndef AUTOCROP_STEADINESS
	#define AUTOCROP_STEADINESS global.AUTOCROP_STEADINESS
#endif


#ifndef TATE
    #define TATE	global.TATE
#endif

#ifndef RESSWITCH_GLITCH_SIZE
    #define RESSWITCH_GLITCH_SIZE	global.RESSWITCH_GLITCH_SIZE
#endif

#ifndef DO_DOT_MATRIX
    #define DO_DOT_MATRIX	global.DO_DOT_MATRIX
#endif

#ifndef DOT_M_G_SHARP
    #define DOT_M_G_SHARP	global.DOT_M_G_SHARP
#endif

#ifndef DOT_M_RGB_SHARP
    #define DOT_M_RGB_SHARP	global.DOT_M_RGB_SHARP
#endif

#ifndef DOT_M_RGB_STR
    #define DOT_M_RGB_STR	global.DOT_M_RGB_STR
#endif

#ifndef DOT_M_G_TRESH
    #define DOT_M_G_TRESH	global.DOT_M_G_TRESH
#endif

#ifndef DOT_M_G_BRT
    #define DOT_M_G_BRT		global.DOT_M_G_BRT
#endif

#ifndef DOT_M_G_GAIN
    #define DOT_M_G_GAIN	global.DOT_M_G_GAIN
#endif

#ifndef DOT_M_G_STR
    #define DOT_M_G_STR		global.DOT_M_G_STR
#endif

#ifndef DOT_M_MBLUR_STR
    #define DOT_M_MBLUR_STR		global.DOT_M_MBLUR_STR
#endif

#ifndef DOT_M_MBLUR_MODE
    #define DOT_M_MBLUR_MODE	global.DOT_M_MBLUR_MODE
#endif

#ifndef DOT_M_MULTIPLY
    #define DOT_M_MULTIPLY  	global.DOT_M_MULTIPLY
#endif

#ifndef DOT_M_SHADOW_STR
    #define DOT_M_SHADOW_STR	global.DOT_M_SHADOW_STR
#endif

#ifndef DOT_M_SHADOW_OFF
    #define DOT_M_SHADOW_OFF	global.DOT_M_SHADOW_OFF
#endif

#ifndef MIN_LINES_INTERLACED
    #define MIN_LINES_INTERLACED global.MIN_LINES_INTERLACED
#endif

#ifndef DO_PIXELGRID
    #define DO_PIXELGRID			global.DO_PIXELGRID
#endif

#ifndef PIXELGRID_OVERMASK
    #define PIXELGRID_OVERMASK	global.PIXELGRID_OVERMASK
#endif

#ifndef DO_PIXELGRID_W
    #define DO_PIXELGRID_W		global.DO_PIXELGRID_W
#endif

#ifndef DO_PIXELGRID_H
    #define DO_PIXELGRID_H				global.DO_PIXELGRID_H
#endif

#ifndef PIXELGRID_H_ANTIMOIRE
	#define PIXELGRID_H_ANTIMOIRE  global.PIXELGRID_H_ANTIMOIRE
#endif
	
#ifndef PIXELGRID_COREY_FAKE_SCAN
    #define PIXELGRID_COREY_FAKE_SCAN 	global.PIXELGRID_COREY_FAKE_SCAN
#endif

#ifndef PIXELGRID_DOUBLESCAN
    #define PIXELGRID_DOUBLESCAN 	global.PIXELGRID_DOUBLESCAN
#endif

#ifndef	PIXELGRID_INFLATION
	#define PIXELGRID_INFLATION global.PIXELGRID_INFLATION
#endif

#ifndef PIXELGRID_MIN_H
    #define PIXELGRID_MIN_H 			global.PIXELGRID_MIN_H
#endif

#ifndef PIXELGRID_MIN_W
    #define PIXELGRID_MIN_W 	global.PIXELGRID_MIN_W
#endif

#ifndef PIXELGRID_MAX_H
    #define PIXELGRID_MAX_H	 	global.PIXELGRID_MAX_H
#endif

#ifndef PIXELGRID_MAX_W
    #define PIXELGRID_MAX_W 	global.PIXELGRID_MAX_W
#endif

#ifndef PIXELGRID_SIZE_W
    #define PIXELGRID_SIZE_W 	global.PIXELGRID_SIZE_W
#endif

#ifndef PIXELGRID_GAMMA_H
    #define PIXELGRID_GAMMA_H	global.PIXELGRID_GAMMA_H
#endif

#ifndef PIXELGRID_MUL_X
    #define PIXELGRID_MUL_X 	global.PIXELGRID_MUL_X
#endif

#ifndef PIXELGRID_NO_INTERBLEED_H
    #define PIXELGRID_NO_INTERBLEED_H global.PIXELGRID_NO_INTERBLEED_H
#endif

#ifndef PIXELGRID_H_DEDOT
    #define PIXELGRID_H_DEDOT	global.PIXELGRID_H_DEDOT
#endif

#ifndef PIXELGRID_BASAL_GRID
    #define PIXELGRID_BASAL_GRID global.PIXELGRID_BASAL_GRID
#endif

#ifndef PIXELGRID_DECON_R_H
    #define PIXELGRID_DECON_R_H		global.PIXELGRID_DECON_R_H
#endif

#ifndef PIXELGRID_DECON_G_H
    #define PIXELGRID_DECON_G_H		global.PIXELGRID_DECON_G_H
#endif

#ifndef PIXELGRID_DECON_B_H
    #define PIXELGRID_DECON_B_H		global.PIXELGRID_DECON_B_H
#endif

#ifndef PIXELGRID_Y_MASK
    #define PIXELGRID_Y_MASK	global.PIXELGRID_Y_MASK
#endif

#ifndef PIXELGRID_Y_MASK_HEIGHT
    #define PIXELGRID_Y_MASK_HEIGHT	global.PIXELGRID_Y_MASK_HEIGHT
#endif

#ifndef PIXELGRID_Y_MASK_OFFSET
    #define PIXELGRID_Y_MASK_OFFSET global.PIXELGRID_Y_MASK_OFFSET
#endif

#ifndef PIXELGRID_DO_SHADOWMASK
    #define PIXELGRID_DO_SHADOWMASK global.PIXELGRID_DO_SHADOWMASK
#endif

#ifndef PIXELGRID_SHADOWMASK_H
    #define PIXELGRID_SHADOWMASK_H global.PIXELGRID_SHADOWMASK_H
#endif

#ifndef PIXELGRID_SHADOWMASK_SHIFT
    #define PIXELGRID_SHADOWMASK_SHIFT global.PIXELGRID_SHADOWMASK_SHIFT
#endif

#ifndef PIXELGRID_Y_MASK_STEEP
    #define PIXELGRID_Y_MASK_STEEP global.PIXELGRID_Y_MASK_STEEP
#endif

#ifndef PIXELGRID_Y_MASK_ON_WHITE
    #define PIXELGRID_Y_MASK_ON_WHITE global.PIXELGRID_Y_MASK_ON_WHITE
#endif

#ifndef PIXELGRID_Y_MASK_COORDS
    #define PIXELGRID_Y_MASK_COORDS global.PIXELGRID_Y_MASK_COORDS
#endif

#ifndef PIXELGRID_OFFSET_CORE
    #define PIXELGRID_OFFSET_CORE global.PIXELGRID_OFFSET_CORE
#endif

#ifndef PIXELGRID_Y_MASK_SHIFT
    #define PIXELGRID_Y_MASK_SHIFT global.PIXELGRID_Y_MASK_SHIFT
#endif

#ifndef PIXELGRID_Y_SPARK
    #define PIXELGRID_Y_SPARK global.PIXELGRID_Y_SPARK
#endif

#ifndef PIXELGRID_R_SHIFT
    #define PIXELGRID_R_SHIFT global.PIXELGRID_R_SHIFT
#endif

#ifndef PIXELGRID_G_SHIFT
    #define PIXELGRID_G_SHIFT global.PIXELGRID_G_SHIFT
#endif

#ifndef PIXELGRID_B_SHIFT
    #define PIXELGRID_B_SHIFT global.PIXELGRID_B_SHIFT
#endif

#ifndef PIXELGRID_H_COUNT
    #define PIXELGRID_H_COUNT global.PIXELGRID_H_COUNT
#endif

#ifndef PIXELGRID_H_PRST
    #define PIXELGRID_H_PRST  global.PIXELGRID_H_PRST
#endif

#ifndef PIXELGRID_INTR_FLICK_MODE
    #define PIXELGRID_INTR_FLICK_MODE global.PIXELGRID_INTR_FLICK_MODE
#endif

#ifndef PIXELGRID_INTR_FLICK_POWR
    #define PIXELGRID_INTR_FLICK_POWR global.PIXELGRID_INTR_FLICK_POWR
#endif

#ifndef PIXELGRID_INTR_DISABLE_Y
    #define PIXELGRID_INTR_DISABLE_Y  global.PIXELGRID_INTR_DISABLE_Y
#endif

#ifndef PIXELGRID_GAMMA_W
    #define PIXELGRID_GAMMA_W	global.PIXELGRID_GAMMA_W
#endif

#ifndef DELTA_RENDER_FORCE_REFRESH
    #define DELTA_RENDER_FORCE_REFRESH global.DELTA_RENDER_FORCE_REFRESH
#endif

#ifndef DELTA_RENDER_CHECK_AREA
    #define DELTA_RENDER_CHECK_AREA    global.DELTA_RENDER_CHECK_AREA
#endif

#ifndef DO_PPERSISTENCE
	#define DO_PPERSISTENCE		global.DO_PPERSISTENCE
#endif

#ifndef PPERSISTENCE_START
	#define PPERSISTENCE_START	global.PPERSISTENCE_START
#endif

#ifndef PPERSISTENCE_END
	#define PPERSISTENCE_END	global.PPERSISTENCE_END
#endif

#ifndef SERVICE1
    #define SERVICE1 global.SERVICE1
#endif

#ifndef SERVICE2
    #define SERVICE2 global.SERVICE2
#endif

