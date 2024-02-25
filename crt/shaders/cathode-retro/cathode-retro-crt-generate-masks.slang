#version 450

/*
   A port of DeadlyRedCube's Cathode-Retro shader to slang format
   based on a snapshot from https://github.com/DeadlyRedCube/Cathode-Retro circa Nov. 2023
   ported by hunterk
   license: MIT
*/

#include "slang_params.inc"

  // The size of the render target we are rendering to.
  //  NOTE: It is expected that width == 2 * height.
  vec2 g_texSize = vec2(params.FinalViewportSize.y * 2.0, params.FinalViewportSize.y);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This shader generates an approximation of a CRT slot mask (the little R, G, and B dots that you can see on some CRTs
//  when you are close to them.
// It is intended to be rendered to a texture that has a 2:1 width:height ratio, and we are generating two sets of RGB
//  rounded rectangles, where the right-most set is offset 50% vertically (so it wraps from bottom to top).

vec4 aperture(vec2 inTexCoord)
{
  uint texelIndex = uint(inTexCoord.x * g_texSize.x - 0.5);
  float x = float(texelIndex) / g_texSize.x;

  x = fract(x * 2.) * 3.;

  // Default to "red zone"
  vec3 color = vec3(1., 0., 0.);
  if (x >= 2.)
  {
    // blue zone
    color = vec3(0.,0.,1.);
  }
  else if (x >= 1.)
  {
    // green zone
    color = vec3(0.,1.,0.);
  }

  // Convert our coordinate to be in [0..1/3rd]. Note that it was already multiplied by 3 above so this "frac" call
  //  gets us into the [0..1] value for this third.
  x = fract(x);
  x /= 3.;

  // These are the positions of where the border of the colored rectangles will go in our third, as well as how round
  //  the edges will be.
  float border = 1.0 / 12.0;
  float smoothing = border / 3.0;
  border -= smoothing;

  // Do similar calculations to the slot mask texture generation, except without any y component since there are no
  //  breaks along the vertical axis in an aperture grille. This could likely be simplified, but I already had that
  //  logic and this is fine for something that is intended to run once ever (at startup).
  x = abs(x - 1.0 / 6.0);
  x -= 1.0 / 6.0 - (smoothing + border);
  x /= smoothing;
  x = max(0.0, x);

  float delta = 1.0 / (g_texSize.x * smoothing);
  float mul = saturate(1.0 - smoothstep(1.0 - delta * 0.5, 1.0 + delta * 0.5, x));

  color *= mul;
  return vec4(color, 1.);
}

vec4 slotmask(vec2 inTexCoord)
{
  uvec2 texelIndex = uvec2(inTexCoord * g_texSize - 0.5);
  vec2 t = vec2(texelIndex) / g_texSize.x * vec2(1.0, 2.0);

  // Adjust our texture coordinates based on whether we're in the left half of the output or the right half. In each
  //  we'll end up with a x, y in [0..1] square that will contain the R, G, B rectangles.
  if (t.x > 0.5)
  {
    // We're in the right-most half of the texture, so offset our y coordinate by 0.5 (and wrap it to within 0..1) so
    //  this set draws offset vertically.
    t.y = fract(t.y + 0.5);

    // Scale x to be within 0..1 where 0 is the middle of our texture (and the left edge of this rightmost square) and
    //  1 is on the right edge of both.
    t.x = t.x * 2.0 - 1.0;
  }
  else
  {
    // We are in the left half of our texture, so scale the x coordinate so that it is 1.0 at the middle of the texture
    //  (so in this leftmost square it goes from [0..1]). The y coordinate is unchanged.
    t.x *= 2.0;
  }

  // Figure out which of the three "zones" we're in - R, G, or B - i.e. which horizontal third of the square we are in.
  // We'll start by multiplying by 3 so that x in [0..1) is R, [1..2) is G, and [2..3] is B.
  t.x *= 3.;

  // Default to "red zone"
  vec3 color = vec3(1., 0., 0.);
  if (t.x >= 2.)
  {
    // blue zone
    color = vec3(0.,0.,1.);
  }
  else if (t.x >= 1.)
  {
    // green zone
    color = vec3(0.,1.,0.);
  }

  // Convert our coordinate to be in [0..1/3rd]. Note that it was already multiplied by 3 above so this "frac" call
  //  gets us into the [0..1] value for this third.
  t.x = fract(t.x);
  t.x /= 3;

  // These are the positions of where the border of the colored rectangles will go in our third, as well as how round
  //  the edges will be.
  vec2 border = vec2(1.0/3.0 * (1.0 / 4.0), 1.0/6.0);
  float rounding = border.x / 3.0;
  border -= rounding;

  // Center the rectangle so we can do a distance calculation from center to find the bounds of the rectangle, using
  //  that to make a final [0..1] multiply value to scale our chosen rectangle color down to black out the areas
  //  outside of the colored portion.
  t -= vec2(1.0 / 6.0, 0.5);
  t = abs(t);
  t -= (vec2(1.0 / 6.0, 0.5)) - (rounding + border);
  t /= rounding;
  t = max(vec2(0.0, 0.0), t);

  float distance = length(t);
  float delta = 1.0 / (g_texSize.x * rounding);
  float mul = saturate(1.0 - smoothstep(1.0 - delta * 0.5, 1.0 + delta * 0.5, distance));

  color *= mul;
  return vec4(color, 1.0);
}

vec4 shadowmask(vec2 inTexCoord)
{
  // changing this from 6.,4. to make circles instead of ovals -HK
  vec2 hexGridF = inTexCoord * vec2(6.0, 6.0);

  bool isOddBlock = (fract(hexGridF.y * 0.5) >= 0.5);
  if (isOddBlock)
  {
    hexGridF.x += .5;
  }

  ivec2 hexID = ivec2(floor(hexGridF));

  vec2 cellCoord = fract(hexGridF);

  const float t = 0.5773502691; // tan(30 degrees)
  const float c = 0.5 * t;

  if (cellCoord.y < (-t * cellCoord.x) + c)
  {
    hexID.y--;
    hexGridF.x += isOddBlock ? -0.5 : 0.5;
    hexID.x -= int(isOddBlock);
  }
  else if (cellCoord.y < (t * cellCoord.x) - c)
  {
    hexID.y--;
    hexGridF.x += isOddBlock ? -0.5 : 0.5;
    hexID.x += int(!isOddBlock);
  }

  if (fract(float(hexID.y) *0.5) >= 0.5)
  {
    hexGridF.x ++;
    hexID.x ++;
  }

  vec3 color;

  uint hx = uint(hexID.x + 1);
  if ((hx % 3U) == 0U)
  {
    color = vec3(1., 0., 0.);
  }
  else if ((hx % 3U) == 1U)
  {
    color = vec3(0., 1., 0.);
  }
  else
  {
    color = vec3(0., 0., 1.);
  }

  cellCoord = ((hexGridF - vec2(hexID)) * vec2(1.0, 1.0 / (1.0 + c)));
  cellCoord = cellCoord * 2. - 1.;

  float dist = length(cellCoord);
  dist = 1.0 - smoothstep(0.75, 0.8, dist);

  // color *= lerp(0.1, 1.0, float(hexID.y % 2));
  return vec4(color * dist, 1.);
}

void main()
{
   if(int(global.cat_mask_picker) == 1) FragColor = aperture(fract(vTexCoord * (params.FinalViewportSize.xy / 6.) / global.mask_scale));
   else if(int(global.cat_mask_picker) == 2) FragColor = slotmask(fract(vTexCoord * (params.FinalViewportSize.xy / 6.) / global.mask_scale));
   else if(int(global.cat_mask_picker) == 3) FragColor = shadowmask(fract(vTexCoord * (params.FinalViewportSize.xy / 6.) / global.mask_scale));
   // we compensate for brightness later, so better darken up the mask-less image -HK
   else FragColor = vec4(0.5);
}
