#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

//    #ifdef DRIVERS_ALLOW_DERIVATIVES
//        #define DRIVERS_ALLOW_FINE_DERIVATIVES
//    #endif

    //#define DRIVERS_ALLOW_DYNAMIC_BRANCHES

    //#define ACCOMODATE_POSSIBLE_DYNAMIC_LOOPS

    //#define DRIVERS_ALLOW_TEX2DLOD

    //#define DRIVERS_ALLOW_TEX2DBIAS

    //#define INTEGRATED_GRAPHICS_COMPATIBILITY_MODE


#define RUNTIME_SHADER_PARAMS_ENABLE
#define RUNTIME_PHOSPHOR_BLOOM_SIGMA
#define RUNTIME_ANTIALIAS_WEIGHTS
//#define RUNTIME_ANTIALIAS_SUBPIXEL_OFFSETS
#define RUNTIME_SCANLINES_HORIZ_FILTER_COLORSPACE
#define RUNTIME_GEOMETRY_TILT
#define RUNTIME_GEOMETRY_MODE
#define FORCE_RUNTIME_PHOSPHOR_MASK_MODE_TYPE_SELECT
    #define PHOSPHOR_MASK_MANUALLY_RESIZE
    #define PHOSPHOR_MASK_RESIZE_LANCZOS_WINDOW
    #define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_3_PIXELS
    //#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_6_PIXELS
    //#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_9_PIXELS
    //#define PHOSPHOR_BLOOM_TRIADS_LARGER_THAN_12_PIXELS

//  GAMMA:
    float crt_gamma_static = 2.5;                  //  range [1, 5]
    float lcd_gamma_static = 2.2;                  //  range [1, 5]

//  LEVELS MANAGEMENT:
    float levels_contrast_static = 1.0;            //  range [0, 4)
    float levels_autodim_temp = 0.5;               //  range (0, 1]

//  HALATION/DIFFUSION/BLOOM:
    float halation_weight_static = 0.0;            //  range [0, 1]
    float diffusion_weight_static = 0.075;         //  range [0, 1]
    float bloom_underestimate_levels_static = 0.8; //  range [0, 5]
    float bloom_excess_static = 0.0;               //  range [0, 1]

    float bloom_approx_filter_static = 0.0;

    float beam_num_scanlines = 3.0;                //  range [2, 6]
    bool beam_generalized_gaussian = true;
    float beam_antialias_level = 1.0;              //  range [0, 2]
    float beam_min_sigma_static = 0.02;            //  range (0, 1]
    float beam_max_sigma_static = 0.3;             //  range (0, 1]
    float beam_spot_shape_function = 0.0;
    float beam_spot_power_static = 1.0/3.0;    //  range (0, 16]
    float beam_min_shape_static = 2.0;         //  range [2, 32]
    float beam_max_shape_static = 4.0;         //  range [2, 32]
    float beam_shape_power_static = 1.0/4.0;   //  range (0, 16]
    float beam_horiz_filter_static = 0.0;
    float beam_horiz_sigma_static = 0.35;      //  range (0, 2/3]
    float beam_horiz_linear_rgb_weight_static = 1.0;   //  range [0, 1]
    bool beam_misconvergence = true;
    vec2  convergence_offsets_r_static = vec2 (0.1, 0.2);
    vec2  convergence_offsets_g_static = vec2 (0.3, 0.4);
    vec2  convergence_offsets_b_static = vec2 (0.5, 0.6);
    bool interlace_detect_static = true;
    bool interlace_1080i_static = false;
    bool interlace_bff_static = false;

//  ANTIALIASING:
    float aa_level = 12.0;                     //  range [0, 24]
    float aa_filter = 6.0;                     //  range [0, 9]
    bool aa_temporal = false;
    vec2  aa_subpixel_r_offset_static = vec2 (-1.0/3.0, 0.0);//vec2 (0.0);
    float aa_cubic_c_static = 0.5;             //  range [0, 4]
    float aa_gauss_sigma_static = 0.5;     //  range [0.0625, 1.0]

//  PHOSPHOR MASK:
    float mask_type_static = 1.0;                  //  range [0, 2]
    float mask_sample_mode_static = 0.0;           //  range [0, 2]
    float mask_specify_num_triads_static = 0.0;    //  range [0, 1]
    float mask_triad_size_desired_static = 24.0 / 8.0;
    float mask_num_triads_desired_static = 480.0;
    float mask_sinc_lobes = 3.0;                   //  range [2, 4]
    float mask_min_allowed_triad_size = 2.0;

//  GEOMETRY:
    float geom_mode_static = 0.0;      //  range [0, 3]
    float geom_radius_static = 2.0;    //  range [1/(2*pi), 1024]
    float geom_view_dist_static = 2.0; //  range [0.5, 1024]
    vec2  geom_tilt_angle_static = vec2 (0.0, 0.0);  //  range [-pi, pi]
    float geom_aspect_ratio_static = 1.313069909;
    vec2  geom_overscan_static = vec2 (1.0, 1.0);// * 1.005 * (1.0, 240/224.0)
    bool geom_force_correct_tangent_matrix = true;

//  BORDERS:
    float border_size_static = 0.015;           //  range [0, 0.5]
    float border_darkness_static = 2.0;        //  range [0, inf)
    float border_compress_static = 2.5;        //  range [1, inf)


#endif  //  USER_SETTINGS_H

