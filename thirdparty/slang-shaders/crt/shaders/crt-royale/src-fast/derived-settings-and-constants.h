#ifndef DERIVED_SETTINGS_AND_CONSTANTS_H
#define DERIVED_SETTINGS_AND_CONSTANTS_H

#include "user-cgp-constants.h"

#define FIX_ZERO(c) (max(abs(c), 0.0000152587890625))   //  2^-16


    float bloom_approx_filter = bloom_approx_filter_static;
    vec2 mask_resize_src_lut_size = mask_texture_small_size;


    #ifdef FORCE_RUNTIME_PHOSPHOR_MASK_MODE_TYPE_SELECT
        #define RUNTIME_PHOSPHOR_MASK_MODE_TYPE_SELECT
    #endif

    float max_aa_base_pixel_border = 0.0;

    float max_aniso_pixel_border = max_aa_base_pixel_border + 0.5;

    float max_tiled_pixel_border = max_aniso_pixel_border;

    float max_mask_texel_border = ceil(max_tiled_pixel_border);

    float max_mask_tile_border = max_mask_texel_border/
        (mask_min_allowed_triad_size * mask_triads_per_tile);

        float mask_resize_num_tiles = 1.0 +
            2.0 * max_mask_tile_border;
        float mask_start_texels = max_mask_texel_border;

float mask_resize_num_triads = mask_resize_num_tiles * mask_triads_per_tile;
vec2 min_allowed_viewport_triads = vec2(mask_resize_num_triads) / mask_resize_viewport_scale;


////////////////////////  COMMON MATHEMATICAL CONSTANTS  ///////////////////////

float pi = 3.141592653589;
float under_half = 0.4995;


#endif  //  DERIVED_SETTINGS_AND_CONSTANTS_H

