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

#include "frag_funcs.inc"

void main()
{
   vec4 lens = vec4(0.);

   // Lenz
   if (bLenzEnable)
   {
      const vec3 lfoffset[19] = {
         vec3(0.9, 0.01, 4),
         vec3(0.7, 0.25, 25),
         vec3(0.3, 0.25, 15),
         vec3(1, 1.0, 5),
         vec3(-0.15, 20, 1),
         vec3(-0.3, 20, 1),
         vec3(6, 6, 6),
         vec3(7, 7, 7),
         vec3(8, 8, 8),
         vec3(9, 9, 9),
         vec3(0.24, 1, 10),
         vec3(0.32, 1, 10),
         vec3(0.4, 1, 10),
         vec3(0.5, -0.5, 2),
         vec3(2, 2, -5),
         vec3(-5, 0.2, 0.2),
         vec3(20, 0.5, 0),
         vec3(0.4, 1, 10),
         vec3(0.00001, 10, 20)
      };
      const vec3 lffactors[19] = {
         vec3(1.5, 1.5, 0),
         vec3(0, 1.5, 0),
         vec3(0, 0, 1.5),
         vec3(0.2, 0.25, 0),
         vec3(0.15, 0, 0),
         vec3(0, 0, 0.15),
         vec3(1.4, 0, 0),
         vec3(1, 1, 0),
         vec3(0, 1, 0),
         vec3(0, 0, 1.4),
         vec3(1, 0.3, 0),
         vec3(1, 1, 0),
         vec3(0, 2, 4),
         vec3(0.2, 0.1, 0),
         vec3(0, 0, 1),
         vec3(1, 1, 0),
         vec3(1, 1, 0),
         vec3(0, 0, 0.2),
         vec3(0.012,0.313,0.588)
      };

      vec2 lfcoord = vec2(0.);
      vec3 lenstemp = vec3(0.);
      vec2 distfact = vTexCoord.xy - 0.5;
      distfact.x *= params.SourceSize.x / params.SourceSize.y;

      for (int i = 0; i < 19; i++)
      {
         lfcoord.xy = lfoffset[i].x * distfact;
         lfcoord.xy *= pow(2.0 * length(distfact), lfoffset[i].y * 3.5);
         lfcoord.xy *= lfoffset[i].z;
         lfcoord.xy = 0.5 - lfcoord.xy;
         vec2 tempfact = (lfcoord.xy - 0.5) * 2.;
         float templensmult = clamp(1.0 - dot(tempfact, tempfact), 0., 1.);
         vec3 lenstemp1 = vec3(dot(textureLod(BackBuffer, vec2(lfcoord.xy), 1.0).rgb, vec3(0.333)));
/* Doesn't exist in RetroArch, so comment it out
#if LENZ_DEPTH_CHECK
         float templensdepth = texture(ReShade::DepthBuffer, lfcoord.xy).x;
         if (templensdepth < 0.99999)
            lenstemp1 = 0;
#endif
*/
         lenstemp1 = max(vec3(0.), lenstemp1.xyz - vec3(global.fLenzThreshold));
         lenstemp1 *= lffactors[i] * templensmult;

         lenstemp += lenstemp1;
      }

      lens.rgb += lenstemp * global.fLenzIntensity;
   }

   // Chapman Lens
   if (bChapFlareEnable)
   {
      vec2 sample_vector = (vec2(0.5, 0.5) - vTexCoord.xy) * global.fChapFlareDispersal;
      vec2 halo_vector = normalize(sample_vector) * global.fChapFlareSize;

      vec3 chaplens = GetDistortedTex(BackBuffer, vTexCoord.xy + halo_vector, halo_vector, fChapFlareCA * 2.5).rgb;

      for (int j = 0; j < int(global.iChapFlareCount); ++j)
      {
         vec2 foffset = sample_vector * float(j);
         chaplens += GetDistortedTex(BackBuffer, vTexCoord.xy + foffset, foffset, fChapFlareCA).rgb;
      }

      chaplens *= 1.0 / global.iChapFlareCount;
      lens.xyz += chaplens;
   }

   // Godrays
   if (bGodrayEnable)
   {
      const vec2 ScreenLightPos = vec2(0.5, 0.5);
      vec2 texcoord2 = vTexCoord;
      vec2 deltaTexCoord = (texcoord2 - ScreenLightPos);
      deltaTexCoord *= 1.0 / global.iGodraySamples * global.fGodrayDensity;

      float illuminationDecay = 1.0;

      for (int g = 0; g < int(global.iGodraySamples); g++)
      {
         texcoord2 -= deltaTexCoord;;
         vec4 sample2 = textureLod(BackBuffer, vec2(texcoord2), 0.);
//         float sampledepth = textureLod(BackBuffer, vec2(texcoord2), 0.).x; //no depth checking in RA so just comment it out
         sample2.w = clamp(dot(sample2.xyz, vec3(0.3333)) - global.fGodrayThreshold, 0., 1.);
         sample2.r *= 1.00;
         sample2.g *= 0.95;
         sample2.b *= 0.85;
         sample2 *= illuminationDecay * global.fGodrayWeight;
#if GODRAY_DEPTH_CHECK == 1
         if (sampledepth > 0.99999)
            lens.rgb += sample2.xyz * sample2.w;
#else
         lens.rgb += sample2.xyz * sample2.w;
#endif
         illuminationDecay *= global.fGodrayDecay;
      }
   }

   // Anamorphic flare
   if (bAnamFlareEnable)
   {
      vec3 anamFlare = vec3(0.);
      const float gaussweight[5] = { 0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162 };

      for (int z = -4; z < 5; z++)
      {
         anamFlare += GetAnamorphicSample(0, vTexCoord.xy + vec2(0, z * PixelSize.y * 2.), global.fFlareBlur) * fFlareTint * gaussweight[abs(z)];
      }

      lens.xyz += anamFlare * global.fFlareIntensity;
   }
   FragColor = lens;
}