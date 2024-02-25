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

#define ENABLE_BLACK_WHITE_MASKS 0

// HSM Removed, For the Mega Bezel this is not the final pass
// #pragma format A2B10G10R10_UNORM_PACK32

layout(push_constant) uniform Push
{
   // User Settings
   float hcrt_hdr;
   float hcrt_colour_space;
   float hcrt_max_nits;
   float hcrt_paper_white_nits;
   float hcrt_expand_gamut;
   float hcrt_gamma_out;
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

/* HSM Removed
layout(std140, set = 0, binding = 0) uniform UBO
{
	mat4 MVP;
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;

   float hcrt_h_size;
   float hcrt_v_size;
   float hcrt_h_cent;
   float hcrt_v_cent;
   float hcrt_pin_phase;
   float hcrt_pin_amp;
} global;
*/

#include "include/parameters.h"

#define HCRT_HDR                             params.hcrt_hdr
#define HCRT_OUTPUT_COLOUR_SPACE             params.hcrt_colour_space
#define HCRT_MAX_NITS                        params.hcrt_max_nits
#define HCRT_PAPER_WHITE_NITS                params.hcrt_paper_white_nits
#define HCRT_EXPAND_GAMUT                    params.hcrt_expand_gamut
#define HCRT_GAMMA_OUT                       params.hcrt_gamma_out
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

/* HSM Removed
#define COMPAT_TEXTURE(c, d) texture(c, d)
*/

// HSM Added
#define COMPAT_TEXTURE(c,d) HSM_GetCroppedTexSample(c,d)
// End Addition

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
layout(set = 0, binding = 4) uniform sampler2D InfoCachePass;
layout(set = 0, binding = 5) uniform sampler2D AvgLumPass;


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

const float kApertureGrilleMaskSize[kResolutionAxis][kTVLAxis] = { 
   { 4.0f, 2.0f, 1.0f, 1.0f },      // 1080p:   300 TVL, 600 TVL, 800 TVL, 1000 TVL 
   { 7.0f, 4.0f, 3.0f, 2.0f },      // 4K:      300 TVL, 600 TVL, 800 TVL, 1000 TVL   
   { 13.0f, 7.0f, 5.0f, 4.0f } };   // 8K:      300 TVL, 600 TVL, 800 TVL, 1000 TVL


// 1080p 

// 300TVL
#define kMaxApertureGrilleSize       4

#define kRGBX           { kRed, kGreen, kBlue, kBlack }
#define kRBGX           { kRed, kBlue, kGreen, kBlack }
#define kBGRX           { kBlue, kGreen, kRed, kBlack }

const uint kApertureGrilleMasks1080p300TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{
   kRGBX, kRBGX, kBGRX
};

#undef kMaxApertureGrilleSize

#undef kRGBX           
#undef kRBGX           
#undef kBGRX  

// 600TVL
#define kMaxApertureGrilleSize       2

#define kMG             { kMagenta, kGreen }
#define kYB             { kYellow, kBlue }
#define kGM             { kGreen, kMagenta }

const uint kApertureGrilleMasks1080p600TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{
   kMG, kYB, kGM
};

#undef kMaxApertureGrilleSize

#undef kMG             
#undef kYB             
#undef kGM             

// 800TVL
#define kMaxApertureGrilleSize       1

#define kW              { kWhite }

const uint kApertureGrilleMasks1080p800TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{
   kW, kW, kW
};

// 1000TVL
const uint kApertureGrilleMasks1080p1000TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{
   kW, kW, kW
};

#undef kMaxApertureGrilleSize   

#undef kW              


// 4K 

// 300TVL
#define kMaxApertureGrilleSize       7

#define kRRGGBBX        { kRed, kRed, kGreen, kGreen, kBlue, kBlue, kBlack }
#define kRRBBGGX        { kRed, kRed, kBlue, kBlue, kGreen, kGreen, kBlack }
#define kBBGGRRX        { kBlue, kBlue, kGreen, kGreen, kRed, kRed, kBlack }

const uint kApertureGrilleMasks4K300TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRRGGBBX, kRRBBGGX, kBBGGRRX
};

#undef kMaxApertureGrilleSize 

#undef kRRGGBBX        
#undef kRRBBGGX       
#undef kBBGGRRX  

// 600TVL
#define kMaxApertureGrilleSize       4

#define kRGBX           { kRed, kGreen, kBlue, kBlack }
#define kRBGX           { kRed, kBlue, kGreen, kBlack }
#define kBGRX           { kBlue, kGreen, kRed, kBlack }

const uint kApertureGrilleMasks4K600TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRGBX, kRBGX, kBGRX 
};

#undef kMaxApertureGrilleSize 

#undef kRGBX           
#undef kRBGX           
#undef kBGRX             

// 800TVL
#define kMaxApertureGrilleSize       3

#define kRGB            { kRed, kGreen, kBlue }
#define kGBR            { kGreen, kBlue, kRed }
#define kBGR            { kBlue, kGreen, kRed }

const uint kApertureGrilleMasks4K800TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kBGR, kGBR, kRGB 
};

#undef kMaxApertureGrilleSize 

#undef kRGB            
#undef kGBR            
#undef kBGR            

// 1000TVL
#define kMaxApertureGrilleSize       2

#define kMG             { kMagenta, kGreen }
#define kYB             { kYellow, kBlue }
#define kGM             { kGreen, kMagenta }

const uint kApertureGrilleMasks4K1000TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kMG, kYB, kGM
};

#undef kMaxApertureGrilleSize  

#undef kMG             
#undef kYB             
#undef kGM             


// 8K 

// 300 TVL
#define kMaxApertureGrilleSize       13

#define kRRRRGGGGBBBBX  { kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kBlack }
#define kRRRRBBBBGGGGX  { kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kBlack }
#define kBBBBGGGGRRRRX  { kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed, kBlack }

const uint kApertureGrilleMasks8K300TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRRRRGGGGBBBBX, kRRRRBBBBGGGGX, kBBBBGGGGRRRRX
};

#undef kMaxApertureGrilleSize 

#undef kRRRRGGGGBBBBX  
#undef kRRRRBBBBGGGGX  
#undef kBBBBGGGGRRRRX  

// 600 TVL
#define kMaxApertureGrilleSize       7

#define kRRGGBBX        { kRed, kRed, kGreen, kGreen, kBlue, kBlue, kBlack }
#define kRRBBGGX        { kRed, kRed, kBlue, kBlue, kGreen, kGreen, kBlack }
#define kBBGGRRX        { kBlue, kBlue, kGreen, kGreen, kRed, kRed, kBlack }

const uint kApertureGrilleMasks8K600TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRRGGBBX, kRRBBGGX, kBBGGRRX 
};

#undef kMaxApertureGrilleSize 

#undef kRRGGBBX        
#undef kRRBBGGX       
#undef kBBGGRRX        

// 800 TVL
#define kMaxApertureGrilleSize       5

#define kRYCBX          { kRed, kYellow, kCyan, kBlue, kBlack }
#define kRMCGX          { kRed, kMagenta, kCyan, kGreen, kBlack }
#define kBCYRX          { kBlue, kCyan, kYellow, kRed, kBlack }

const uint kApertureGrilleMasks8K800TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRYCBX, kRMCGX, kBCYRX
};

#undef kMaxApertureGrilleSize 

#undef kRYCBX          
#undef kRMCGX          
#undef kBCYRX          

// 1000 TVL
#define kMaxApertureGrilleSize       4

#define kRGBX           { kRed, kGreen, kBlue, kBlack }
#define kRBGX           { kRed, kBlue, kGreen, kBlack }
#define kBGRX           { kBlue, kGreen, kRed, kBlack }

const uint kApertureGrilleMasks8K1000TVL[kBGRAxis][kMaxApertureGrilleSize] = 
{ 
   kRGBX, kRBGX, kBGRX 
};

#undef kMaxApertureGrilleSize  

#undef kRGBX           
#undef kRBGX           
#undef kBGRX            


// SHADOW MASKS

const float kShadowMaskSizeX[kResolutionAxis][kTVLAxis] = { { 6.0f, 2.0f, 1.0f, 1.0f }, { 12.0f, 6.0f, 2.0f, 2.0f }, { 12.0f, 12.0f, 6.0f, 6.0f } }; 
const float kShadowMaskSizeY[kResolutionAxis][kTVLAxis] = { { 4.0f, 2.0f, 1.0f, 1.0f }, {  8.0f, 4.0f, 2.0f, 2.0f }, {  8.0f,  8.0f, 4.0f, 4.0f } }; 


// 1080p 


#define kXXXX              { kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }

// 300 TVL
#define kMaxShadowMaskSizeX     6
#define kMaxShadowMaskSizeY     4

#define kGRRBBG            { kGreen, kRed, kRed, kBlue, kBlue, kGreen }
#define kBBGGRR            { kBlue, kBlue, kGreen, kGreen, kRed, kRed }

#define kBRRGGB            { kBlue, kRed, kRed, kGreen, kGreen, kBlue }
#define kGGBBRR            { kGreen, kGreen, kBlue, kBlue, kRed, kRed }

#define kGBBRRG            { kGreen, kBlue, kBlue, kRed, kRed, kGreen }
#define kRRGGBB            { kRed, kRed, kGreen, kGreen, kBlue, kBlue }

#define kGRRBBG_GRRBBG_BBGGRR_BBGGRR  { kGRRBBG, kGRRBBG, kBBGGRR, kBBGGRR }
#define kBRRGGB_BRRGGB_GGBBRR_GGBBRR  { kBRRGGB, kBRRGGB, kGGBBRR, kGGBBRR }
#define kGBBRRG_GBBRRG_RRGGBB_RRGGBB  { kGBBRRG, kGBBRRG, kRRGGBB, kRRGGBB }

const uint kShadowMasks1080p300TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGRRBBG_GRRBBG_BBGGRR_BBGGRR, kBRRGGB_BRRGGB_GGBBRR_GGBBRR, kGBBRRG_GBBRRG_RRGGBB_RRGGBB        // 300 TVL
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kGRRBBG           
#undef kBBGGRR       
    
#undef kBRRGGB            
#undef kGGBBRR       

#undef kGBBRRG            
#undef kRRGGBB            

#undef kGRRBBG_GRRBBG_BBGGRR_BBGGRR  
#undef kBRRGGB_BRRGGB_GGBBRR_GGBBRR  
#undef kGBBRRG_GBBRRG_RRGGBB_RRGGBB 


// 600 TVL
#define kMaxShadowMaskSizeX     2
#define kMaxShadowMaskSizeY     2

#define kMG                { kMagenta, kGreen }
#define kGM                { kGreen, kMagenta }

#define kYB                { kYellow, kBlue }
#define kBY                { kBlue, kYellow }

#define kMG_GM             { kMG, kGM }
#define kYB_BY             { kYB, kBY }
#define kGM_MG             { kGM, kMG }

const uint kShadowMasks1080p600TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kMG_GM, kYB_BY, kGM_MG                                                                          // 600 TVL
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kMG               
#undef kGM    

#undef kYB                
#undef kBY           

#undef kMG_GM             
#undef kYB_BY             
#undef kGM_MG             

// 800 TVL
#define kMaxShadowMaskSizeX     1
#define kMaxShadowMaskSizeY     1

#define kW                 { kWhite }
#define kW_W               { kW }

const uint kShadowMasks1080p800TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kW_W, kW_W, kW_W                                                                                // 800 TVL
};

// 1000 TVL
const uint kShadowMasks1080p1000TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kW_W, kW_W, kW_W                                                                                // 1000 TVL
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kW       
#undef kW_W


// 4K

// 300 TVL

#define kMaxShadowMaskSizeX     12
#define kMaxShadowMaskSizeY     8

#define kGGRRRRBBBBGG      { kGreen, kGreen, kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen }
#define kBBBBGGGGRRRR      { kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed }

#define kBBRRRRGGGGBB      { kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue }
#define kGGGGBBBBRRRR      { kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed }

#define kGGBBBBRRRRGG      { kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen }
#define kRRRRGGGGBBBB      { kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue }

#define kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR  { kGGRRRRBBBBGG, kGGRRRRBBBBGG, kGGRRRRBBBBGG, kGGRRRRBBBBGG, kBBBBGGGGRRRR, kBBBBGGGGRRRR, kBBBBGGGGRRRR, kBBBBGGGGRRRR }
#define kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR  { kBBRRRRGGGGBB, kBBRRRRGGGGBB, kBBRRRRGGGGBB, kBBRRRRGGGGBB, kGGGGBBBBRRRR, kGGGGBBBBRRRR, kGGGGBBBBRRRR, kGGGGBBBBRRRR }
#define kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB  { kGGBBBBRRRRGG, kGGBBBBRRRRGG, kGGBBBBRRRRGG, kGGBBBBRRRRGG, kRRRRGGGGBBBB, kRRRRGGGGBBBB, kRRRRGGGGBBBB, kRRRRGGGGBBBB }

const uint kShadowMasks4K300TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR, 
   kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR,
   kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB                                                                                         
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kGGRRRRBBBBGG      
#undef kBBBBGGGGRRRR   

#undef kBBRRRRGGGGBB     
#undef kGGGGBBBBRRRR    

#undef kGGBBBBRRRRGG      
#undef kRRRRGGGGBBBB      

#undef kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR  
#undef kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR  
#undef kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB  


// 600 TVL

#define kMaxShadowMaskSizeX     6
#define kMaxShadowMaskSizeY     4

#define kGRRBBG            { kGreen, kRed, kRed, kBlue, kBlue, kGreen }
#define kBBGGRR            { kBlue, kBlue, kGreen, kGreen, kRed, kRed }

#define kBRRGGB            { kBlue, kRed, kRed, kGreen, kGreen, kBlue }
#define kGGBBRR            { kGreen, kGreen, kBlue, kBlue, kRed, kRed }

#define kGBBRRG            { kGreen, kBlue, kBlue, kRed, kRed, kGreen }
#define kRRGGBB            { kRed, kRed, kGreen, kGreen, kBlue, kBlue }

#define kGRRBBG_GRRBBG_BBGGRR_BBGGRR  { kGRRBBG, kGRRBBG, kBBGGRR, kBBGGRR }
#define kBRRGGB_BRRGGB_GGBBRR_GGBBRR  { kBRRGGB, kBRRGGB, kGGBBRR, kGGBBRR }
#define kGBBRRG_GBBRRG_RRGGBB_RRGGBB  { kGBBRRG, kGBBRRG, kRRGGBB, kRRGGBB }

const uint kShadowMasks4K600TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGRRBBG_GRRBBG_BBGGRR_BBGGRR, kBRRGGB_BRRGGB_GGBBRR_GGBBRR, kGBBRRG_GBBRRG_RRGGBB_RRGGBB
};

#undef kGRRBBG_GRRBBG_BBGGRR_BBGGRR  
#undef kBRRGGB_BRRGGB_GGBBRR_GGBBRR  
#undef kGBBRRG_GBBRRG_RRGGBB_RRGGBB 

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kGRRBBG           
#undef kBBGGRR    

#undef kBRRGGB            
#undef kGGBBRR      

#undef kGBBRRG            
#undef kRRGGBB         


// 800 TVL

#define kMaxShadowMaskSizeX     2
#define kMaxShadowMaskSizeY     2

#define kMG                { kMagenta, kGreen }
#define kGM                { kGreen, kMagenta }

#define kYB                { kYellow, kBlue }
#define kBY                { kBlue, kYellow }

#define kMG_GM             { kMG, kGM }
#define kYB_BY             { kYB, kBY }
#define kGM_MG             { kGM, kMG }

const uint kShadowMasks4K800TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kMG_GM, kYB_BY, kGM_MG
};

// 1000 TVL
const uint kShadowMasks4K1000TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kMG_GM, kYB_BY, kGM_MG
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kXXXX     

#undef kMG               
#undef kGM    

#undef kYB                
#undef kBY         

#undef kMG_GM             
#undef kYB_BY             
#undef kGM_MG         


// 8K 

// 300 TVL
#define kMaxShadowMaskSizeX     12
#define kMaxShadowMaskSizeY     8

#define kGGRRRRBBBBGG      { kGreen, kGreen, kRed, kRed, kRed, kRed, kBlue, kBlue, kBlue, kBlue, kGreen, kGreen }
#define kBBBBGGGGRRRR      { kBlue, kBlue, kBlue, kBlue, kGreen, kGreen, kGreen, kGreen, kRed, kRed, kRed, kRed }

#define kBBRRRRGGGGBB      { kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue }
#define kGGGGBBBBRRRR      { kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed }

#define kGGBBBBRRRRGG      { kGreen, kGreen, kBlue, kBlue, kBlue, kBlue, kRed, kRed, kRed, kRed, kGreen, kGreen }
#define kRRRRGGGGBBBB      { kRed, kRed, kRed, kRed, kGreen, kGreen, kGreen, kGreen, kBlue, kBlue, kBlue, kBlue }

#define kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR  { kGGRRRRBBBBGG, kGGRRRRBBBBGG, kGGRRRRBBBBGG, kGGRRRRBBBBGG, kBBBBGGGGRRRR, kBBBBGGGGRRRR, kBBBBGGGGRRRR, kBBBBGGGGRRRR }
#define kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR  { kBBRRRRGGGGBB, kBBRRRRGGGGBB, kBBRRRRGGGGBB, kBBRRRRGGGGBB, kGGGGBBBBRRRR, kGGGGBBBBRRRR, kGGGGBBBBRRRR, kGGGGBBBBRRRR }
#define kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB  { kGGBBBBRRRRGG, kGGBBBBRRRRGG, kGGBBBBRRRRGG, kGGBBBBRRRRGG, kRRRRGGGGBBBB, kRRRRGGGGBBBB, kRRRRGGGGBBBB, kRRRRGGGGBBBB }

const uint kShadowMasks8K300TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR,
   kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR,
   kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB                    
};

// 600 TVL
const uint kShadowMasks8K600TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR, 
   kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR,
   kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kGGRRRRBBBBGG      
#undef kBBBBGGGGRRRR      

#undef kBBRRRRGGGGBB     
#undef kGGGGBBBBRRRR      

#undef kGGBBBBRRRRGG      
#undef kRRRRGGGGBBBB      

#undef kGGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_GGRRRRBBBBGG_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR_BBBBGGGGRRRR  
#undef kBBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_BBRRRRGGGGBB_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR_GGGGBBBBRRRR  
#undef kGGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_GGBBBBRRRRGG_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB_RRRRGGGGBBBB  

// 800 TVL
#define kMaxShadowMaskSizeX     6
#define kMaxShadowMaskSizeY     4

#define kGRRBBG            { kGreen, kRed, kRed, kBlue, kBlue, kGreen }
#define kBBGGRR            { kBlue, kBlue, kGreen, kGreen, kRed, kRed }

#define kBRRGGB            { kBlue, kRed, kRed, kGreen, kGreen, kBlue }
#define kGGBBRR            { kGreen, kGreen, kBlue, kBlue, kRed, kRed }

#define kGBBRRG            { kGreen, kBlue, kBlue, kRed, kRed, kGreen }
#define kRRGGBB            { kRed, kRed, kGreen, kGreen, kBlue, kBlue }

#define kGRRBBG_GRRBBG_BBGGRR_BBGGRR  { kGRRBBG, kGRRBBG, kBBGGRR, kBBGGRR }
#define kBRRGGB_BRRGGB_GGBBRR_GGBBRR  { kBRRGGB, kBRRGGB, kGGBBRR, kGGBBRR }
#define kGBBRRG_GBBRRG_RRGGBB_RRGGBB  { kGBBRRG, kGBBRRG, kRRGGBB, kRRGGBB }

const uint kShadowMasks8K800TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGRRBBG_GRRBBG_BBGGRR_BBGGRR, kBRRGGB_BRRGGB_GGBBRR_GGBBRR, kGBBRRG_GBBRRG_RRGGBB_RRGGBB
};

// 1000 TVL
const uint kShadowMasks8K1000TVL[kBGRAxis][kMaxShadowMaskSizeY][kMaxShadowMaskSizeX]  = 
{
   kGRRBBG_GRRBBG_BBGGRR_BBGGRR, kBRRGGB_BRRGGB_GGBBRR_GGBBRR, kGBBRRG_GBBRRG_RRGGBB_RRGGBB
};

#undef kMaxShadowMaskSizeX 
#undef kMaxShadowMaskSizeY 

#undef kGRRBBG           
#undef kBBGGRR            

#undef kBRRGGB            
#undef kGGBBRR            

#undef kGBBRRG            
#undef kRRGGBB         

#undef kGRRBBG_GRRBBG_BBGGRR_BBGGRR  
#undef kBRRGGB_BRRGGB_GGBBRR_GGBBRR  
#undef kGBBRRG_GBBRRG_RRGGBB_RRGGBB 

// SLOT MASKS

#define kMaxSlotSizeX      2

const float kSlotMaskSizeX[kResolutionAxis][kTVLAxis] = { { 4.0f, 2.0f, 1.0f, 1.0f }, { 7.0f, 4.0f, 3.0f, 2.0f }, { 7.0f, 7.0f, 5.0f, 4.0f } }; //1080p: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   4K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   8K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL
const float kSlotMaskSizeY[kResolutionAxis][kTVLAxis] = { { 4.0f, 4.0f, 1.0f, 1.0f }, { 8.0f, 6.0f, 4.0f, 4.0f }, { 6.0f, 6.0f, 4.0f, 4.0f } }; //1080p: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   4K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   8K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL


// 1080p 


// 300 TVL
#define kMaxSlotMaskSize   4
#define kMaxSlotSizeY      4

#define kXXXX     { kBlack, kBlack, kBlack, kBlack }

#define kRGBX     { kRed, kGreen, kBlue, kBlack }
#define kRBGX     { kRed, kBlue, kGreen, kBlack }
#define kBGRX     { kBlue, kGreen, kRed, kBlack }

#define kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   { { kRGBX, kRGBX }, { kRGBX, kXXXX }, { kRGBX, kRGBX }, { kXXXX, kRGBX } }
#define kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   { { kRBGX, kRBGX }, { kRBGX, kXXXX }, { kRBGX, kRBGX }, { kXXXX, kRBGX } }
#define kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX   { { kBGRX, kBGRX }, { kBGRX, kXXXX }, { kBGRX, kBGRX }, { kXXXX, kBGRX } }

const uint kSlotMasks1080p300TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX, kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX, kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX                                                                                       
};

#undef kMaxSlotMaskSize  
#undef kMaxSlotSizeY     

#undef kXXXX

#undef kRGBX     
#undef kRBGX
#undef kBGRX     

#undef kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   
#undef kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   
#undef kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX     


// 600 TVL
#define kMaxSlotMaskSize   2
#define kMaxSlotSizeY      4

#define kXX     { kBlack, kBlack }

#define kMG       { kMagenta, kGreen }
#define kYB       { kYellow, kBlue }
#define kGM       { kGreen, kMagenta }

#define kMGMG_MGXX_MGMG_XXMG   { { kMG, kMG }, { kMG, kXX }, { kMG, kMG }, { kXX, kMG } }
#define kYBYB_YBXX_YBYB_XXYB   { { kYB, kYB }, { kYB, kXX }, { kYB, kYB }, { kXX, kYB } }
#define kGMGM_GMXX_GMGM_XXGM   { { kGM, kGM }, { kGM, kXX }, { kGM, kGM }, { kXX, kGM } }

const uint kSlotMasks1080p600TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kMGMG_MGXX_MGMG_XXMG, kYBYB_YBXX_YBYB_XXYB, kGMGM_GMXX_GMGM_XXGM
};

#undef kMaxSlotMaskSize  
#undef kMaxSlotSizeY     

#undef kXX

#undef kMG     
#undef kYB  
#undef kGM        

#undef kMGMG_MGXX_MGMG_XXMG  
#undef kYBYB_YBXX_YBYB_XXYB   
#undef kGMGM_GMXX_GMGM_XXGM  


// 800 TVL
#define kMaxSlotMaskSize   1
#define kMaxSlotSizeY      4

#define kX        { kBlack }
#define kW        { kWhite }

#define kW_W_W_W   { { kW, kW }, { kW, kX }, { kW, kW }, { kX, kW } }

const uint kSlotMasks1080p800TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kW_W_W_W, kW_W_W_W, kW_W_W_W
};

// 1000 TVL
const uint kSlotMasks1080p1000TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kW_W_W_W, kW_W_W_W, kW_W_W_W 
};

#undef kMaxSlotMaskSize  
#undef kMaxSlotSizeY     

#undef kX  
#undef kW   

#undef kW_W_W_W 


// 4K

// 300 TVL
#define kMaxSlotMaskSize   7
#define kMaxSlotSizeY      8

#define kXXXX     { kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }

#define kRRGGBBX  { kRed, kRed, kGreen, kGreen, kBlue, kBlue, kBlack }
#define kRRBBGGX  { kRed, kRed, kBlue, kBlue, kGreen, kGreen, kBlack }
#define kBBGGRRX  { kBlue, kBlue, kGreen, kGreen, kRed, kRed, kBlack }

#define kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX   { { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kXXXX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kXXXX, kRRGGBBX } }
#define kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX   { { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kXXXX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kXXXX, kRRBBGGX } }
#define kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX   { { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kXXXX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kXXXX, kBBGGRRX } }

const uint kSlotMasks4K300TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX, kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX, kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX    
  
#undef kRRGGBBX  
#undef kRRBBGGX  
#undef kBBGGRRX 

#undef kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX   
#undef kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX   
#undef kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX   


// 600 TVL
#define kMaxSlotMaskSize   4
#define kMaxSlotSizeY      6

#define kXXXX     { kBlack, kBlack, kBlack, kBlack }

#define kRGBX     { kRed, kGreen, kBlue, kBlack }
#define kRBGX     { kRed, kBlue, kGreen, kBlack }
#define kBGRX     { kBlue, kGreen, kRed, kBlack }

#define kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   { { kRGBX, kRGBX }, { kRGBX, kRGBX }, { kRGBX, kXXXX }, { kRGBX, kRGBX }, { kRGBX, kRGBX }, { kXXXX, kRGBX }}
#define kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   { { kRBGX, kRBGX }, { kRBGX, kRBGX }, { kRBGX, kXXXX }, { kRBGX, kRBGX }, { kRBGX, kRBGX }, { kXXXX, kRBGX }}
#define kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX   { { kBGRX, kBGRX }, { kBGRX, kBGRX }, { kBGRX, kXXXX }, { kBGRX, kBGRX }, { kBGRX, kBGRX }, { kXXXX, kBGRX }}

const uint kSlotMasks4K600TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX, kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX, kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX     

#undef kRGBX     
#undef kRBGX
#undef kBGRX

#undef kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   
#undef kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   
#undef kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX   


// 800 TVL
#define kMaxSlotMaskSize   3
#define kMaxSlotSizeY      4

#define kXXXX     { kBlack, kBlack, kBlack }

#define kBGR      { kBlue, kGreen, kRed }
#define kGBR      { kGreen, kBlue, kRed }
#define kRGB      { kRed, kGreen, kBlue }

#define kBGRBGR_BGRXXX_BGRBGR_XXXBGR   { { kBGR, kBGR }, { kBGR, kXXXX }, { kBGR, kBGR }, { kXXXX, kBGR } }
#define kGBRGBR_GBRXXX_GBRGBR_XXXGBR   { { kGBR, kGBR }, { kGBR, kXXXX }, { kGBR, kGBR }, { kXXXX, kGBR } }
#define kRGBRGB_RGBXXX_RGBRGB_XXXRGB   { { kRGB, kRGB }, { kRGB, kXXXX }, { kRGB, kRGB }, { kXXXX, kRGB } }

const uint kSlotMasks4K800TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kBGRBGR_BGRXXX_BGRBGR_XXXBGR, kGBRGBR_GBRXXX_GBRGBR_XXXGBR, kRGBRGB_RGBXXX_RGBRGB_XXXRGB
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX     

#undef kBGR    
#undef kGBR 
#undef kRGB      

#undef kBGRBGR_BGRXXX_BGRBGR_XXXBGR   
#undef kGBRGBR_GBRXXX_GBRGBR_XXXGBR   
#undef kRGBRGB_RGBXXX_RGBRGB_XXXRGB   


// 1000 TVL
#define kMaxSlotMaskSize   2
#define kMaxSlotSizeY      4

#define kXX     { kBlack, kBlack }

#define kMG       { kMagenta, kGreen }
#define kYB       { kYellow, kBlue }
#define kGM       { kGreen, kMagenta }

#define kMGMG_MGXX_MGMG_XXMG   { { kMG, kMG }, { kMG, kXX }, { kMG, kMG }, { kXX, kMG } }
#define kYBYB_YBXX_YBYB_XXYB   { { kYB, kYB }, { kYB, kXX }, { kYB, kYB }, { kXX, kYB } }
#define kGMGM_GMXX_GMGM_XXGM   { { kGM, kGM }, { kGM, kXX }, { kGM, kGM }, { kXX, kGM } }

const uint kSlotMasks4K1000TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kMGMG_MGXX_MGMG_XXMG, kYBYB_YBXX_YBYB_XXYB, kGMGM_GMXX_GMGM_XXGM
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXX     

#undef kMG     
#undef kYB  
#undef kGM        

#undef kMGMG_MGXX_MGMG_XXMG  
#undef kYBYB_YBXX_YBYB_XXYB   
#undef kGMGM_GMXX_GMGM_XXGM   
#undef kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX   


// 8K

// 300 TVL
#define kMaxSlotMaskSize   7
#define kMaxSlotSizeY      6

#define kXXXX     { kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }

#define kRRGGBBX  { kRed, kRed, kGreen, kGreen, kBlue, kBlue, kBlack }
#define kRRBBGGX  { kRed, kRed, kBlue, kBlue, kGreen, kGreen, kBlack }
#define kBBGGRRX  { kBlue, kBlue, kGreen, kGreen, kRed, kRed, kBlack }

#define kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX   { { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kXXXX }, { kRRGGBBX, kRRGGBBX }, { kRRGGBBX, kRRGGBBX }, { kXXXX, kRRGGBBX } }
#define kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX   { { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kXXXX }, { kRRBBGGX, kRRBBGGX }, { kRRBBGGX, kRRBBGGX }, { kXXXX, kRRBBGGX } }
#define kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX   { { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kXXXX }, { kBBGGRRX, kBBGGRRX }, { kBBGGRRX, kBBGGRRX }, { kXXXX, kBBGGRRX } }

const uint kSlotMasks8K300TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX, kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX, kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX 
};

// 600 TVL
const uint kSlotMasks8K600TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX, kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX, kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX 
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX    

#undef kRRGGBBX  
#undef kRRBBGGX  
#undef kBBGGRRX 

#undef kRRGGBBXRRGGBBX_RRGGBBXRRGGBBX_RRGGBBXXXXX_RRGGBBXRRGGBBX_RRGGBBXRRGGBBX_XXXXRRGGBBX   
#undef kRRBBGGXRRBBGGX_RRBBGGXRRBBGGX_RRBBGGXXXXX_RRBBGGXRRBBGGX_RRBBGGXRRBBGGX_XXXXRRBBGGX   
#undef kBBGGRRXBBGGRRX_BBGGRRXBBGGRRX_BBGGRRXXXXX_BBGGRRXBBGGRRX_BBGGRRXBBGGRRX_XXXXBBGGRRX   

// 800 TVL
#define kMaxSlotMaskSize   5
#define kMaxSlotSizeY      4

#define kXXXX     { kBlack, kBlack, kBlack, kBlack, kBlack }

#define kRYCBX    { kRed, kYellow, kCyan, kBlue, kBlack }
#define kRMCGX    { kRed, kMagenta, kCyan, kGreen, kBlack }
#define kBCYRX    { kBlue, kCyan, kYellow, kRed, kBlack }

#define kRYCBXRYCBX_RYCBXXXXX_RYCBXRYCBX_XXXXRYCBX   { { kRYCBX, kRYCBX }, { kRYCBX, kXXXX }, { kRYCBX, kRYCBX }, { kXXXX, kRYCBX } }
#define kRMCGXRMCGX_RMCGXXXXX_RMCGXRMCGX_XXXXRMCGX   { { kRMCGX, kRMCGX }, { kRMCGX, kXXXX }, { kRMCGX, kRMCGX }, { kXXXX, kRMCGX } }
#define kBCYRXBCYRX_BCYRXXXXX_BCYRXBCYRX_XXXXBCYRX   { { kBCYRX, kBCYRX }, { kBCYRX, kXXXX }, { kBCYRX, kBCYRX }, { kXXXX, kBCYRX } }

const uint kSlotMasks8K800TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRYCBXRYCBX_RYCBXXXXX_RYCBXRYCBX_XXXXRYCBX, kRMCGXRMCGX_RMCGXXXXX_RMCGXRMCGX_XXXXRMCGX, kBCYRXBCYRX_BCYRXXXXX_BCYRXBCYRX_XXXXBCYRX
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX    

#undef kRYCBX    
#undef kRMCGX
#undef kBCYRX    

#undef kRYCBXRYCBX_RYCBXXXXX_RYCBXRYCBX_XXXXRYCBX   
#undef kRMCGXRMCGX_RMCGXXXXX_RMCGXRMCGX_XXXXRMCGX   
#undef kBCYRXBCYRX_BCYRXXXXX_BCYRXBCYRX_XXXXBCYRX   


// 1000 TVL
#define kMaxSlotMaskSize   4
#define kMaxSlotSizeY      4

#define kXXXX     { kBlack, kBlack, kBlack, kBlack }

#define kRGBX     { kRed, kGreen, kBlue, kBlack }
#define kRBGX     { kRed, kBlue, kGreen, kBlack }
#define kBGRX     { kBlue, kGreen, kRed, kBlack }

#define kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   { { kRGBX, kRGBX }, { kRGBX, kXXXX }, { kRGBX, kRGBX }, { kXXXX, kRGBX } }
#define kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   { { kRBGX, kRBGX }, { kRBGX, kXXXX }, { kRBGX, kRBGX }, { kXXXX, kRBGX } }
#define kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX   { { kBGRX, kBGRX }, { kBGRX, kXXXX }, { kBGRX, kBGRX }, { kXXXX, kBGRX } }

const uint kSlotMasks8K1000TVL[kBGRAxis][kMaxSlotSizeY][kMaxSlotSizeX][kMaxSlotMaskSize] = 
{
   kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX, kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX, kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX 
};

#undef kMaxSlotMaskSize   
#undef kMaxSlotSizeY   

#undef kXXXX     

#undef kRGBX    
#undef kRBGX  
#undef kBGRX     
 
#undef kRGBXRGBX_RGBXXXXX_RGBXRGBX_XXXXRGBX   
#undef kRBGXRBGX_RBGXXXXX_RBGXRBGX_XXXXRBGX   
#undef kBGRXBGRX_BGRXXXXX_BGRXBGRX_XXXXBGRX   

// BLACK WHITE MASKS
#if ENABLE_BLACK_WHITE_MASKS

#define kMaxBlackWhiteSize       14

#define kW                 { kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }

#define kWX                { kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }
#define kWWX               { kWhite, kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }
#define kWWXX              { kWhite, kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }
#define kWWWWX             { kWhite, kWhite, kWhite, kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }
#define kWWWWWXX           { kWhite, kWhite, kWhite, kWhite, kWhite, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack, kBlack }
#define kWWWWWWWWWWWXXX    { kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite, kWhite /*kBlack, kBlack, kBlack*/ }

const float kBlackWhiteMaskSize[kResolutionAxis][kTVLAxis] = { { 4.0f, 2.0f, 1.0f, 1.0f }, { 7.0f, 4.0f, 3.0f, 2.0f }, { 14.0f, 7.0f, 5.0f, 4.0f } }; //4K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL   8K: 300 TVL, 600 TVL, 800 TVL, 1000 TVL

const uint kBlackWhiteMasks[kResolutionAxis][kTVLAxis][kBGRAxis][kMaxBlackWhiteSize] = {
   { // 1080p
      { kWWXX, kWWXX, kWWXX },                                 // 300 TVL
      { kWX, kWX, kWX },                                       // 600 TVL
      { kW, kW, kW },                                          // 800 TVL
      { kW, kW, kW }                                           // 1000 TVL
   },
   { // 4K
      { kWWWWWXX, kWWWWWXX, kWWWWWXX },                        // 300 TVL
      { kWWXX, kWWXX, kWWXX },                                 // 600 TVL
      { kWWX, kWWX, kWWX },                                    // 800 TVL
      { kWX, kWX, kWX }                                        // 1000 TVL
   },
   { // 8K
      { kWWWWWWWWWWWXXX, kWWWWWWWWWWWXXX, kWWWWWWWWWWWXXX },   // 300 TVL
      { kWWWWWXX, kWWWWWXX, kWWWWWXX },                        // 600 TVL
      { kWWWWX, kWWWWX, kWWWWX },                              // 800 TVL
      { kWWXX, kWWXX, kWWXX }                                  // 1000 TVL
   }
};

#undef kW                
#undef kWX                
#undef kWWX               
#undef kWWXX              
#undef kWWWWX             
#undef kWWWWWXX           
#undef kWWWWWWWWWWWXXX 

#endif // ENABLE_BLACK_WHITE_MASKS

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
// HSM Added
	vec2 viewportCoordTransformed = HSM_GetViewportCoordWithZoomAndPan(vTexCoord);
	HSM_UpdateGlobalScreenValuesFromCache(InfoCachePass, vTexCoord);

	vec2 cache_bounds_coord = SCREEN_COORD;

// If it's the potato preset render the whole frame
#ifndef IS_POTATO_PRESET
#ifndef IS_NO_REFLECT_PRESET
	vec2 bezel_outside_flat_coord;
	vec2 frame_outside_flat_coord;
	HSM_GetSimpleBezelCoords(TUBE_DIFFUSE_COORD, 
							TUBE_DIFFUSE_SCALE, 
							TUBE_SCALE, 
							SCREEN_ASPECT,
							bezel_outside_flat_coord, 
							frame_outside_flat_coord);
	cache_bounds_coord = (bezel_outside_flat_coord - 0.5) * 0.9 + 0.5;
#endif
#endif

	if (HHLP_IsOutsideCoordSpace(cache_bounds_coord))
	{
			FragColor = vec4(0);
			return;
	}

	// HSM Added
	float raster_bloom_avg_lum = texture(AvgLumPass, vec2(0.5,0.5)).a;
	vec2 screen_curved_coord = vec2(0.5);

	vec2 screen_curved_coord_with_overscan_and_mirror = HSM_GetCrtShaderFinalCoord(SCREEN_COORD,
		SCREEN_SCALE,
		raster_bloom_avg_lum,
		screen_curved_coord);

	vec2 screen_final_coord = screen_curved_coord_with_overscan_and_mirror;
   // End HSM Added

   const uint screen_type           = uint(HCRT_CRT_SCREEN_TYPE);
   const uint crt_resolution        = uint(HCRT_CRT_RESOLUTION);
   const uint lcd_resolution        = uint(HCRT_LCD_RESOLUTION);
   const uint lcd_subpixel_layout   = uint(HCRT_LCD_SUBPIXEL);
   const vec2 source_size           = CROPPED_ROTATED_SIZE_WITH_RES_MULT;
   const vec2 output_size           = global.OutputSize.xy;

   vec2 tex_coord                   = screen_final_coord - vec2(0.5f);
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
         uint mask = uint(floor(mod(current_position.x, kApertureGrilleMaskSize[lcd_resolution][crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     colour_mask = kApertureGrilleMasks1080p300TVL[lcd_subpixel_layout][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kApertureGrilleMasks1080p600TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kApertureGrilleMasks1080p800TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kApertureGrilleMasks1080p1000TVL[lcd_subpixel_layout][mask]; 

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
                     colour_mask = kApertureGrilleMasks4K300TVL[lcd_subpixel_layout][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kApertureGrilleMasks4K600TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kApertureGrilleMasks4K800TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kApertureGrilleMasks4K1000TVL[lcd_subpixel_layout][mask]; 

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
                     colour_mask = kApertureGrilleMasks8K300TVL[lcd_subpixel_layout][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kApertureGrilleMasks8K600TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kApertureGrilleMasks8K800TVL[lcd_subpixel_layout][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kApertureGrilleMasks8K1000TVL[lcd_subpixel_layout][mask]; 

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
         uint shadow_y = uint(floor(mod(current_position.y, kShadowMaskSizeY[lcd_resolution][crt_resolution])));

         uint mask = uint(floor(mod(current_position.x, kShadowMaskSizeX[lcd_resolution][crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     colour_mask = kShadowMasks1080p300TVL[lcd_subpixel_layout][shadow_y][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kShadowMasks1080p600TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kShadowMasks1080p800TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kShadowMasks1080p1000TVL[lcd_subpixel_layout][shadow_y][mask]; 

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
                     colour_mask = kShadowMasks4K300TVL[lcd_subpixel_layout][shadow_y][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kShadowMasks4K600TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kShadowMasks4K800TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kShadowMasks4K1000TVL[lcd_subpixel_layout][shadow_y][mask]; 

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
                     colour_mask = kShadowMasks8K300TVL[lcd_subpixel_layout][shadow_y][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kShadowMasks8K600TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kShadowMasks8K800TVL[lcd_subpixel_layout][shadow_y][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kShadowMasks8K1000TVL[lcd_subpixel_layout][shadow_y][mask]; 

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
         uint slot_x = uint(floor(mod(current_position.x / kSlotMaskSizeX[lcd_resolution][crt_resolution], kMaxSlotSizeX)));
         uint slot_y = uint(floor(mod(current_position.y, kSlotMaskSizeY[lcd_resolution][crt_resolution])));

         uint mask = uint(floor(mod(current_position.x, kSlotMaskSizeX[lcd_resolution][crt_resolution])));

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               switch(crt_resolution)
               {
                  case k300TVL:
                  { 
                     colour_mask = kSlotMasks1080p300TVL[lcd_subpixel_layout][slot_y][slot_x][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kSlotMasks1080p600TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kSlotMasks1080p800TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kSlotMasks1080p1000TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

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
                     colour_mask = kSlotMasks4K300TVL[lcd_subpixel_layout][slot_y][slot_x][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kSlotMasks4K600TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kSlotMasks4K800TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kSlotMasks4K1000TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

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
                     colour_mask = kSlotMasks8K300TVL[lcd_subpixel_layout][slot_y][slot_x][mask];      
                     
                     break;
                  }
                  case k600TVL:
                  {
                     colour_mask = kSlotMasks8K600TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k800TVL:
                  {
                     colour_mask = kSlotMasks8K800TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

                     break;
                  }
                  case k1000TVL:
                  {
                     colour_mask = kSlotMasks8K1000TVL[lcd_subpixel_layout][slot_y][slot_x][mask]; 

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
#if ENABLE_BLACK_WHITE_MASKS
      case kBlackWhiteMask:
      {
         uint mask = uint(floor(mod(current_position.x, kBlackWhiteMaskSize[lcd_resolution][crt_resolution])));

         colour_mask = kBlackWhiteMasks[lcd_resolution][crt_resolution][lcd_subpixel_layout][mask];      

         switch(lcd_resolution)
         {
            case k1080p:
            { 
               break;
            }
            case k4K:
            {
               break;
            }
            case k8K:
            {
               break;
            }
            default:
            {
               break;
            }                 
         }

         break;
      }
#endif // ENABLE_BLACK_WHITE_MASKS
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

/*
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
*/

   scanline_colour = pow(scanline_colour, vec3(1 / (HSM_GAMMA_OUT_CRT / GAMMA_INPUT)));
   FragColor = vec4(scanline_colour, 1.0f);

/*
   vec3 output_colour = vec3(0);
   LinearToOutputColor(scanline_colour, output_colour);
   FragColor = vec4(output_colour, 1.0f);
*/
}
