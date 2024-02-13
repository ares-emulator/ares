#define PixelSize (params.SourceSize.zw)
#define BackBuffer Original

vec4 GaussBlur22(vec2 coord, sampler2D tex, float mult, float lodlevel, bool isBlurVert)
{
   vec4 sum = vec4(0.);
   vec2 axis = isBlurVert ? vec2(0., 1.) : vec2(1., 0.);

   const float weight[11] = {
      0.082607,
      0.080977,
      0.076276,
      0.069041,
      0.060049,
      0.050187,
      0.040306,
      0.031105,
      0.023066,
      0.016436,
      0.011254
   };

   for (int i = -10; i < 11; i++)
   {
      float currweight = weight[abs(i)];
      sum += textureLod(tex, vec2(coord.xy + axis.xy * float(i) * PixelSize * mult), lodlevel) * currweight;
   }

   return sum;
}

vec3 GetDnB(sampler2D tex, vec2 coords)
{
   vec3 color = vec3(max(0, dot(textureLod(tex, vec2(coords.xy), 4.).rgb, vec3(0.333)) - global.fChapFlareThreshold) * global.fChapFlareIntensity);
/* This isn't something RA has, so just comment it out
#if CHAPMAN_DEPTH_CHECK
   if (textureLod(ReShade::DepthBuffer, vec4(coords.xy, 0, 3)).x < 0.99999)
      color = 0;
#endif
*/
   return color;
}
vec3 GetDistortedTex(sampler2D tex, vec2 sample_center, vec2 sample_vector, vec3 distortion)
{
   vec2 final_vector = sample_center + sample_vector * min(min(distortion.r, distortion.g), distortion.b);

   if (final_vector.x > 1.0 || final_vector.y > 1.0 || final_vector.x < -1.0 || final_vector.y < -1.0)
      return vec3(0, 0, 0);
   else
      return vec3(
         GetDnB(tex, sample_center + sample_vector * distortion.r).r,
         GetDnB(tex, sample_center + sample_vector * distortion.g).g,
         GetDnB(tex, sample_center + sample_vector * distortion.b).b);
}

vec3 GetBrightPass(vec2 coords)
{
   vec3 c = texture(BackBuffer, coords).rgb;
   vec3 bC = max(c - global.fFlareLuminance.xxx, 0.0);
   float bright = dot(bC, vec3(1.0));
   bright = smoothstep(0.0f, 0.5, bright);
   vec3 result = mix(vec3(0.0), c, vec3(bright));
/* This isn't something RA has, so just comment it out
#if FLARE_DEPTH_CHECK
   float checkdepth = tex2D(ReShade::DepthBuffer, coords).x;
   if (checkdepth < 0.99999)
      result = 0;
#endif
*/
   return result;
}
vec3 GetAnamorphicSample(int axis, vec2 coords, float blur)
{
   coords = 2.0 * coords - 1.0;
   coords.x /= -blur;
   coords = 0.5 * coords + 0.5;
   return GetBrightPass(coords);
}