#version 450


#define CWHITE  vec3(1.0)
#define CGRAY   vec3(0.5)
#define CBLACK  vec3(0.0)
#define CPURPLE vec3(1.0,0.0,1.0)
#define CRED    vec3(1.0,0.0,0.0)
#define CGREEN    vec3(0.0,1.0,0.0)
#define CBLUE    vec3(0.0,0.0,1.0)
#define CYELLOW  vec3(1.0,1.0,0.0)
#define CCUSTOM vec3(0.3,0.5,0.1)

#define DEBUG_COLOR vec3(1,1,1)
#define DEBUG_BLINK_TIME 100
#define DEBUG_GRADIENT_STEPS 64



//#define DEBUG
   //#define DEBUG_COLOR_CALIB
   //#define DEBUG_TIME_FADE
   #define DEBUG_SOLID_COLOR
   //#define DEBUG_STEP_GRADIENT
   //#define DEBUG_STEP_GRADIENT_V
   //#define DEBUG_BLINK
   //#define DEBUG_BLINK_CIRCLE
   //#define DEBUG_WALKING_CIRCLE
   //#define DEBUG_COLORWHEEL
   
   
#include "config.inc"

#define NTSC_FILTER_WIDTH_MIN 5
#define NTSC_FILTER_WIDTH_MAX 21

#define NTSC_FILTER_MIDDLE NTSC_FILTER_WIDTH/2

//Clamp min and max artifacts loop to unroll loops? Can't observe any speed increase (maybe a bit lower)
//#define UNROLL_N

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 1) out vec3 vTemperature_rgb;
layout(location = 0) out vec2 vTexCoord;
layout(location = 2) out float vNTSC_FILTER_FC;
layout(location = 3) out float vSubcarrierFrequency_mul_size_mul_TAU;
layout(location = 4) out float vNTSC_weights[NTSC_FILTER_WIDTH_MAX];
  

#include "includes/functions.include.slang"


/* hann() sinc() functions by xot:
 * copyright (c) 2017-2018, John Leffingwell
 * license CC BY-SA Attribution-ShareAlike
 * ntscdec() function, same license and attribution,
 * slightly modified by me.
 * https://www.shadertoy.com/view/Mdffz7
 */

float hann(float n, float N) {
    return 0.5 * (1.0 - cos((TAU*n)/(N-1.0))) ;
}

float sinc(float x) {
   //if (x == 0.0) return 1.0;
   x = x + eps;
   return sin(pi*x) / (pi*x); 
   //FIXME maybe faster (parallelism) with:
   //return sin( pi*(x+eps) / (pi*(x+eps) ) );
}


void main() {
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * 1.0001;

   if (DO_GAME_GEOM_OVERRIDE > 0.5) {
      vec2 shift = vec2(GAME_GEOM_HSHIFT,GAME_GEOM_VSHIFT);
      vTexCoord = content_geom_override_shift(vTexCoord, GAME_GEOM_ASPECT, get_in_aspect(), shift.y, shift.x, GAME_GEOM_ZOOM);
   }
   
   
   if (DO_NTSC_ARTIFACTS > 0.0) {
      vNTSC_FILTER_FC = NTSC_FILTER_FC * 0.1;
      vSubcarrierFrequency_mul_size_mul_TAU = NTSC_FILTER_SCF * 0.1 * params.SourceSize.x * TAU;
      float sum = 0.0;
      float N = NTSC_FILTER_WIDTH;
      //  Compute sampling weights
      int i = 0;
      #ifdef UNROLL_N
         N = clamp(N, NTSC_FILTER_WIDTH_MIN, NTSC_FILTER_WIDTH_MAX); //Hopefully help compiler to unroll better here
      #endif
      for (float n = 0.0 ; n < N; n++) {
         int i = int(n);
         vNTSC_weights[i] = hann(n, N) * sinc(vNTSC_FILTER_FC * (n-NTSC_FILTER_MIDDLE));
         sum += vNTSC_weights[i];
      }
      //  Normalize sampling weights
      for (int n = 0; n < N; n++) {
         vNTSC_weights[n] /= sum;
      }
   }
   if (TEMPERATURE != 6500)
      vTemperature_rgb = kelvin2rgb(TEMPERATURE);
}


#pragma stage fragment

#include "includes/functions.include.slang"

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vTemperature_rgb;
layout(location = 2) in float vNTSC_FILTER_FC;
layout(location = 3) in float vSubcarrierFrequency_mul_size_mul_TAU;
layout(location = 4) in float vNTSC_weights[NTSC_FILTER_WIDTH_MAX];


layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D colortools_and_ntsc_passFeedback;
#ifndef D3D_WORKAROUND
layout(set = 0, binding = 4) uniform sampler2D avglum_passFeedback;
#endif

vec3 colorwheel(vec2 uv){
   float timeoffset = params.FrameCount / 120.0;
   vec2 uvc = -1.0 * ((2.0 * uv) - 1.0);
   float dist = length(uvc);
   float ang = (atan(uvc.y, uvc.x) + pi) / (2.0 * pi) - timeoffset;
   ang = mod(ang, 1.0);
   vec3 colHSV = vec3(ang, 1.0, dist);
   return hsv2rgb(colHSV);
}


#define pi10 pi*10
#define PHASE_SHIFT_FOR_BLUR_K pi/3
vec4 ntscdec(vec2 uv, vec3 RGB_ori) {
   
   vec3 YIQ_ori = RGB_ori * RGB2YIQ;
   
   //	Sample composite signal and decode to YIQ
      vec2 YIQ_processed = vec2(0.0);
      vec2 YIQ_processed_shifted = vec2(0.0);
      //by doing another ntsc filtering with this shifted phase, we're able to better identify
      //artifacts that will drive the blur modulation.
         float phase_shift_for_blur;
         
      int N = int(NTSC_FILTER_WIDTH);
      #ifdef UNROLL_N
         N = clamp(N, NTSC_FILTER_WIDTH_MIN, NTSC_FILTER_WIDTH_MAX); //Hopefully help compiler to unroll better here
      #endif
      for (int n=0; n < N; n++) {
         vec2 pos = uv + vec2( (n-NTSC_FILTER_MIDDLE)  * params.SourceSize.z  , 0.0);
         //vec3 smp_mul_weight = vec3((texture(Source, pos).rgb * RGB2YIQ).x) * vNTSC_weights[n] ;
         //need texture nearest here, because we have to sample linearly from previous pass,
         //In turn, because we need linear feedback from this pass (linear feedback mipmaps needed by deltarender)
         //and linear feedback is controlled by this pass filtering, contrary to the present linear filtering
         //which is controlled by next pass filtering (What a mess RetroArch!)
         vec3 smp_mul_weight = vec3((texture_NEAREST(Source, pos, params.SourceSize).rgb * RGB2YIQ).x) * vNTSC_weights[n] ;
         
         float phase =  vSubcarrierFrequency_mul_size_mul_TAU * pos.x;


         float carrier = (pos.x*params.SourceSize.x * NTSC_FILTER_SCF ) - (pos.y*params.SourceSize.y * NTSC_PHASE_SHIFT);
         phase = carrier * (pi * (2/3.0));
         float phase_shift_for_blur = phase  + PHASE_SHIFT_FOR_BLUR_K;
         
         phase += ( pos.y * -pi10 ) * (NTSC_PHASE_SHIFT);
         
         //Just sample luminance via yiq:
            //vec3 smp_mul_weight = vec3((texture(Source, pos).rgb * RGB2YIQ).x) * vNTSC_weights[n] ; <--- moved up for speedup through parallelism
            YIQ_processed         += vec2(cos(phase), sin(phase)  ) * smp_mul_weight.yz;
            YIQ_processed_shifted += vec2(cos(phase_shift_for_blur), sin(phase_shift_for_blur)) * smp_mul_weight.yz;
      }

   
   vec3 YIQ_result_to_RGB = vec3(YIQ_ori.x, YIQ_ori.yz + YIQ_processed.xy * NTSC_MIX) * YIQ2RGB ;
   
   YIQ_processed = abs(YIQ_processed);
   YIQ_processed_shifted = abs(YIQ_processed_shifted);
   
   float artifacts_mask = (YIQ_processed.x         + YIQ_processed.y) + 
                          (YIQ_processed_shifted.x + YIQ_processed_shifted.y);
   
                          
   //artifacts_mask = pow(artifacts_mask, 0.25);
                          
   return vec4(YIQ_result_to_RGB, artifacts_mask);
}


vec3 gradient(vec3 start, vec3 end, float m, float q){
   m = ceil(m*q)/q;
   return mix(start, end, m);
}

vec3 debug() {
//Just test patterns here
   vec3 pixel_debug;
   
   #ifdef DEBUG_TIME_FADE
      float time_fade = abs(sin(params.FrameCount/3.14/20));
      vec3 color_fade = DEBUG_COLOR * time_fade;
      pixel_debug = color_fade;
      
   #elif defined DEBUG_SOLID_COLOR   
      pixel_debug = DEBUG_COLOR;
      
   #elif defined DEBUG_GRADIENT
      pixel_debug = DEBUG_COLOR * vTexCoord.x;
      
   #elif defined DEBUG_STEP_GRADIENT
      pixel_debug = DEBUG_COLOR * floor(vTexCoord.x* DEBUG_GRADIENT_STEPS )/ DEBUG_GRADIENT_STEPS ;

   #elif defined DEBUG_STEP_GRADIENT_V
      pixel_debug = DEBUG_COLOR * floor(vTexCoord.y* DEBUG_GRADIENT_STEPS )/ DEBUG_GRADIENT_STEPS ;
      
   #elif defined DEBUG_BLINK
      if (mod(params.FrameCount, DEBUG_BLINK_TIME*2) < DEBUG_BLINK_TIME) 
         pixel_debug = vec3(0.0) ;
      else 
         pixel_debug = DEBUG_COLOR;
   
   #elif defined DEBUG_BLINK_CIRCLE
      vec2 center = vTexCoord - vec2(0.5,0.5);
      float radius = 0.2;
      pixel_debug = vec3(1 - step(radius, length(center)));
      pixel_debug *= float( ( mod(params.FrameCount, DEBUG_BLINK_TIME * 2) < DEBUG_BLINK_TIME));
      pixel_debug *= DEBUG_COLOR;
      
   #elif defined DEBUG_WALKING_CIRCLE
      float speed = 4.0;
      vec2 center = vec2(  mod(params.FrameCount*speed, params.SourceSize.x*1.2)/params.SourceSize.x  -0.1      , 0.5);
      center = vTexCoord - center;
      float radius = 0.05;
      pixel_debug = vec3(1 - step(radius, length(center)));
      pixel_debug *= DEBUG_COLOR;
      
   #elif defined DEBUG_COLORWHEEL   
      pixel_debug = colorwheel(vTexCoord);   
   
   #elif defined DEBUG_COLOR_CALIB
      pixel_debug = vec3(0.0);
      if (vTexCoord.y < 0.1)
         pixel_debug = gradient(CBLACK, CBLUE, vTexCoord.x, DEBUG_GRADIENT_STEPS);
      else if (vTexCoord.y < 0.2)
         pixel_debug = gradient(CBLACK, CGREEN, vTexCoord.x, DEBUG_GRADIENT_STEPS);
      else if (vTexCoord.y < 0.3)
         pixel_debug = gradient(CBLACK, vec3(0.0,1.0,1.0), vTexCoord.x, DEBUG_GRADIENT_STEPS);
      else if (vTexCoord.y < 0.4)
         pixel_debug = gradient(CBLACK, CRED, vTexCoord.x, DEBUG_GRADIENT_STEPS);
      else if (vTexCoord.y < 0.5)
         pixel_debug = gradient(CBLACK, CPURPLE, vTexCoord.x, DEBUG_GRADIENT_STEPS); 
      else if (vTexCoord.y < 0.6)
         pixel_debug = gradient(CBLACK, CYELLOW, vTexCoord.x, DEBUG_GRADIENT_STEPS); 
      else if (vTexCoord.y < 0.7)
         pixel_debug = gradient(CBLACK, CWHITE, vTexCoord.x, DEBUG_GRADIENT_STEPS); 
      else if (vTexCoord.y < 0.8)
         pixel_debug = gradient(vec3(0.9), vec3(1.0), vTexCoord.x, 8); 
      else if (vTexCoord.y < 0.9)
         pixel_debug = gradient(vec3(0.0), vec3(0.1), vTexCoord.x, 8);    
   #endif
   
   return pixel_debug;
}


bool first_in_range_strict(float a, float lo, float hi) {
   // return a inside ]lo...hi[ (lo and hi not included in range)
   float bounded = clamp(a, lo+eps, hi-eps);
   return a == bounded;
}

vec3 pixel_dedither(sampler2D tex, vec2 coords, vec4 sourcesize, float tmin, float tmax, float strength1, float strength2) {
   #define DX sourcesize.z
   #define DY sourcesize.w
   float accum_div1 = 0.0;
   float accum_div2 = 0.0;
   

   vec3 p0 = texture(tex, coords).rgb;
   
//    Horizontal xoxox   
      vec3 avg_x = vec3(0.0);
      vec3 pxb = p0;                                                 // oxOxo
      vec3 pxa = texture(tex, coords + vec2(-DX    ,0.0)).rgb; // oXoxo
      vec3 pxc = texture(tex, coords + vec2(+DX    ,0.0)).rgb; // oxoXo
      vec3 pxd = texture(tex, coords + vec2(-DX*2.0,0.0)).rgb; // Oxoxo
      vec3 pxe = texture(tex, coords + vec2(+DX*2.0,0.0)).rgb; // oxoxO
   
      float dx1= dot(abs(pxb-pxa), vec3(1.0));
      if (first_in_range_strict(dx1,tmin,tmax) && pxa == pxc && (pxb == pxd || pxb == pxe)) {
          avg_x = (pxa+pxb) /2.0;
          accum_div1 += 1.0;
       }
 
//    Vertical
      vec3 avg_y= vec3(0.0);
      vec3 pyb = p0;
      vec3 pya = texture(tex, coords + vec2(0.0, -DY    )).rgb;
      vec3 pyc = texture(tex, coords + vec2(0.0, +DY    )).rgb;
      vec3 pyd = texture(tex, coords + vec2(0.0, -DY*2.0)).rgb;
      vec3 pye = texture(tex, coords + vec2(0.0, +DY*2.0)).rgb;
   
      float dy1= dot(abs(pya-pyb), vec3(1.0));
      if (first_in_range_strict(dy1,tmin,tmax) && pya == pyc && (pyb == pyd || pyb == pye)) {
          avg_y = (pya+pyb) /2.0;
          accum_div1 += 1.0;
       }

   vec3 avg_xy= vec3(0.0);
   vec3 avg_yx= vec3(0.0);
   
   if (DEDITHER_STRENGTH2 > 0.0) {
      vec3 pxyb = p0;
      vec3 pxya = texture(tex, coords + vec2(-DX,     -DY    )).rgb;
      vec3 pxyc = texture(tex, coords + vec2(+DX,     +DY    )).rgb;
      vec3 pxyd = texture(tex, coords + vec2(+DX*2.0, +DY*2.0)).rgb;
      vec3 pxye = texture(tex, coords + vec2(-DX*2.0, -DY*2.0)).rgb;

      float dxy1= dot(abs(pxyb-pxya), vec3(1.0));
      if (first_in_range_strict(dxy1,tmin,tmax) && pxya == pxyc && (pxyb == pxyd || pxyb == pxye)) {
         //avg_xy = (pxya+pxyb) / 2.0;
         avg_xy = fma(pxya, vec3(0.5), pxyb*vec3(0.5));
         accum_div2 += 1.0;
      }

      vec3 pxyb2 = p0;
      vec3 pxya2 = texture(tex, coords + vec2(+DX,    -DY    )).rgb;
      vec3 pxyc2 = texture(tex, coords + vec2(-DX,    +DY    )).rgb;
      vec3 pxyd2 = texture(tex, coords + vec2(-DX*2.0,+DY*2.0)).rgb;
      vec3 pxye2 = texture(tex, coords + vec2(+DX*2.0,-DY*2.0)).rgb;

      float dxy12= dot(abs(pxyb2-pxya2), vec3(1.0));

      if (first_in_range_strict(dxy12,tmin,tmax) && pxya2 == pxyc2 && (pxyb2 == pxyd2 || pxyb2 == pxye)) {
         //avg_yx = (pxya2+pxyb2) / 2.0;
         avg_yx = fma(pxya2, vec3(0.5), pxyb2*vec3(0.5));
         accum_div2 += 1.0;
      }
   }

   vec3 pixel_out = p0;
   if (accum_div1 > 0.0) {
      pixel_out = mix(pixel_out, fma(avg_x, vec3(1.0)/accum_div1, avg_y / accum_div1) , strength1); //ASM PROOF FASTER
   }
   if (accum_div2 > 0.0) {
      //pixel_out = mix(pixel_out, (avg_xy + avg_yx) / accum_div2, strength2);
      pixel_out = mix(pixel_out, fma(avg_xy, vec3(1.0)/accum_div2, avg_yx / accum_div2 ) , strength2); //ASM PROOF FASTER
   }

   return pixel_out;
}


vec3 persistence(vec3 feedback, vec3 pixel_now, float start_decay, float late_persistence, float motion_blur_cancel) {
   //start_decay: is the initial decay, the fast one: 1.0 means instantaneous, try around 0.8
   //end decay the higher, the more the persistance in the long run, try around 1.0;
   //motion_blur_cancel: 0.0 means full blur on bright areas; try around 2.5

   vec3 diff=(feedback - pixel_now);

   vec3 speed_fast = diff * start_decay;
   vec3 speed_slow = diff / (late_persistence+eps); //fma() not usable pr slower. 
      
   vec3 fade_speed = mix(speed_slow, speed_fast, diff );
   
   //higher speed is needed when composing over brighter colors to avoid motion blur
   fade_speed = fade_speed + (pixel_now*motion_blur_cancel);
   //fade_speed = fma(pixel_now, vec3(motion_blur_cancel), vec3(fade_speed)); //ASM PROOF: SLOWER
   
   //Ensure a minimum speed to reach 0.0 and clamp to 1.0 to avoid funky effects
   float min_speed = 0.05  ;
   fade_speed=clamp(fade_speed, min_speed, 1.0);
   

   //Compute the persistent light by mixing pixel current and previous one
   vec3 pixel_persistent = mix(feedback, pixel_now, fade_speed );  
   
   //blend only if we are fading out
      vec3 light_fading_out = vec3(greaterThan(feedback, pixel_now));
      return mix(pixel_now, pixel_persistent, light_fading_out);
   }
   


void main() {
   vec2 co = vTexCoord;
   
   if (AUTOCROP_MAX > 0.0)
      co = clamp(co,  params.OriginalSize.zw * AUTOCROP_MIN, 
                    1-params.OriginalSize.zw * AUTOCROP_MIN );
   
   #ifndef D3D_WORKAROUND
      float shake = texture(avglum_passFeedback, RESSWITCH_SAMPLING_POINT).a;
      //co.y += sin(shake * 2 * pi) * RESSWITCH_GLITCH_SIZE;
      co.y = co.y + (sin(shake * 2 * pi) * RESSWITCH_GLITCH_SIZE);
   #endif

   vec3 pixel_in = texture(Source, co).rgb;
   vec3 pixel_out = pixel_in;
   //This will go into alpha channel to make glow aware of it and blur more.
   //It could be configurable.
   float artifact_mask = 0.0;

   if (DO_NTSC_ARTIFACTS > 0.5) {
      //Get ntsc picture
         vec4 ntsc_decoded = ntscdec(co, pixel_in);
      //apply the minimum treshold to artifact mask
         artifact_mask = smoothstep_fast(NTSC_ARTF_TRSH , 1.0, ntsc_decoded.a);

         ntsc_decoded = clamp(ntsc_decoded, vec4(0.0), vec4(1.0));

      //Optionally completely cut-off artifacts under treshold:
         ntsc_decoded = mix(ntsc_decoded, vec4(pixel_in, 0.0) , (1-artifact_mask)*NTSC_ARTF_ZAP );
         //Update the artifact mask if something has been cancelled
            artifact_mask = min(artifact_mask, ntsc_decoded.a);

            //artifact_mask = clamp (artifact_mask*1.0, 0.0,1.0);
            artifact_mask *= 2.0;
            
      pixel_out = ntsc_decoded.rgb;
      
   } else if (DO_DEDITHER > 0.5) {
      //vec3 pixel_dedither(vec2 coords, vec4 sourcesize, float tmin, float tmax, float strength) {
      float tmin = 0.0;
      pixel_out = pixel_dedither(Source, co, params.SourceSize, tmin, DEDITHER_TRESHMAX, DEDITHER_STRENGTH, DEDITHER_STRENGTH2);
   }

   
   #ifdef DEBUG
      //Cant "debug" ntsc artifacts because it samples from source (original)
      pixel_out = debug();
   #endif
      

   if (DO_CCORRECTION == 1.0) {
      pixel_out = color_tools(pixel_out, vTemperature_rgb);
      #ifndef D3D_WORKAROUND
         if (ADAPTIVE_BLACK > 0.0) {
            float avglum = texture(avglum_passFeedback, vec2(0.5,0.5)).a ;
            pixel_out = adaptive_black(pixel_out,  avglum, ADAPTIVE_BLACK);
         }
      #endif
	}

   if (DO_DOT_MATRIX + DOT_M_MBLUR_STR > 1.0) {
      vec3 feedback = texture(colortools_and_ntsc_passFeedback, co).rgb;
      
      vec3 m_blur_adapted = vec3(DOT_M_MBLUR_STR);

      //Dot matrix motion blur inertia on white or black (not both)?
      if (DOT_M_MBLUR_MODE > 0.0) {
         vec3 adaption = mix_step (pixel_out - feedback,
                                   feedback - pixel_out,
                                   DOT_M_MBLUR_MODE -1);
            
         //adaption = mix_step(adaption, -adaption, DOT_M_MBLUR_MODE -1);
         m_blur_adapted *= clamp( adaption, 0.0, 1.0 );
         m_blur_adapted = pow(m_blur_adapted, vec3(0.25));
      }
      pixel_out = mix(pixel_out, feedback, m_blur_adapted  );
   }
   

   /*if (mod(  floor(co.y * params.OutputSize.y) , 2.0) == 0.0)
      if (mod(params.FrameCount , 2.0) == 0.0)
      pixel_out = vec3(0.0);
      
     if (mod( floor(co.y * params.OutputSize.y), 2.0) != 0.0)
      if (mod(params.FrameCount , 2.0) != 0.0)
      pixel_out = vec3(0.0);    
   */

   if (DO_PPERSISTENCE == 1.0) {
      if (params.OutputSize.xy == global.colortools_and_ntsc_passFeedbackSize.xy) {
         vec3 feedback = texture(colortools_and_ntsc_passFeedback, co).rgb;
         float motion_blur_cancel = 2.5;
         pixel_out = persistence(feedback, pixel_out, PPERSISTENCE_START, PPERSISTENCE_END, motion_blur_cancel);
      }
   }
 
  
   //Clamp artifacts to 0..1
   //Clamp RGB to 0..1000.0
   pixel_out = max(pixel_out, vec3(0.0));
   artifact_mask = clamp(artifact_mask, 0.0, 1.0);
   FragColor = vec4(pixel_out, artifact_mask);

}


