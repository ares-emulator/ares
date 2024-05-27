#version 450
#include "config.inc"

#define RGB_SHIFT_RANGE 20

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec2 vR_offset;
layout(location = 2) out vec2 vG_offset;
layout(location = 3) out vec2 vB_offset;
layout(location = 4) out float vDo_shadow_mode;
layout(location = 5) out float vDecon_or_JustShadow;
layout(location = 6) out float vShow_artifact_mask;
#include "includes/functions.include.slang"

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord ;  
   
   //...to tell fragment shader if dot matrix feature requests shadows.
      vDo_shadow_mode = float(DOT_M_SHADOW_STR + DO_DOT_MATRIX > 1 + eps);
   
   //Shadow mode disables deconvergence, they do not coexist in real life.
   if (vDo_shadow_mode == 1.0) {
      vR_offset = vec2(DOT_M_SHADOW_OFF, abs(DOT_M_SHADOW_OFF));
      vG_offset = vR_offset;
      vB_offset = vR_offset;
   } else if (DO_SHIFT_RGB == 1.0) {
      vR_offset=offsets_from_float(SHIFT_R+210.0,RGB_SHIFT_RANGE);
      vG_offset=offsets_from_float(SHIFT_G+210.0,RGB_SHIFT_RANGE);
      vB_offset=offsets_from_float(SHIFT_B+210.0,RGB_SHIFT_RANGE);
   }
   
   
   vec2 d = -params.OutputSize.zw * 0.5;
   vR_offset *= d;
   vG_offset *= d;
   vB_offset *= d;
   
   
   //Precalc some conditions:
   bool do_shadow = (DO_DOT_MATRIX == 1.0 && DOT_M_SHADOW_STR > 0.0);
   vDecon_or_JustShadow =  float(DO_SHIFT_RGB > 0.0 || do_shadow    );
   vShow_artifact_mask = float(NTSC_SHOW_ARTF_MASK + DO_NTSC_ARTIFACTS == 2.0);
}


#pragma stage fragment
#include "includes/functions.include.slang"
layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec2 vR_offset;
layout(location = 2) in vec2 vG_offset;
layout(location = 3) in vec2 vB_offset;
layout(location = 4) in float vDo_shadow_mode;
layout(location = 5) in float vDecon_or_JustShadow;
layout(location = 6) in float vShow_artifact_mask;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D FXAA_pass;
layout(set = 0, binding = 2) uniform sampler2D flick_and_noise_pass;


#define bandwidth_mhz_Y_ntsc 4.2
#define bandwidth_mhz_I 1.5
#define bandwidth_mhz_Q 0.5

const mat3 mat3_RGB2YIQ = mat3(
   0.2989,  0.5959,  0.2115,
   0.5870, -0.2744, -0.5229,
   0.1140, -0.3216,  0.3114);

const mat3 mat3_YIQ2RGB = mat3(
   1.0,     1.0,     1.0,
   0.956,  -0.2720, -1.1060,
   0.6210, -0.6474,  1.7046);

//https://www.sciencedirect.com/topics/computer-science/color-subcarrier
#define bandwidth_mhz_Y_pal 5.0
#define bandwidth_mhz_U 1.3
#define bandwidth_mhz_V 1.3

const mat3 mat3_RGB2YUV = mat3(
   0.299,     0.587,     0.114,
   -0.14713, -0.28886,   0.436,
   0.615,    -0.514991, -0.10001);

const mat3 mat3_YUV2RGB = mat3(
   1.000, 0.000,   1.13983,
   1.000,-0.39465,-0.58060,
   1.000, 2.03211, 0.00000);



vec4 deconvergence_shadow(vec2 coords, sampler2D in_texture, vec4 source_pixel) {
      //Emulates deconvergence or shadowing for dot matrix screens like Gameboy.
      //Since both effects should not coexist in real life, we use a singe function for both.
      //The function stays more or less the same, but rgb offsets calculated in vertex shader
      //will be the same for shadow mode. Hopefully shader texture cache will take care of
      //multiple sampling for the same tex coords.
   
      vec3 pixel_offset;
      //Since we are sampling from a "clamp-to-border" texture, ensure we've no black bleeding
      //by clamping the texture coords
      coords = clamp(coords,params.SourceSize.zw*1 ,1.0-params.SourceSize.zw);
      pixel_offset.r=texture(in_texture,coords + vR_offset).r;
      pixel_offset.g=texture(in_texture,coords + vG_offset).g;
      pixel_offset.b=texture(in_texture,coords + vB_offset).b;
      
      vec3 deconvergence_mode = mix(source_pixel.rgb, pixel_offset, OFFSET_STRENGTH);
      vec3 shadow_mode = source_pixel.rgb - max( (source_pixel.rgb - pixel_offset), vec3(0.0) ) * DOT_M_SHADOW_STR;
      
      return vec4( mix_step(deconvergence_mode, shadow_mode, vDo_shadow_mode),
                   source_pixel.a );
}


vec4 deconvergence_shadow_wrap(vec2 coord, vec4 source_pixel) {
   if ( DO_FXAA == 1.0)
      return deconvergence_shadow(coord, FXAA_pass, source_pixel);
   else
      return deconvergence_shadow(coord, flick_and_noise_pass, source_pixel);
}

#define SIDE_RIGHT 1
#define SIDE_LEFT -1

vec3 pixel_bleed_side_NTSC(vec4 pixel_in, vec2 co, float size, sampler2D in_texture, vec4 sourcesize2) {
   //apply strength modifier to blur ntsc artifacts more.
   
   float strength_modifier = mix(1.0, pixel_in.a, DO_NTSC_ARTIFACTS); 
   float w = SAT_BLEED_STRENGTH * clamp (strength_modifier, (1-NTSC_ARTF_NOBLEED), 1.0 ); 
   vec3 blur_YIQ_l = pixel_in.rgb * mat3_RGB2YIQ;    //Work in YIQ space
   vec3 blur_YIQ_r = blur_YIQ_l;

   if (vShow_artifact_mask == 1.0) return vec3(strength_modifier);
   
   vec2 off_l = vec2(SIDE_LEFT  * sourcesize2.z,0.0);
   vec2 off_r = vec2(SIDE_RIGHT * sourcesize2.z,0.0);
   
   for ( float i=1 ; i <= size ; i++ ){
      w=w/SAT_BLEED_FALLOFF;  //w = w * exp(i*i*(1-SAT_BLEED_FALLOFF)*0.1);
      vec3 smp_YIQ = texture(in_texture, co - i*off_l ).rgb * mat3_RGB2YIQ;
      //Blur Y, I and Q
      blur_YIQ_l = mix(blur_YIQ_l.xyz, smp_YIQ.xyz, w/vec3( bandwidth_mhz_Y_ntsc, bandwidth_mhz_I, bandwidth_mhz_Q));
      
      smp_YIQ = texture(in_texture, co - i*off_r ).rgb * mat3_RGB2YIQ;
      //Blur Y, I and Q
      blur_YIQ_r = mix(blur_YIQ_r.xyz, smp_YIQ.xyz, w/vec3( bandwidth_mhz_Y_ntsc, bandwidth_mhz_I, bandwidth_mhz_Q));
   }
   vec3 blur_RGB_l = blur_YIQ_l * mat3_YIQ2RGB; //return to RGB colorspace
   vec3 blur_RGB_r = blur_YIQ_r * mat3_YIQ2RGB; //return to RGB colorspace
   
   // Clamping min to 0.0 is needed for nvidia to avoid bad graphical glitches, why?
   return max(mix(blur_RGB_l,blur_RGB_r,0.5), 0.0);
}

vec3 pixel_bleed_side_PAL(vec3 pixel_in, vec2 co, float size, sampler2D in_texture, vec4 sourcesize2) {
   float w = SAT_BLEED_STRENGTH;
   vec3 blur_YUV_l = pixel_in * mat3_RGB2YUV;    //Work in YIQ space
   vec3 blur_YUV_r = blur_YUV_l;
   
   vec2 off_l = vec2(SIDE_LEFT  * sourcesize2.z,0.0);
   vec2 off_r = vec2(SIDE_RIGHT * sourcesize2.z,0.0);   
   for ( float i=1 ; i <= size ; i++ ){
      w=w/SAT_BLEED_FALLOFF;
      vec2 off = vec2(sourcesize2.z*i,0.0);
      vec3 smp_YUV = texture(in_texture, co - i*off_l ).rgb * mat3_RGB2YUV;
      //Blur Y, U, V
      blur_YUV_l = mix(blur_YUV_l.xyz, smp_YUV.xyz, w/vec3( bandwidth_mhz_Y_pal, bandwidth_mhz_U, bandwidth_mhz_V));
      
      smp_YUV = texture(in_texture, co - i*off_r ).rgb * mat3_RGB2YUV;
      //Blur Y, U, V
      blur_YUV_r = mix(blur_YUV_r.xyz, smp_YUV.xyz, w/vec3( bandwidth_mhz_Y_pal, bandwidth_mhz_U, bandwidth_mhz_V));
   }
   vec3 blur_RGB_l = blur_YUV_l * mat3_YUV2RGB; //return to RGB colorspace
   vec3 blur_RGB_r = blur_YUV_r * mat3_YUV2RGB; //return to RGB colorspace
   
   // Clamping min to 0.0 is needed for nvidia to avoid bad graphical glitches, why?
   return max(mix(blur_RGB_l,blur_RGB_r,0.5), 0.0);

}

vec4 pixel_bleed(vec4 pixel_in, vec2 co, sampler2D in_texture, vec4 sourcesize2) {
   float size = SAT_BLEED_SIZE;
   #ifdef D3D_WORKAROUND
      // d3d11 compiler complains it is unable to unroll the loop because it is
      // clueless about the maximum pragma value; d3d12 hangs somewhere.
      size = min(5.0,SAT_BLEED_SIZE) ;
   #else 
      size = min(20.0,SAT_BLEED_SIZE) ;
   #endif
   
   if (SAT_BLEED_PAL == 1.0) {
         return vec4 ( pixel_bleed_side_PAL(pixel_in.rgb, co, size, in_texture, sourcesize2), pixel_in.a);
   } else {
         return vec4 ( pixel_bleed_side_NTSC(pixel_in.rgba, co, size, in_texture, sourcesize2), pixel_in.a);
   }
}

void main() {
   vec4 pixel_out;

      if ( DO_FXAA == 1.0)  {
         pixel_out = texture(FXAA_pass, vTexCoord );
      } else {
         pixel_out = texture(flick_and_noise_pass, vTexCoord);
      }

      
   //shift
   if (vDecon_or_JustShadow > 0.0 )
      pixel_out = deconvergence_shadow_wrap(vTexCoord, pixel_out);
   
   //..and bleed
   if (DO_SAT_BLEED > 0.0)
      pixel_out = pixel_bleed(pixel_out, vTexCoord, flick_and_noise_pass, global.flick_and_noise_passSize);

   
   //pre-gamma if needed by glow.
   if (DO_CCORRECTION > 0.5) 
      pixel_out.rgb = pow(pixel_out.rgb, vec3(IN_GLOW_GAMMA));
 
   FragColor = pixel_out;
}
