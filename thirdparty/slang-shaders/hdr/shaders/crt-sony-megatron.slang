#version 450

/* 
A shader that tries to emulate a sony PVM type aperture grille screen but with full brightness.

The novel thing about this shader is that it relies on the HDR shaders to brighten up the image so that when 
we apply this shader which emulates the apperture grille the resulting screen isn't left too dark.  

I think you need at least a DisplayHDR 600 monitor but to get close to CRT levels of brightness I think DisplayHDR 1000.

Please Enable HDR in RetroArch 1.10+

NOTE: when this shader is envoked the Contrast, Peak Luminance and Paper White Luminance in the HDR menu do nothing instead set those values through the shader parameters 

For this shader set Paper White Luminance to above 700 and Peak Luminance to the peak luminance of your monitor.  

Also try to use a integer scaling - its just better - overscaling is fine.

This shader doesn't do any geometry warping or bouncing of light around inside the screen etc - I think these effects just add unwanted noise, I know people disagree. Please feel free to make you own and add them

Dont use this shader directly - use the hdr\crt-make-model-hdr.slangp where make and model are the make and model of the CRT you want.

THIS SHADER DOES NOT SUPPORT WRGB OLED (Due to the sub pixel layout of WRGB - RGB QD-OLED or LCD (and variants thereof screens are fine)
*/

#pragma format A2B10G10R10_UNORM_PACK32

layout(push_constant) uniform Push
{
   // User Settings
   float hcrt_hdr;
   float hcrt_colour_space;
   float hcrt_max_nits;
   float hcrt_paper_white_nits;
   float hcrt_expand_gamut;
   
   float hcrt_colour_accurate;

   float hcrt_lcd_resolution;
   float hcrt_lcd_subpixel;

   float hcrt_red_vertical_convergence;
   float hcrt_green_vertical_convergence;
   float hcrt_blue_vertical_convergence;
   float hcrt_red_horizontal_convergence;
   float hcrt_green_horizontal_convergence;
   float hcrt_blue_horizontal_convergence;

   // Developer Settings
   float hcrt_crt_screen_type;
   float hcrt_crt_resolution;

   // Vertical Settings
   float hcrt_red_scanline_min;
   float hcrt_red_scanline_max;
   float hcrt_red_scanline_attack;
   float hcrt_green_scanline_min;
   float hcrt_green_scanline_max;
   float hcrt_green_scanline_attack;
   float hcrt_blue_scanline_min;
   float hcrt_blue_scanline_max;
   float hcrt_blue_scanline_attack;

   // Horizontal Settings
   float hcrt_red_beam_sharpness;
   float hcrt_red_beam_attack;
   float hcrt_green_beam_sharpness;
   float hcrt_green_beam_attack;
   float hcrt_blue_beam_sharpness;
   float hcrt_blue_beam_attack;

} params;

layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;

   float hcrt_r709_gamma_out;
   float hcrt_srgb_gamma_out;
   float hcrt_p3_gamma_out;
   float hcrt_r709_gamma_cutoff;
   float hcrt_srgb_gamma_cutoff;

   float hcrt_h_size;
   float hcrt_v_size;
   float hcrt_h_cent;
   float hcrt_v_cent;
   float hcrt_pin_phase;
   float hcrt_pin_amp;
} global;

#include "include/parameters.h"

#define HCRT_HDR                             params.hcrt_hdr
#define HCRT_OUTPUT_COLOUR_SPACE             params.hcrt_colour_space
#define HCRT_MAX_NITS                        params.hcrt_max_nits
#define HCRT_PAPER_WHITE_NITS                params.hcrt_paper_white_nits
#define HCRT_EXPAND_GAMUT                    params.hcrt_expand_gamut
#define HCRT_R709_GAMMA_OUT                  global.hcrt_r709_gamma_out
#define HCRT_SRGB_GAMMA_OUT                  global.hcrt_srgb_gamma_out
#define HCRT_P3_GAMMA_OUT                    global.hcrt_p3_gamma_out
#define HCRT_R709_GAMMA_CUTOFF               global.hcrt_r709_gamma_cutoff
#define HCRT_SRGB_GAMMA_CUTOFF               global.hcrt_srgb_gamma_cutoff
#define HCRT_COLOUR_ACCURATE                 params.hcrt_colour_accurate

#define HCRT_LCD_RESOLUTION                  params.hcrt_lcd_resolution 
#define HCRT_LCD_SUBPIXEL                    params.hcrt_lcd_subpixel
#define HCRT_RED_VERTICAL_CONVERGENCE        params.hcrt_red_vertical_convergence
#define HCRT_GREEN_VERTICAL_CONVERGENCE      params.hcrt_green_vertical_convergence
#define HCRT_BLUE_VERTICAL_CONVERGENCE       params.hcrt_blue_vertical_convergence
#define HCRT_RED_HORIZONTAL_CONVERGENCE      params.hcrt_red_horizontal_convergence
#define HCRT_GREEN_HORIZONTAL_CONVERGENCE    params.hcrt_green_horizontal_convergence
#define HCRT_BLUE_HORIZONTAL_CONVERGENCE     params.hcrt_blue_horizontal_convergence

#define HCRT_CRT_SCREEN_TYPE                 params.hcrt_crt_screen_type
#define HCRT_CRT_RESOLUTION                  params.hcrt_crt_resolution

#define HCRT_RED_SCANLINE_MIN                params.hcrt_red_scanline_min
#define HCRT_RED_SCANLINE_MAX                params.hcrt_red_scanline_max
#define HCRT_RED_SCANLINE_ATTACK             params.hcrt_red_scanline_attack
#define HCRT_GREEN_SCANLINE_MIN              params.hcrt_green_scanline_min
#define HCRT_GREEN_SCANLINE_MAX              params.hcrt_green_scanline_max
#define HCRT_GREEN_SCANLINE_ATTACK           params.hcrt_green_scanline_attack
#define HCRT_BLUE_SCANLINE_MIN               params.hcrt_blue_scanline_min
#define HCRT_BLUE_SCANLINE_MAX               params.hcrt_blue_scanline_max
#define HCRT_BLUE_SCANLINE_ATTACK            params.hcrt_blue_scanline_attack

#define HCRT_RED_BEAM_SHARPNESS              params.hcrt_red_beam_sharpness
#define HCRT_RED_BEAM_ATTACK                 params.hcrt_red_beam_attack
#define HCRT_GREEN_BEAM_SHARPNESS            params.hcrt_green_beam_sharpness
#define HCRT_GREEN_BEAM_ATTACK               params.hcrt_green_beam_attack
#define HCRT_BLUE_BEAM_SHARPNESS             params.hcrt_blue_beam_sharpness
#define HCRT_BLUE_BEAM_ATTACK                params.hcrt_blue_beam_attack

#define HCRT_H_SIZE                          global.hcrt_h_size
#define HCRT_V_SIZE                          global.hcrt_v_size
#define HCRT_H_CENT                          global.hcrt_h_cent
#define HCRT_V_CENT                          global.hcrt_v_cent
#define HCRT_PIN_PHASE                       global.hcrt_pin_phase
#define HCRT_PIN_AMP                         global.hcrt_pin_amp

#define COMPAT_TEXTURE(c, d) texture(c, d)

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord * vec2(1.00001);  // To resolve rounding issues when sampling
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D SourceSDR;
layout(set = 0, binding = 3) uniform sampler2D SourceHDR;


#define kChannelMask          3
#define kFirstChannelShift    2
#define kSecondChannelShift   4
#define kThirdChannelShift    6

#define kRedId   0
#define kGreenId 1
#define kBlueId  2

#define kRed     (1 | (kRedId << kFirstChannelShift))
#define kGreen   (1 | (kGreenId << kFirstChannelShift))
#define kBlue    (1 | (kBlueId << kFirstChannelShift))
#define kMagenta (2 | (kRedId << kFirstChannelShift) | (kBlueId << kSecondChannelShift))
#define kYellow  (2 | (kRedId << kFirstChannelShift) | (kGreenId << kSecondChannelShift))
#define kCyan    (2 | (kGreenId << kFirstChannelShift) | (kBlueId << kSecondChannelShift))
#define kWhite   (3 | (kRedId << kFirstChannelShift) | (kGreenId << kSecondChannelShift) | (kBlueId << kThirdChannelShift))
#define kBlack   0

#define kRedChannel     vec3(1.0, 0.0, 0.0)
#define kGreenChannel   vec3(0.0, 1.0, 0.0)
#define kBlueChannel    vec3(0.0, 0.0, 1.0)

const vec3 kColourMask[3] = { kRedChannel, kGreenChannel, kBlueChannel };

#define kApertureGrille    0
#define kShadowMask        1
#define kSlotMask          2
#define kBlackWhiteMask    3

#define kBGRAxis           3
#define kTVLAxis           4
#define kResolutionAxis    3

// APERTURE GRILLE MASKS

const float kApertureGrilleMaskSize[kResolutionAxis * kTVLAxis] = { 
     4.0f, 2.0f, 1.0f, 1.0f ,      // 1080p:   300 TVL, 600 TVL, 800 TVL, 1000 TVL 
     7.0f, 4.0f, 3.0f, 2.0f ,      // 4K:      300 TVL, 600 TVL, 800 TVL, 1000 TVL   
    13.0f, 7.0f, 5.0f, 4.0f   };   // 8K:      300 TVL, 600 TVL, 800 TVL, 1000 TVL

// SHADOW MASKS

const float kShadowMaskSizeX[kResolutionAxis * kTVLAxis] = {   6.0f, 2.0f, 1.0f, 1.0f  ,   12.0f, 6.0f, 2.0f, 2.0f  ,   12.0f, 12.0f, 6.0f, 6.0f   }; 
const float kShadowMaskSizeY[kResolutionAxis * kTVLAxis] = {   4.0f, 2.0f, 1.0f, 1.0f  ,    8.0f, 4.0f, 2.0f, 2.0f  ,    8.0f,  8.0f, 4.0f, 4.0f   }; 

// SLOT MASKS

const float kSlotMaskSizeX[kResolutionAxis * kTVLAxis] = {   4.0f, 2.0f, 1.0f, 1.0f  ,   7.0f, 4.0f, 3.0f, 2.0f  ,   7.0f, 7.0f, 5.0f, 4.0f   }; //1080p: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   4K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   8K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL
const float kSlotMaskSizeY[kResolutionAxis * kTVLAxis] = {   4.0f, 4.0f, 1.0f, 1.0f  ,   8.0f, 6.0f, 4.0f, 4.0f  ,   6.0f, 6.0f, 4.0f, 4.0f   }; //1080p: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   4K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   8K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL


#include "include/scanline_generation.h"
#include "include/gamma_correct.h"

#define k1080p     0
#define k4K        1
#define k8K        2

#define k300TVL    0
#define k600TVL    1
#define k800TVL    2
#define k1000TVL   3

void main()
{
   const uint screen_type           = uint(HCRT_CRT_SCREEN_TYPE);
   const uint crt_resolution        = uint(HCRT_CRT_RESOLUTION);
   const uint lcd_resolution        = uint(HCRT_LCD_RESOLUTION);
   const uint lcd_subpixel_layout   = uint(HCRT_LCD_SUBPIXEL);
   const vec2 source_size           = global.SourceSize.xy;
   const vec2 output_size           = global.OutputSize.xy;

   vec2 tex_coord                   = vTexCoord - vec2(0.5f);
   tex_coord                        = tex_coord * vec2(1.0f + (HCRT_PIN_PHASE * tex_coord.y), 1.0f);
   tex_coord                        = tex_coord * vec2(HCRT_H_SIZE, HCRT_V_SIZE);
   tex_coord                        = tex_coord + vec2(0.5f);
   tex_coord                        = tex_coord + (vec2(HCRT_H_CENT, HCRT_V_CENT) / output_size); 

   const vec2 current_position      = vTexCoord * output_size;

   uint colour_mask = 0;

   switch(screen_type)
   {
      case kApertureGrille:
      {
         uint mask = uint(floor(mod(current_position.x, kApertureGrilleMaskSize[(lcd_resolution * kTVLAxis) + crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     
                     #undef kRGBX           
                     #undef kRBGX           
                     #undef kBGRX  
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kMG               (kMagenta << 0) | (kGreen   << 6)  
                     #define kYB               (kYellow  << 0) | (kBlue    << 6)
                     #define kGM               (kGreen   << 0) | (kMagenta << 6)
                     
                     const uint rgb_mask[kBGRAxis] = { kMG, kYB, kGM };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;

                     #undef kMG             
                     #undef kYB             
                     #undef kGM

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kWhite;

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kWhite;

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k4K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  {
                     #define kRRGGBBX          (kRed  << 0) | (kRed  << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20) | (kBlack << 24)   
                     #define kRRBBGGX          (kRed  << 0) | (kRed  << 4) | (kBlue  << 8) | (kBlue  << 12) | (kGreen << 16) | (kGreen << 20) | (kBlack << 24)    
                     #define kBBGGRRX          (kBlue << 0) | (kBlue << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20) | (kBlack << 24)    
                     
                     const uint rgb_mask[kBGRAxis] = {  kRRGGBBX, kRRBBGGX, kBBGGRRX };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;  
                     
                     #undef kRRGGBBX        
                     #undef kRRBBGGX       
                     #undef kBBGGRRX  
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     
                     #undef kRGBX           
                     #undef kRBGX           
                     #undef kBGRX  

                     break;
                  }
                  case k800TVL:
                  {
                     #define kBGR              (kBlue  << 0) | (kGreen << 4) | (kRed  << 8) 
                     #define kGBR              (kGreen << 0) | (kBlue  << 4) | (kRed  << 8)  
                     #define kRGB              (kRed   << 0) | (kGreen << 4) | (kBlue << 8)  
                     
                     const uint rgb_mask[kBGRAxis] = { kBGR, kGBR, kRGB };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   

                     #undef kRGB            
                     #undef kGBR            
                     #undef kBGR  

                     break;
                  }
                  case k1000TVL:
                  {                     
                     #define kMG               (kMagenta << 0) | (kGreen   << 6)  
                     #define kYB               (kYellow  << 0) | (kBlue    << 6)
                     #define kGM               (kGreen   << 0) | (kMagenta << 6)
                     
                     const uint rgb_mask[kBGRAxis] = { kMG, kYB, kGM };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;

                     #undef kMG             
                     #undef kYB             
                     #undef kGM 

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k8K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kMaxApertureGrilleSize       13
                     
                     #define kRRRRGGGGBBBBX    kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kBlack  
                     #define kRRRRBBBBGGGGX    kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kBlack  
                     #define kBBBBGGGGRRRRX    kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed, kBlack  
                     
                     const uint kApertureGrilleMasks8K300TVL[kBGRAxis * kMaxApertureGrilleSize] = 
                     { 
                        kRRRRGGGGBBBBX, kRRRRBBBBGGGGX, kBBBBGGGGRRRRX
                     };

                     colour_mask = kApertureGrilleMasks8K300TVL[(lcd_subpixel_layout * kMaxApertureGrilleSize) + mask];  
                     
                     #undef kMaxApertureGrilleSize 
                     
                     #undef kRRRRGGGGBBBBX  
                     #undef kRRRRBBBBGGGGX  
                     #undef kBBBBGGGGRRRRX      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kRRGGBBX          (kRed  << 0) | (kRed  << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20) | (kBlack << 24)   
                     #define kRRBBGGX          (kRed  << 0) | (kRed  << 4) | (kBlue  << 8) | (kBlue  << 12) | (kGreen << 16) | (kGreen << 20) | (kBlack << 24)    
                     #define kBBGGRRX          (kBlue << 0) | (kBlue << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20) | (kBlack << 24)    
                     
                     const uint rgb_mask[kBGRAxis] = { kRRGGBBX, kRRBBGGX, kBBGGRRX };

                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;  
                     
                     #undef kRRGGBBX        
                     #undef kRRBBGGX       
                     #undef kBBGGRRX 

                     break;
                  }
                  case k800TVL:
                  {                     
                     #define kRYCBX            (kRed  << 0) | (kYellow  << 6) | (kCyan   << 12) | (kBlue  << 18) | (kBlack << 24)  
                     #define kRMCGX            (kRed  << 0) | (kMagenta << 6) | (kCyan   << 12) | (kGreen << 18) | (kBlack << 24)  
                     #define kBCYRX            (kBlue << 0) | (kCyan    << 6) | (kYellow << 12) | (kRed   << 18) | (kBlack << 24)  
                     
                     const uint rgb_mask[kBGRAxis] = { kRYCBX, kRMCGX, kBCYRX  };
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;
                     
                     #undef kRYCBX          
                     #undef kRMCGX          
                     #undef kBCYRX   

                     break;
                  }
                  case k1000TVL:
                  {
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                     
                     colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     
                     #undef kRGBX           
                     #undef kRBGX           
                     #undef kBGRX      

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            default:
            {
               break;
            }                 
         }

         break;
      }
      case kShadowMask:
      {
         uint shadow_y = uint(floor(mod(current_position.y, kShadowMaskSizeY[(lcd_resolution * kTVLAxis) + crt_resolution])));

         uint mask = uint(floor(mod(current_position.x, kShadowMaskSizeX[(lcd_resolution * kTVLAxis) + crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kGRRBBG              (kGreen << 0) | (kRed   << 4) | (kRed   << 8) | (kBlue  << 12) | (kBlue  << 16) | (kGreen << 20)
                     #define kBBGGRR              (kBlue  << 0) | (kBlue  << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kBRRGGB              (kBlue  << 0) | (kRed   << 4) | (kRed   << 8) | (kGreen << 12) | (kGreen << 16) | (kBlue  << 20)
                     #define kGGBBRR              (kGreen << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlue  << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kGBBRRG              (kGreen << 0) | (kBlue  << 4) | (kBlue  << 8) | (kRed   << 12) | (kRed   << 16) | (kGreen << 20)
                     #define kRRGGBB              (kRed   << 0) | (kRed   << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20)
                     
                     /*
                        kGRRBBG
                        kGRRBBG
                        kBBGGRR
                        kBBGGRR

                        kBRRGGB
                        kBRRGGB
                        kGGBBRR
                        kGGBBRR

                        kGBBRRG
                        kGBBRRG
                        kRRGGBB
                        kRRGGBB
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kGRRBBG, kBBGGRR, kBRRGGB, kGGBBRR, kGBBRRG, kRRGGBB };

                     if(shadow_y < 2)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 4)) & 0xF;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 4)) & 0xF;
                     }
                     
                     #undef kGRRBBG           
                     #undef kBBGGRR    
                     
                     #undef kBRRGGB            
                     #undef kGGBBRR      
                     
                     #undef kGBBRRG            
                     #undef kRRGGBB   
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kMG                  (kMagenta << 0) | (kGreen   << 6)  
                     #define kGM                  (kGreen   << 0) | (kMagenta << 6)  
                     
                     #define kYB                  (kYellow  << 0) | (kBlue    << 6)  
                     #define kBY                  (kBlue    << 0) | (kYellow  << 6)  
                     
                     /*
                        kMG
                        kGM  
                        
                        kYB
                        kBY  
                        
                        kGM
                        kMG
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kMG, kGM, kYB, kBY, kGM, kMG };

                     if(shadow_y < 1)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 6)) & 0x3F;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 6)) & 0x3F;
                     }

                     #undef kMG               
                     #undef kGM    
                     
                     #undef kYB                
                     #undef kBY   

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kWhite; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kWhite; 

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k4K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kMaxShadowMaskSizeX     12
                     #define kMaxShadowMaskSizeY     8
                     
                     #define kGGRRRRBBBBGG        kGreen, kGreen, kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen  
                     #define kBBBBGGGGRRRR        kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed  
                     
                     #define kBBRRRRGGGGBB        kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue  
                     #define kGGGGBBBBRRRR        kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed  
                     
                     #define kGGBBBBRRRRGG        kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen  
                     #define kRRRRGGGGBBBB        kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue  
                     
                     /*
                        kGGRRRRBBBBGG
                        kGGRRRRBBBBGG
                        kGGRRRRBBBBGG
                        kGGRRRRBBBBGG
                        kBBBBGGGGRRRR
                        kBBBBGGGGRRRR
                        kBBBBGGGGRRRR
                        kBBBBGGGGRRRR

                        kBBRRRRGGGGBB
                        kBBRRRRGGGGBB
                        kBBRRRRGGGGBB
                        kBBRRRRGGGGBB
                        kGGGGBBBBRRRR
                        kGGGGBBBBRRRR
                        kGGGGBBBBRRRR
                        kGGGGBBBBRRRR

                        kGGBBBBRRRRGG
                        kGGBBBBRRRRGG
                        kGGBBBBRRRRGG
                        kGGBBBBRRRRGG
                        kRRRRGGGGBBBB
                        kRRRRGGGGBBBB
                        kRRRRGGGGBBBB
                        kRRRRGGGGBBBB
                     */

                     if(shadow_y < 4)
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kGGRRRRBBBBGG, kBBRRRRGGGGBB, kGGBBBBRRRRGG };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kBBBBGGGGRRRR, kGGGGBBBBRRRR, kRRRRGGGGBBBB };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }

                     #undef kMaxShadowMaskSizeX 
                     #undef kMaxShadowMaskSizeY 
                     
                     #undef kGGRRRRBBBBGG      
                     #undef kBBBBGGGGRRRR   
                     
                     #undef kBBRRRRGGGGBB     
                     #undef kGGGGBBBBRRRR    
                     
                     #undef kGGBBBBRRRRGG      
                     #undef kRRRRGGGGBBBB 

                     break;
                  }
                  case k600TVL:
                  {                     
                     #define kGRRBBG              (kGreen << 0) | (kRed   << 4) | (kRed   << 8) | (kBlue  << 12) | (kBlue  << 16) | (kGreen << 20)
                     #define kBBGGRR              (kBlue  << 0) | (kBlue  << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kBRRGGB              (kBlue  << 0) | (kRed   << 4) | (kRed   << 8) | (kGreen << 12) | (kGreen << 16) | (kBlue  << 20)
                     #define kGGBBRR              (kGreen << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlue  << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kGBBRRG              (kGreen << 0) | (kBlue  << 4) | (kBlue  << 8) | (kRed   << 12) | (kRed   << 16) | (kGreen << 20)
                     #define kRRGGBB              (kRed   << 0) | (kRed   << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20)
                     
                     /*
                        kGRRBBG
                        kGRRBBG
                        kBBGGRR
                        kBBGGRR

                        kBRRGGB
                        kBRRGGB
                        kGGBBRR
                        kGGBBRR

                        kGBBRRG
                        kGBBRRG
                        kRRGGBB
                        kRRGGBB
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kGRRBBG, kBBGGRR, kBRRGGB, kGGBBRR, kGBBRRG, kRRGGBB };

                     if(shadow_y < 2)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 4)) & 0xF;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 4)) & 0xF;
                     }
                     
                     #undef kGRRBBG           
                     #undef kBBGGRR    
                     
                     #undef kBRRGGB            
                     #undef kGGBBRR      
                     
                     #undef kGBBRRG            
                     #undef kRRGGBB     

                     break;
                  }
                  case k800TVL:
                  {                     
                     #define kMG                  (kMagenta << 0) | (kGreen   << 6)  
                     #define kGM                  (kGreen   << 0) | (kMagenta << 6)  
                     
                     #define kYB                  (kYellow  << 0) | (kBlue    << 6)  
                     #define kBY                  (kBlue    << 0) | (kYellow  << 6)  
                     
                     /*
                        kMG
                        kGM  
                        
                        kYB
                        kBY  
                        
                        kGM
                        kMG
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kMG, kGM, kYB, kBY, kGM, kMG };

                     if(shadow_y < 1)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 6)) & 0x3F;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 6)) & 0x3F;
                     }

                     #undef kMG               
                     #undef kGM    
                     
                     #undef kYB                
                     #undef kBY  

                     break;
                  }
                  case k1000TVL:
                  {                    
                     #define kMG                  (kMagenta << 0) | (kGreen   << 6)  
                     #define kGM                  (kGreen   << 0) | (kMagenta << 6)  
                     
                     #define kYB                  (kYellow  << 0) | (kBlue    << 6)  
                     #define kBY                  (kBlue    << 0) | (kYellow  << 6)  
                     
                     /*
                        kMG
                        kGM  
                        
                        kYB
                        kBY  
                        
                        kGM
                        kMG
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kMG, kGM, kYB, kBY, kGM, kMG };

                     if(shadow_y < 1)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 6)) & 0x3F;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 6)) & 0x3F;
                     }

                     #undef kMG               
                     #undef kGM    
                     
                     #undef kYB                
                     #undef kBY  

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k8K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kMaxShadowMaskSizeX     12
                     #define kMaxShadowMaskSizeY     8
                     
                     #define kGGRRRRBBBBGG        kGreen, kGreen, kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen  
                     #define kBBBBGGGGRRRR        kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed  
                     
                     #define kBBRRRRGGGGBB        kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue  
                     #define kGGGGBBBBRRRR        kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed  
                     
                     #define kGGBBBBRRRRGG        kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen  
                     #define kRRRRGGGGBBBB        kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue  

                     /*
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR

                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR

                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                     */

                     if(shadow_y < 4)
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kGGRRRRBBBBGG, kBBRRRRGGGGBB, kGGBBBBRRRRGG };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kBBBBGGGGRRRR, kGGGGBBBBRRRR, kRRRRGGGGBBBB };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }
                     
                     #undef kMaxShadowMaskSizeX 
                     #undef kMaxShadowMaskSizeY 
                     
                     #undef kGGRRRRBBBBGG      
                     #undef kBBBBGGGGRRRR      
                     
                     #undef kBBRRRRGGGGBB     
                     #undef kGGGGBBBBRRRR      
                     
                     #undef kGGBBBBRRRRGG      
                     #undef kRRRRGGGGBBBB 

                     break;
                  }
                  case k600TVL:
                  {
                     #define kMaxShadowMaskSizeX     12
                     #define kMaxShadowMaskSizeY     8
                     
                     #define kGGRRRRBBBBGG        kGreen, kGreen, kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen  
                     #define kBBBBGGGGRRRR        kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed  
                     
                     #define kBBRRRRGGGGBB        kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue  
                     #define kGGGGBBBBRRRR        kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed  
                     
                     #define kGGBBBBRRRRGG        kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen  
                     #define kRRRRGGGGBBBB        kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue  
                     
                     /*
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kGGRRRRBBBBGG
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR
                         kBBBBGGGGRRRR

                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kBBRRRRGGGGBB
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR
                         kGGGGBBBBRRRR

                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kGGBBBBRRRRGG
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                         kRRRRGGGGBBBB
                     */

                     if(shadow_y < 4)
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kGGRRRRBBBBGG, kBBRRRRGGGGBB, kGGBBBBRRRRGG };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis * kMaxShadowMaskSizeX] = { kBBBBGGGGRRRR, kGGGGBBBBRRRR, kRRRRGGGGBBBB };
                        colour_mask = rgb_mask[(lcd_subpixel_layout * kMaxShadowMaskSizeX) + mask];
                     }
                     
                     #undef kMaxShadowMaskSizeX 
                     #undef kMaxShadowMaskSizeY 
                     
                     #undef kGGRRRRBBBBGG      
                     #undef kBBBBGGGGRRRR      
                     
                     #undef kBBRRRRGGGGBB     
                     #undef kGGGGBBBBRRRR      
                     
                     #undef kGGBBBBRRRRGG      
                     #undef kRRRRGGGGBBBB

                     break;
                  }
                  case k800TVL:
                  {
                     #define kGRRBBG              (kGreen << 0) | (kRed   << 4) | (kRed   << 8) | (kBlue  << 12) | (kBlue  << 16) | (kGreen << 20)
                     #define kBBGGRR              (kBlue  << 0) | (kBlue  << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kBRRGGB              (kBlue  << 0) | (kRed   << 4) | (kRed   << 8) | (kGreen << 12) | (kGreen << 16) | (kBlue  << 20)
                     #define kGGBBRR              (kGreen << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlue  << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kGBBRRG              (kGreen << 0) | (kBlue  << 4) | (kBlue  << 8) | (kRed   << 12) | (kRed   << 16) | (kGreen << 20)
                     #define kRRGGBB              (kRed   << 0) | (kRed   << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20)
                     
                     /*
                        kGRRBBG
                        kGRRBBG
                        kBBGGRR
                        kBBGGRR

                        kBRRGGB
                        kBRRGGB
                        kGGBBRR
                        kGGBBRR

                        kGBBRRG
                        kGBBRRG
                        kRRGGBB
                        kRRGGBB
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kGRRBBG, kBBGGRR, kBRRGGB, kGGBBRR, kGBBRRG, kRRGGBB };

                     if(shadow_y < 2)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 4)) & 0xF;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 4)) & 0xF;
                     }
                     
                     #undef kGRRBBG           
                     #undef kBBGGRR    
                     
                     #undef kBRRGGB            
                     #undef kGGBBRR      
                     
                     #undef kGBBRRG            
                     #undef kRRGGBB 
                     
                     break;
                  }
                  case k1000TVL:
                  {
                     #define kGRRBBG              (kGreen << 0) | (kRed   << 4) | (kRed   << 8) | (kBlue  << 12) | (kBlue  << 16) | (kGreen << 20)
                     #define kBBGGRR              (kBlue  << 0) | (kBlue  << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kBRRGGB              (kBlue  << 0) | (kRed   << 4) | (kRed   << 8) | (kGreen << 12) | (kGreen << 16) | (kBlue  << 20)
                     #define kGGBBRR              (kGreen << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlue  << 12) | (kRed   << 16) | (kRed   << 20)
                     
                     #define kGBBRRG              (kGreen << 0) | (kBlue  << 4) | (kBlue  << 8) | (kRed   << 12) | (kRed   << 16) | (kGreen << 20)
                     #define kRRGGBB              (kRed   << 0) | (kRed   << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20)
                     
                     /*
                        kGRRBBG
                        kGRRBBG
                        kBBGGRR
                        kBBGGRR

                        kBRRGGB
                        kBRRGGB
                        kGGBBRR
                        kGGBBRR

                        kGBBRRG
                        kGBBRRG
                        kRRGGBB
                        kRRGGBB
                     */

                     const uint rgb_mask[kBGRAxis * 2] = { kGRRBBG, kBBGGRR, kBRRGGB, kGGBBRR, kGBBRRG, kRRGGBB };

                     if(shadow_y < 2)
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 0] >> (mask * 4)) & 0xF;
                     }
                     else
                     {
                         colour_mask = (rgb_mask[(lcd_subpixel_layout * 2) + 1] >> (mask * 4)) & 0xF;
                     }
                     
                     #undef kGRRBBG           
                     #undef kBBGGRR    
                     
                     #undef kBRRGGB            
                     #undef kGGBBRR      
                     
                     #undef kGBBRRG            
                     #undef kRRGGBB 
                     
                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            default:
            {
               break;
            }                 
         }

         break;
      }
      case kSlotMask:
      {
         #define kMaxSlotSizeX      2

         uint slot_x = uint(floor(mod(current_position.x / kSlotMaskSizeX[(lcd_resolution * kTVLAxis) + crt_resolution], kMaxSlotSizeX)));
         uint slot_y = uint(floor(mod(current_position.y, kSlotMaskSizeY[(lcd_resolution * kTVLAxis) + crt_resolution])));
         
         uint element = (slot_y * kMaxSlotSizeX) + slot_x;

         uint mask = uint(floor(mod(current_position.x, kSlotMaskSizeX[(lcd_resolution * kTVLAxis) + crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  {
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     /*
                         kRGBX, kRGBX
                         kRGBX, kXXXX
                         kRGBX, kRGBX
                         kXXXX, kRGBX

                         kRBGX, kRBGX
                         kRBGX, kXXXX
                         kRBGX, kRBGX
                         kXXXX, kRBGX  

                         kBGRX, kBGRX
                         kBGRX, kXXXX
                         kBGRX, kBGRX
                         kXXXX, kBGRX
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     }
                     
                     #undef kRGBX     
                     #undef kRBGX
                     #undef kBGRX         

                     break;
                  }
                  case k600TVL:
                  {
                     #define kMG               (kMagenta << 0) | (kGreen   << 6)  
                     #define kYB               (kYellow  << 0) | (kBlue    << 6)
                     #define kGM               (kGreen   << 0) | (kMagenta << 6)
                     
                     /*
                        kMG, kMG
                        kMG, kXX
                        kMG, kMG
                        kXX, kMG

                        kYB, kYB
                        kYB, kXX
                        kYB, kYB
                        kXX, kYB

                        kGM, kGM
                        kGM, kXX
                        kGM, kGM
                        kXX, kGM
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kMG, kYB, kGM };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;
                     }
                   
                     #undef kMG     
                     #undef kYB  
                     #undef kGM        

                     break;
                  }
                  case k800TVL:
                  {
                     #define kMaxSlotMaskSize   1
                     #define kMaxSlotSizeY      4
                     
                     #define kX          kBlack  
                     #define kW          kWhite  
                     
                     /*
                        kW, kW
                        kW, kX
                        kW, kW
                        kX, kW
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        colour_mask = kWhite;
                     }
                      
                     #undef kMaxSlotMaskSize  
                     #undef kMaxSlotSizeY     
                     
                     #undef kX
                     #undef kW

                     break;
                  }
                  case k1000TVL:
                  {
                     #define kMaxSlotMaskSize   1
                     #define kMaxSlotSizeY      4
                     
                     #define kX          kBlack  
                     #define kW          kWhite  

                     /*
                        kW, kW
                        kW, kX
                        kW, kW
                        kX, kW
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        colour_mask = kWhite;
                     }
                     
                     #undef kMaxSlotMaskSize  
                     #undef kMaxSlotSizeY     
                     
                     #undef kX  
                     #undef kW   

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k4K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kRRGGBBX          (kRed  << 0) | (kRed  << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20) | (kBlack << 24)   
                     #define kRRBBGGX          (kRed  << 0) | (kRed  << 4) | (kBlue  << 8) | (kBlue  << 12) | (kGreen << 16) | (kGreen << 20) | (kBlack << 24)    
                     #define kBBGGRRX          (kBlue << 0) | (kBlue << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20) | (kBlack << 24)    
                     
                     /*
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kXXXXXXX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kXXXXXXX, kRRGGBBX

                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kXXXXXXX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kXXXXXXX, kRRBBGGX

                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kXXXXXXX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kXXXXXXX, kBBGGRRX
                     */

                     if((element == 7) || (element == 14))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = {  kRRGGBBX, kRRBBGGX, kBBGGRRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;  
                     }
                     
                     #undef kRRGGBBX  
                     #undef kRRBBGGX  
                     #undef kBBGGRRX 
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     /*
                         kRGBX, kRGBX
                         kRGBX, kXXXX
                         kRGBX, kRGBX
                         kXXXX, kRGBX

                         kRBGX, kRBGX
                         kRBGX, kXXXX
                         kRBGX, kRBGX
                         kXXXX, kRBGX  

                         kBGRX, kBGRX
                         kBGRX, kXXXX
                         kBGRX, kBGRX
                         kXXXX, kBGRX
                     */

                     if((element == 5) || (element == 10))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     }
                     
                     #undef kRGBX     
                     #undef kRBGX
                     #undef kBGRX

                     break;
                  }
                  case k800TVL:
                  {
                     #define kBGR              (kBlue  << 0) | (kGreen << 4) | (kRed  << 8) 
                     #define kGBR              (kGreen << 0) | (kBlue  << 4) | (kRed  << 8)  
                     #define kRGB              (kRed   << 0) | (kGreen << 4) | (kBlue << 8)   
                     
                     /*
                        kBGR, kBGR
                        kBGR, kXXX
                        kBGR, kBGR
                        kXXX, kBGR
                        
                        kGBR, kGBR
                        kGBR, kXXX
                        kGBR, kGBR
                        kXXX, kGBR
                        
                        kRGB, kRGB
                        kRGB, kXXX
                        kRGB, kRGB
                        kXXX, kRGB
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kBGR, kGBR, kRGB };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;  
                     }  
                     
                     #undef kBGR    
                     #undef kGBR 
                     #undef kRGB       

                     break;
                  }
                  case k1000TVL:
                  {
                     #define kMG               (kMagenta << 0) | (kGreen   << 6)  
                     #define kYB               (kYellow  << 0) | (kBlue    << 6)
                     #define kGM               (kGreen   << 0) | (kMagenta << 6)
                     
                     /*
                        kMG, kMG
                        kMG, kXX
                        kMG, kMG
                        kXX, kMG

                        kYB, kYB
                        kYB, kXX
                        kYB, kYB
                        kXX, kYB

                        kGM, kGM
                        kGM, kXX
                        kGM, kGM
                        kXX, kGM
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kMG, kYB, kGM };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;
                     }
                   
                     #undef kMG     
                     #undef kYB  
                     #undef kGM        

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            case k8K:
            {
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     #define kRRGGBBX          (kRed  << 0) | (kRed  << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20) | (kBlack << 24)   
                     #define kRRBBGGX          (kRed  << 0) | (kRed  << 4) | (kBlue  << 8) | (kBlue  << 12) | (kGreen << 16) | (kGreen << 20) | (kBlack << 24)    
                     #define kBBGGRRX          (kBlue << 0) | (kBlue << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20) | (kBlack << 24) 
                     
                     /*
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kXXXXXXX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kXXXXXXX, kRRGGBBX
                     
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kXXXXXXX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kXXXXXXX, kRRBBGGX
                     
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kXXXXXXX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kXXXXXXX, kBBGGRRX
                     */

                     if((element == 5) || (element == 10))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = {  kRRGGBBX, kRRBBGGX, kBBGGRRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF; 
                     }   
                     
                     #undef kRRGGBBX  
                     #undef kRRBBGGX  
                     #undef kBBGGRRX 
                     
                     break;
                  }
                  case k600TVL:
                  {
                     #define kRRGGBBX          (kRed  << 0) | (kRed  << 4) | (kGreen << 8) | (kGreen << 12) | (kBlue  << 16) | (kBlue  << 20) | (kBlack << 24)   
                     #define kRRBBGGX          (kRed  << 0) | (kRed  << 4) | (kBlue  << 8) | (kBlue  << 12) | (kGreen << 16) | (kGreen << 20) | (kBlack << 24)    
                     #define kBBGGRRX          (kBlue << 0) | (kBlue << 4) | (kGreen << 8) | (kGreen << 12) | (kRed   << 16) | (kRed   << 20) | (kBlack << 24) 
                     
                     /*
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kXXXXXXX
                         kRRGGBBX, kRRGGBBX
                         kRRGGBBX, kRRGGBBX
                         kXXXXXXX, kRRGGBBX
                     
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kXXXXXXX
                         kRRBBGGX, kRRBBGGX
                         kRRBBGGX, kRRBBGGX
                         kXXXXXXX, kRRBBGGX
                     
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kXXXXXXX
                         kBBGGRRX, kBBGGRRX
                         kBBGGRRX, kBBGGRRX
                         kXXXXXXX, kBBGGRRX
                     */

                     if((element == 5) || (element == 10))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = {  kRRGGBBX, kRRBBGGX, kBBGGRRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF; 
                     }

                     #undef kMaxSlotMaskSize   
                     #undef kMaxSlotSizeY 
                     
                     #undef kRRGGBBX  
                     #undef kRRBBGGX  
                     #undef kBBGGRRX 

                     break;
                  }
                  case k800TVL:
                  {
                     #define kRYCBX            (kRed  << 0) | (kYellow  << 6) | (kCyan   << 12) | (kBlue  << 18) | (kBlack << 24)  
                     #define kRMCGX            (kRed  << 0) | (kMagenta << 6) | (kCyan   << 12) | (kGreen << 18) | (kBlack << 24)  
                     #define kBCYRX            (kBlue << 0) | (kCyan    << 6) | (kYellow << 12) | (kRed   << 18) | (kBlack << 24)  
                     
                     /*
                        kRYCBX, kRYCBX
                        kRYCBX, kXXXXX
                        kRYCBX, kRYCBX
                        kXXXXX, kRYCBX    

                        kRMCGX, kRMCGX
                        kRMCGX, kXXXXX
                        kRMCGX, kRMCGX
                        kXXXXX, kRMCGX    

                        kBCYRX, kBCYRX
                        kBCYRX, kXXXXX
                        kBCYRX, kBCYRX
                        kXXXXX, kBCYRX    
                     */

                     if((element == 3) || (element == 6))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kRYCBX, kRMCGX, kBCYRX  };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 6)) & 0x3F;
                     }
                     
                     #undef kRYCBX    
                     #undef kRMCGX
                     #undef kBCYRX

                     break;
                  }
                  case k1000TVL:
                  {
                     #define kRGBX             (kRed  << 0) | (kGreen << 4) | (kBlue  << 8) | (kBlack << 12)  
                     #define kRBGX             (kRed  << 0) | (kBlue  << 4) | (kGreen << 8) | (kBlack << 12)  
                     #define kBGRX             (kBlue << 0) | (kGreen << 4) | (kRed   << 8) | (kBlack << 12)  
                     
                     /*
                         kRGBX, kRGBX
                         kRGBX, kXXXX
                         kRGBX, kRGBX
                         kXXXX, kRGBX

                         kRBGX, kRBGX
                         kRBGX, kXXXX
                         kRBGX, kRBGX
                         kXXXX, kRBGX  

                         kBGRX, kBGRX
                         kBGRX, kXXXX
                         kBGRX, kBGRX
                         kXXXX, kBGRX
                     */

                     if((element == 5) || (element == 10))
                     {
                        colour_mask = kBlack;
                     }
                     else
                     {
                        const uint rgb_mask[kBGRAxis] = { kRGBX, kRBGX, kBGRX };
                        colour_mask = (rgb_mask[lcd_subpixel_layout] >> (mask * 4)) & 0xF;   
                     }
                     
                     #undef kRGBX     
                     #undef kRBGX
                     #undef kBGRX

                     break;
                  }
                  default:
                  {
                     break;
                  }                 
               }

               break;
            }
            default:
            {
               break;
            }                 
         }

         break;
      }
      default:
      {
         break;
      }
   }

   const float scanline_size           = output_size.y / source_size.y;

   const vec3 horizontal_convergence   = vec3(HCRT_RED_HORIZONTAL_CONVERGENCE, HCRT_GREEN_HORIZONTAL_CONVERGENCE, HCRT_BLUE_HORIZONTAL_CONVERGENCE);
   const vec3 vertical_convergence     = vec3(HCRT_RED_VERTICAL_CONVERGENCE, HCRT_GREEN_VERTICAL_CONVERGENCE, HCRT_BLUE_VERTICAL_CONVERGENCE);
   const vec3 beam_sharpness           = vec3(HCRT_RED_BEAM_SHARPNESS, HCRT_GREEN_BEAM_SHARPNESS, HCRT_BLUE_BEAM_SHARPNESS);
   const vec3 beam_attack              = vec3(HCRT_RED_BEAM_ATTACK, HCRT_GREEN_BEAM_ATTACK, HCRT_BLUE_BEAM_ATTACK);
   const vec3 scanline_min             = vec3(HCRT_RED_SCANLINE_MIN, HCRT_GREEN_SCANLINE_MIN, HCRT_BLUE_SCANLINE_MIN);
   const vec3 scanline_max             = vec3(HCRT_RED_SCANLINE_MAX, HCRT_GREEN_SCANLINE_MAX, HCRT_BLUE_SCANLINE_MAX);
   const vec3 scanline_attack          = vec3(HCRT_RED_SCANLINE_ATTACK, HCRT_GREEN_SCANLINE_ATTACK, HCRT_BLUE_SCANLINE_ATTACK);

   const uint channel_count            = colour_mask & 3;

   vec3 scanline_colour = vec3(0.0f);

   if(channel_count > 0)
   {
      const uint channel_0             = (colour_mask >> kFirstChannelShift) & 3;

      const float scanline_channel_0   = GenerateScanline(  channel_0,
                                                            tex_coord,
                                                            source_size.xy, 
                                                            scanline_size, 
                                                            horizontal_convergence[channel_0], 
                                                            vertical_convergence[channel_0], 
                                                            beam_sharpness[channel_0], 
                                                            beam_attack[channel_0], 
                                                            scanline_min[channel_0], 
                                                            scanline_max[channel_0], 
                                                            scanline_attack[channel_0]);

      scanline_colour =  scanline_channel_0 * kColourMask[channel_0];
   }

   if(channel_count > 1)
   {
      const uint channel_1             = (colour_mask >> kSecondChannelShift) & 3;

      const float scanline_channel_1   = GenerateScanline(channel_1,
                                                          tex_coord,
                                                          source_size.xy, 
                                                          scanline_size, 
                                                          horizontal_convergence[channel_1], 
                                                          vertical_convergence[channel_1], 
                                                          beam_sharpness[channel_1], 
                                                          beam_attack[channel_1], 
                                                          scanline_min[channel_1], 
                                                          scanline_max[channel_1], 
                                                          scanline_attack[channel_1]);

      scanline_colour += scanline_channel_1 * kColourMask[channel_1];
   }

   if(channel_count > 2)
   {
      const uint channel_2             = (colour_mask >> kThirdChannelShift) & 3;

      const float scanline_channel_2   = GenerateScanline(channel_2,
                                                          tex_coord,
                                                          source_size.xy, 
                                                          scanline_size, 
                                                          horizontal_convergence[channel_2], 
                                                          vertical_convergence[channel_2], 
                                                          beam_sharpness[channel_2], 
                                                          beam_attack[channel_2], 
                                                          scanline_min[channel_2], 
                                                          scanline_max[channel_2], 
                                                          scanline_attack[channel_2]);

      scanline_colour += scanline_channel_2 * kColourMask[channel_2];
   }

   vec3 transformed_colour;

   if(HCRT_COLOUR_ACCURATE >= 1.0f)
   {
      if(HCRT_HDR >= 1.0f)
      {
         const vec3 rec2020  = scanline_colour * k2020Gamuts[uint(HCRT_EXPAND_GAMUT)];
         transformed_colour  = rec2020 * (HCRT_PAPER_WHITE_NITS / kMaxNitsFor2084);
      }
      else if(HCRT_OUTPUT_COLOUR_SPACE == 2.0f)
      {
         transformed_colour = (scanline_colour * k709_to_XYZ) * kXYZ_to_DCIP3; 
      }
      else
      {
         transformed_colour = scanline_colour;
      }
   } 
   else
   {      
      transformed_colour = scanline_colour;
   }

   vec3 gamma_corrected; 
   
   GammaCorrect(transformed_colour, gamma_corrected);

   FragColor = vec4(gamma_corrected, 1.0f);
}
