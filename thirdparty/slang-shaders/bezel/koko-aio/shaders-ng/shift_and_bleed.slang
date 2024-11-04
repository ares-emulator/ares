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
layout(location = 7) out float vDo_noise;
layout(location = 8) out float vDynamicSeed;
layout(location = 9) out float vDotCrawlShift;
layout(location = 10) out vec4 vDotCrawlCoords_and_period;
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
      vec2 d = -params.OutputSize.zw * 0.5;
      vR_offset *= d;
      vG_offset *= d;
      vB_offset *= d;
   } else if (DO_SHIFT_RGB == 1.0) {
      vR_offset=-vec2(DECON_RX, DECON_RY) * vec2(NEW_SCALEMOD_X, NEW_SCALEMOD_Y) / global.upscale_passSize.xy ;
      vG_offset=-vec2(DECON_GX, DECON_GY) * vec2(NEW_SCALEMOD_X, NEW_SCALEMOD_Y) / global.upscale_passSize.xy ;
      vB_offset=-vec2(DECON_BX, DECON_BY) * vec2(NEW_SCALEMOD_X, NEW_SCALEMOD_Y) / global.upscale_passSize.xy ;
   }
   
   
   //Precalc some conditions:
   bool do_shadow = (DO_DOT_MATRIX == 1.0 && DOT_M_SHADOW_STR > 0.0);
   vDecon_or_JustShadow =  float(DO_SHIFT_RGB > 0.0 || do_shadow    );
   vShow_artifact_mask = float(NTSC_SHOW_ARTF_MASK + DO_NTSC_ARTIFACTS == 2.0);
   
   //noise?
   vDo_noise = float( DO_RF_NOISE < -0.5 || ( DO_RF_NOISE > 0.5 && DO_IN_GLOW < 0.5 ));
   vDynamicSeed = mod(params.FrameCount, 120.0001);
   
   //Mod framecount for animating dot crawl (needed to avoid precision errors)   
      float dot_c_steps = 6.0; // frames needed to reach sin period; higher = slower;
      dot_c_steps = 22-( abs(DOT_C_SPEED) *2)+0.0001;
      vDotCrawlShift =((pi*2)/dot_c_steps) * mod(params.FrameCount, dot_c_steps);
   
      vDotCrawlCoords_and_period = vec4(vTexCoord, params.OriginalSize.xy);
   
      // switch to vertical movement
      if (DOT_C_SPEED < 0.0)
         vDotCrawlCoords_and_period.xyzw = vDotCrawlCoords_and_period.yxwz;
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
layout(location = 7) in float vDo_noise;
layout(location = 8) in float vDynamicSeed;
layout(location = 9) in float vDotCrawlShift;
layout(location = 10) in vec4 vDotCrawlCoords_and_period;

layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform sampler2D upscale_pass;


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



vec3 deconvergence_shadow(vec2 coords, sampler2D in_texture, vec3 source_pixel) {
      //Emulates deconvergence or shadowing for dot matrix screens like Gameboy.
      //Since both effects should not coexist in real life, we use a singe function for both.
      //The function stays more or less the same, but rgb offsets calculated in vertex shader
      //will be the same for shadow mode. Hopefully shader texture cache will take care of
      //multiple sampling for the same tex coords.
   
      vec3 pixel_offsetted;
      //Since we are sampling from a "clamp-to-border" texture, ensure we've no black bleeding
      //by clamping the texture coords
      //Removed, since shader chain changed, and this is now useless (?) WARNING <----
      //coords = clamp(coords,params.SourceSize.zw*1 ,1.0-params.SourceSize.zw);

      vec2 R_offset=vR_offset;
      vec2 G_offset=vG_offset;
      vec2 B_offset=vB_offset;

      if (DOT_M_SHADOW_STR == 0.0) {
         // Edge defocus:
         float dist_from_edge = distance_from_edge_linear(coords, EDGE_DEFOCUS_AREA_INV);
         float offset_multiplier = mix(EDGE_DEFOCUS, 1.0, dist_from_edge);
         R_offset *= offset_multiplier;
         G_offset *= offset_multiplier;
         B_offset *= offset_multiplier;
      }

      pixel_offsetted.r=texture(in_texture,coords + R_offset).r;
      pixel_offsetted.g=texture(in_texture,coords + G_offset).g;
      pixel_offsetted.b=texture(in_texture,coords + B_offset).b;
     
      vec3 deconvergence_mode = mix(source_pixel.rgb, pixel_offsetted, OFFSET_STRENGTH);
      vec3 shadow_mode = source_pixel.rgb - max( (source_pixel.rgb - pixel_offsetted), vec3(0.0) ) * DOT_M_SHADOW_STR;
     
      return mix_step(deconvergence_mode, shadow_mode, vDo_shadow_mode);
}

#define SIDE_RIGHT 1
#define SIDE_LEFT -1

vec3 pixel_bleed_side_NTSC(vec4 pixel_in, vec2 co, float size, sampler2D in_texture, float sourcesize_z) {
   //apply strength modifier to blur ntsc artifacts more.
   
   float strength_modifier = mix(1.0, pixel_in.a, DO_NTSC_ARTIFACTS);
   float w = SAT_BLEED_STRENGTH * clamp (strength_modifier, (1-NTSC_ARTF_NOBLEED), 1.0 );
   vec3 blur_YIQ_l = pixel_in.rgb * mat3_RGB2YIQ;    //Work in YIQ space
   vec3 blur_YIQ_r = blur_YIQ_l;

   if (vShow_artifact_mask == 1.0) return vec3(strength_modifier);
   
   vec2 off_l = vec2(SIDE_LEFT  * sourcesize_z,0.0);
   vec2 off_r = vec2(SIDE_RIGHT * sourcesize_z,0.0);
   
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

vec3 pixel_bleed_side_PAL(vec3 pixel_in, vec2 co, float size, sampler2D in_texture, float sourcesize_z) {
   float w = SAT_BLEED_STRENGTH;
   vec3 blur_YUV_l = pixel_in * mat3_RGB2YUV;    //Work in YIQ space
   vec3 blur_YUV_r = blur_YUV_l;
   
   vec2 off_l = vec2(SIDE_LEFT  * sourcesize_z,0.0);
   vec2 off_r = vec2(SIDE_RIGHT * sourcesize_z,0.0);  
   for ( float i=1 ; i <= size ; i++ ){
      w=w/SAT_BLEED_FALLOFF;
      vec2 off = vec2(sourcesize_z*i,0.0);
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

float dotc_carrier(vec2 co, vec2 OriginalSize) {  
   float sin_crawl = sin(co.x*OriginalSize.x*pi - vDotCrawlShift ) *
                     sin(co.y*OriginalSize.y*pi);
   
   sin_crawl = step(0.0, sin_crawl);
               
   sin_crawl *= DOT_C; //Modulate strength
   return sin_crawl;
}

float distance_approx(vec2 a, vec2 b, float p) {
   // for p = 0.5 it acts like distance(), but with more cycles.
   float d = dot( a-b, a-b );
   return pow(d,p);
}

vec3 dotc(sampler2D in_texture, vec3 pixel_in, vec2 co, vec4 tex_size, mat3 rgb2x, mat3 x2rgb){
   //Sample adiacent texels and switch colorspace.
   const float dot_crawl_size = 1.0; //0.5, 1.0, 1.5
   vec3 cm = texture(in_texture, co).rgb * rgb2x;
   vec3 cr = texture(in_texture, co + vec2(tex_size.z*dot_crawl_size, 0.0)).rgb * rgb2x;
   vec3 cl = texture(in_texture, co - vec2(tex_size.z*dot_crawl_size, 0.0)).rgb * rgb2x;
   //Compute max chroma change
      //float d = max(distance(cm.yz,cr.yz),  distance(cm.yz,cl.yz));
      //d = pow(d, 1.5); // bias, less on lower distance.
      //The following have less cycles, but same result.
      const float p = 0.75;
      float d = max(distance_approx(cm.yz,cr.yz,p),  distance_approx(cm.yz,cl.yz,p));
   
   //apply moving square carrier (already modulatd by strength) to the chroma distance
      float crosstalk = d * dotc_carrier(vDotCrawlCoords_and_period.xy, vDotCrawlCoords_and_period.zw);
      //return vec3(dotc_carrier(co, tex_size.xy));
   
   // chroma into luma
      pixel_in *= rgb2x;
      pixel_in.x = mix(pixel_in.x, (pixel_in.y+pixel_in.z)*0.5, crosstalk);
   
   return pixel_in * x2rgb;

}

vec4 pixel_bleed(vec4 pixel_in, vec2 co, sampler2D in_texture, float sourcesize_z) { //FIXME pixelin.a è passante, non serve vec4
   float size = SAT_BLEED_SIZE;
   #ifdef D3D_WORKAROUND
      // d3d11 compiler complains it is unable to unroll the loop because it is
      // clueless about the maximum pragma value; d3d12 hangs somewhere.
      size = min(5.0,SAT_BLEED_SIZE) ;
   #else
      size = min(20.0,SAT_BLEED_SIZE) ;
   #endif
   
   if (SAT_BLEED_PAL == 1.0) {
         return vec4 ( pixel_bleed_side_PAL(pixel_in.rgb, co, size, in_texture, sourcesize_z), pixel_in.a);
   } else {
         return vec4 ( pixel_bleed_side_NTSC(pixel_in.rgba, co, size, in_texture, sourcesize_z), pixel_in.a);
   }
}

void main() {
   vec4 pixel_out;

   pixel_out = texture(upscale_pass, vTexCoord);
   
   // CVBS bleeding before deconvergence is better when CVBS bleeding is set to pal.
   // That way deconvergence does not completely succumb to PAL bleed.
   // For performance reasons, is still a tradeoff, but at least you can modulate
   // between the two.
   
   //bleed
   if (DO_SAT_BLEED > 0.0)
      pixel_out = pixel_bleed(pixel_out, vTexCoord, upscale_pass, params.OriginalSize.z*0.5);


   //pixel_out.rgb = dotc(upscale_pass, vTexCoord, params.OriginalSize, mat3_RGB2YIQ, mat3_YIQ2RGB);
   if (DOT_C > 0.0) {
      if (DOT_C_YUV > 0.0)
         pixel_out.rgb = dotc(upscale_pass, pixel_out.rgb, vTexCoord, params.OriginalSize, mat3_RGB2YUV, mat3_YUV2RGB);
      else
         pixel_out.rgb = dotc(upscale_pass, pixel_out.rgb, vTexCoord, params.OriginalSize, mat3_RGB2YIQ, mat3_YIQ2RGB);
   }
   
   
   //... and shift
   if (vDecon_or_JustShadow > 0.0 )
      pixel_out.rgb = deconvergence_shadow(vTexCoord, upscale_pass, pixel_out.rgb);


   
   //gamma-in
   pixel_out.rgb = max(vec3(0.0), pixel_out.rgb);  //<-- before powing make sure everything is over 0.0
   
   pixel_out.rgb = pow(pixel_out.rgb, vec3(IN_GLOW_GAMMA));
   
   if (vDo_noise > 0.5 ) {
      //Unfortunately we need to clamp to min 0.0 again after applying noise
      //to avoid glitches on amd (and probably nvidi too).
      //It should be possible to change the noise function to just emit positive
      //numbers and pre-lowering the brightness of pixel_out in the very first pass tho.
      pixel_out.rgb += get_rf_noise(vTexCoord * vDynamicSeed);
      pixel_out.rgb = max(pixel_out.rgb, vec3(0.0));
   }
   
   FragColor = pixel_out;
}
