#version 450

layout(push_constant) uniform Push
{
	vec4 SourceSize;
	vec4 OriginalSize;
	vec4 OutputSize;
	uint FrameCount;
} params;

#include "params.inc"

#pragma stage vertex
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 TexCoord;
layout(location = 0) out vec2 vTexCoord;

void main()
{
   gl_Position = global.MVP * Position;
   vTexCoord = TexCoord;
}

#pragma stage fragment
layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 2) uniform sampler2D Source;
layout(set = 0, binding = 3) uniform sampler2D Original;
layout(set = 0, binding = 4) uniform sampler2D SamplerBloom3;
layout(set = 0, binding = 5) uniform sampler2D SamplerBloom5;
layout(set = 0, binding = 6) uniform sampler2D Dirt;
layout(set = 0, binding = 7) uniform sampler2D Sprite;
layout(set = 0, binding = 8) uniform sampler2D LensFlare1;

#include "frag_funcs.inc"

#define saturate(c) clamp(c, 0., 1.)

void main()
{
   vec4 color = vec4(0.);
   color = texture(BackBuffer, vTexCoord.xy);

   // Bloom
   vec3 colorbloom = vec3(0.);
   colorbloom += texture(SamplerBloom3, vTexCoord.xy).rgb * 1.0;
   colorbloom += texture(SamplerBloom5, vTexCoord.xy).rgb * 9.0;
   colorbloom *= 0.1;
   colorbloom = saturate(colorbloom);
   float colorbloomgray = dot(colorbloom, vec3(0.333));
   colorbloom = mix(vec3(colorbloomgray), colorbloom, vec3(global.fBloomSaturation));
   colorbloom *= fBloomTint;

   if (global.iBloomMixmode == 0.)
      color.rgb += colorbloom;
   else if (global.iBloomMixmode == 1.)
      color.rgb = 1. - (1. - color.rgb) * (1. - colorbloom);
   else if (global.iBloomMixmode == 2.)
      color.rgb = max(vec3(0.0), max(color.rgb, mix(color.rgb, (1. - (1. - saturate(colorbloom)) * (1. - saturate(colorbloom))), 1.0)));
   else if (global.iBloomMixmode == 3.)
      color.rgb = max(color.rgb, colorbloom);

   // Anamorphic flare
   if (bAnamFlareEnable)
   {
      vec3 anamflare = texture(SamplerBloom5, vTexCoord.xy).w * 2. * fAnamFlareColor;
      anamflare = max(anamflare, 0.0);
      color.rgb += pow(anamflare, vec3(1.0 / global.fAnamFlareCurve));
   }

   // Lens dirt
   if (bLensdirtEnable)
   {
      float lensdirtmult = dot(texture(SamplerBloom5, vTexCoord).rgb, vec3(0.333));
      vec3 dirttex = texture(Dirt, vTexCoord).rgb;
      vec3 lensdirt = dirttex * lensdirtmult * global.fLensdirtIntensity;

      lensdirt = mix(vec3(dot(lensdirt.xyz, vec3(0.333))), lensdirt.xyz, vec3(global.fLensdirtSaturation));

      if (global.iLensdirtMixmode == 0.)
         color.rgb += lensdirt;
      else if (global.iLensdirtMixmode == 1.)
         color.rgb = 1. - (1. - color.rgb) * (1. - lensdirt);
      else if (global.iLensdirtMixmode == 2.)
         color.rgb = max(vec3(0.0), max(color.rgb, mix(color.rgb, (1. - (1. - saturate(lensdirt)) * (1. - saturate(lensdirt))), 1.0)));
      else if (global.iLensdirtMixmode == 3.)
         color.rgb = max(color.rgb, lensdirt);
   }

   // Lens flares
   if (bAnamFlareEnable || bLenzEnable || bGodrayEnable || bChapFlareEnable)
   {
      vec3 lensflareSample = texture(LensFlare1, vTexCoord.xy).rgb, lensflareMask;
      lensflareMask  = texture(Sprite, vTexCoord + vec2( 0.5,  0.5) * PixelSize).rgb;
      lensflareMask += texture(Sprite, vTexCoord + vec2(-0.5,  0.5) * PixelSize).rgb;
      lensflareMask += texture(Sprite, vTexCoord + vec2( 0.5, -0.5) * PixelSize).rgb;
      lensflareMask += texture(Sprite, vTexCoord + vec2(-0.5, -0.5) * PixelSize).rgb;

      color.rgb += lensflareMask * 0.25 * lensflareSample;
   }
   FragColor = color;
}