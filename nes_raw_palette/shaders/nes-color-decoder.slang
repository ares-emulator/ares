#version 450

/*
   NES NTSC Color Decoder shader
   Ported from Bisqwit's C++ NES Palette Generator
   https://forums.nesdev.com/viewtopic.php?p=85060#p85060

   Hue Preserve Clip functions ported from Drag's Palette Generator
   http://drag.wootest.net/misc/palgen.html

   Use with Nestopia or FCEUmm libretro cores with the palette set to 'raw'.
*/

layout(std140, set = 0, binding = 0) uniform UBO
{
   mat4 MVP;
   vec4 OutputSize;
   vec4 OriginalSize;
   vec4 SourceSize;
   uint FrameCount;
} global;

layout(push_constant) uniform Push
{
   float nes_saturation;
   float nes_hue;
   float nes_contrast;
   float nes_brightness;
   float nes_gamma;
   float nes_sony_matrix;
   float nes_clip_method;
} params;

#pragma parameter nes_saturation "Saturation" 1.0 0.0 5.0 0.05
#pragma parameter nes_hue "Hue" 0.0 -360.0 360.0 1.0
#pragma parameter nes_contrast "Contrast" 1.0 0.0 2.0 0.05
#pragma parameter nes_brightness "Brightness" 1.0 0.0 2.0 0.05
#pragma parameter nes_gamma "Gamma" 1.8 1.0 2.5 0.05
#pragma parameter nes_sony_matrix "Sony CXA2025AS US colors" 0.0 0.0 1.0 1.0
#pragma parameter nes_clip_method "Palette clipping method" 0.0 0.0 2.0 1.0

#define saturation params.nes_saturation
#define hue params.nes_hue
#define contrast params.nes_contrast
#define brightness params.nes_brightness
#define gamma params.nes_gamma
#define nes_sony_matrix params.nes_sony_matrix
#define nes_clip_method params.nes_clip_method

//comment the define out to use the "common" conversion matrix instead of the FCC sanctioned one
#define USE_FCC_MATRIX

bool wave (int p, int color)
{
   return ((color + p + 8) % 12 < 6);
}

float gammafix (float f)
{
   return f < 0.0 ? 0.0 : pow(f, 2.2 / gamma);
}

vec3 huePreserveClipDarken(float r, float g, float b)
{
   float ratio = 1.0;
   if ((r > 1.0) || (g > 1.0) || (b > 1.0))
   {
      float max = r;
      if (g > max)
         max = g;
      if (b > max)
         max = b;
      ratio = 1.0 / max;
   }

   r *= ratio;
   g *= ratio;
   b *= ratio;

   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   return vec3(r, g, b);
}

vec3 huePreserveClipDesaturate(float r, float g, float b)
{
   float l = (.299 * r) + (0.587 * g) + (0.114 * b);
   bool ovr = false;
   float ratio = 1.0;

   if ((r > 1.0) || (g > 1.0) || (b > 1.0))
   {
      ovr = true;
      float max = r;
      if (g > max) max = g;
      if (b > max) max = b;
      ratio = 1.0 / max;
   }

   if (ovr)
   {
      r -= 1.0;
      g -= 1.0;
      b -= 1.0;
      r *= ratio;
      g *= ratio;
      b *= ratio;
      r += 1.0;
      g += 1.0;
      b += 1.0;
   }

   r = clamp(r, 0.0, 1.0);
   g = clamp(g, 0.0, 1.0);
   b = clamp(b, 0.0, 1.0);

   return vec3(r, g, b);
}

vec3 MakeRGBColor(int emphasis, int level, int color)
{
   float y = 0.0;
   float i = 0.0;
   float q = 0.0;

   float r = 0.0;
   float g = 0.0;
   float b = 0.0;

   float yiq2rgb[6];

   // Color 0xE and 0xF are black
   level = (color < 14) ? level : 1;

   // Voltage levels, relative to synch voltage
   float black = 0.518;
   float white = 1.962;
   float attenuation = 0.746;
   const float levels[8] = float[] (   0.350 , 0.518, 0.962, 1.550,
                                       1.094, 1.506, 1.962, 1.962);
   
   float low  = levels[level + 4 * int(color == 0)];
   float high = levels[level + 4 * int(color < 13)];
   
   // Calculate the luma and chroma by emulating the relevant circuits:
   for(int p = 0; p < 12; p++) // 12 clock cycles per pixel.
   {
      // NES NTSC modulator (square wave between two voltage levels):
      float spot = wave(p, color) ? high : low;

      // De-emphasis bits attenuate a part of the signal:
      if ((bool(emphasis & 1) && wave(p, 12)) ||
          (bool(emphasis & 2) && wave(p, 4)) ||
          (bool(emphasis & 4) && wave(p, 8))) 
      {
          spot *= attenuation;
      }

      // Normalize:
      float v = (spot - black) / (white - black);

      // Ideal TV NTSC demodulator:
      // Apply contrast/brightness
      v = (v - 0.5) * contrast + 0.5;
      v *= (brightness / 12.0);

      float hue_tweak = hue * 12.0 / 360.0;

      y += v;
      i += v * cos((3.141592653 / 6.0) * (p + hue_tweak) );
      q += v * sin((3.141592653 / 6.0) * (p + hue_tweak) );

   }

   i *= saturation;
   q *= saturation;

   if (nes_sony_matrix > 0.5)
   {
      // Sony CXA2025AS US conversion matrix
      yiq2rgb[0] = 1.630;
      yiq2rgb[1] = 0.317;
      yiq2rgb[2] = -0.378;
      yiq2rgb[3] = -0.466;
      yiq2rgb[4] = -1.089;
      yiq2rgb[5] = 1.677;
   }
   else
   {
#ifdef USE_FCC_MATRIX
      // FCC sanctioned conversion matrix
      yiq2rgb[0] = 0.946882;
      yiq2rgb[1] = 0.623557;
      yiq2rgb[2] = -0.274788;
      yiq2rgb[3] = -0.635691;
      yiq2rgb[4] = -1.108545;
      yiq2rgb[5] = 1.709007;
#else
      // commonly used conversion matrix
      yiq2rgb[0] = 0.956;
      yiq2rgb[1] = 0.621;
      yiq2rgb[2] = -0.272;
      yiq2rgb[3] = -0.647;
      yiq2rgb[4] = -1.105;
      yiq2rgb[5] = 1.702;
#endif
   }

   // Convert YIQ into RGB according to selected conversion matrix
   r = gammafix(y + yiq2rgb[0] * i + yiq2rgb[1] * q);
   g = gammafix(y + yiq2rgb[2] * i + yiq2rgb[3] * q);
   b = gammafix(y + yiq2rgb[4] * i + yiq2rgb[5] * q);

   vec3 corrected_rgb;

   // Apply desired clipping method to out-of-gamut colors.
   if (nes_clip_method < 0.5)
   {
      //If a channel is out of range (> 1.0), it's simply clamped to 1.0. This may change hue, saturation, and/or lightness.
      r = clamp(r, 0.0, 1.0);
      g = clamp(g, 0.0, 1.0);
      b = clamp(b, 0.0, 1.0);
      corrected_rgb = vec3(r, g, b);
   }
   else if (nes_clip_method == 1.0)
   {
      //If any channels are out of range, the color is darkened until it is completely in range.
      corrected_rgb = huePreserveClipDarken(r, g, b);
   }
   else if (nes_clip_method == 2.0)
   {
      //If any channels are out of range, the color is desaturated towards the luminance it would've had.
      corrected_rgb = huePreserveClipDesaturate(r, g, b);
   }

   return corrected_rgb;
}

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord   = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;

void main()
{
   vec4 c = texture(Source, vTexCoord.xy);

   // Extract the chroma, level, and emphasis from the normalized RGB triplet
   int color =    int(floor((c.r * 15.0) + 0.5));
   int level =    int(floor((c.g *  3.0) + 0.5));
   int emphasis = int(floor((c.b *  7.0) + 0.5));

   vec3 out_color = MakeRGBColor(emphasis, level, color);
   FragColor = vec4(out_color, 1.0);
}
