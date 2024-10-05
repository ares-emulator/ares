#ifndef USER_CGP_CONSTANTS_H
#define USER_CGP_CONSTANTS_H

//  PASS SCALES AND RELATED CONSTANTS:
float bloom_approx_size_x = 320.0;
float bloom_approx_size_x_for_fake = 400.0;
vec2 mask_resize_viewport_scale = vec2(0.0625, 0.0625);
float geom_max_aspect_ratio = 4.0/3.0;

//  PHOSPHOR MASK TEXTURE CONSTANTS:
vec2 mask_texture_small_size = vec2(64.0, 64.0);
//vec2 mask_texture_large_size = vec2(512.0, 512.0);
float mask_triads_per_tile = 8.0;
//#define PHOSPHOR_MASK_GRILLE14
//float mask_grille14_avg_color = 50.6666666/255.0;
float mask_grille15_avg_color = 53.0/255.0;
float mask_slot_avg_color     = 46.0/255.0;
float mask_shadow_avg_color   = 41.0/255.0;

    float mask_grille_avg_color = mask_grille15_avg_color;


#endif  //  USER_CGP_CONSTANTS_H

