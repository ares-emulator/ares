#version 450
/* Copyright (c) 2020 Themaister
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#extension GL_EXT_samplerless_texture_functions : require

#include "small_types.h"
#include "vi_status.h"
#include "vi_debug.h"
#include "noise.h"

layout(set = 0, binding = 0) uniform mediump utexture2DArray uDivotOutput;
layout(set = 0, binding = 1) uniform itextureBuffer uHorizontalInfo;
layout(set = 1, binding = 0) uniform mediump utextureBuffer uGammaTable;
layout(location = 0) out vec4 FragColor;

layout(push_constant, std430) uniform Registers
{
    ivec2 frag_coord_offset;
    int v_start;
    int y_add;
    int frame_count;

    int serrate_shift;
    int serrate_mask;
    int serrate_select;

    int info_y_shift;
} registers;

uvec3 vi_lerp(uvec3 a, uvec3 b, uint l)
{
    return (a + (((b - a) * l + 16u) >> 5u)) & 0xffu;
}

uvec3 integer_gamma(uvec3 color)
{
    uvec3 res;
    if (GAMMA_DITHER)
    {
        color = (color << 6) + noise_get_full_gamma_dither() + 256u;
        res = uvec3(
            texelFetch(uGammaTable, int(color.r)).r,
            texelFetch(uGammaTable, int(color.g)).r,
            texelFetch(uGammaTable, int(color.b)).r);
    }
    else
    {
        res = uvec3(
            texelFetch(uGammaTable, int(color.r)).r,
            texelFetch(uGammaTable, int(color.g)).r,
            texelFetch(uGammaTable, int(color.b)).r);
    }
    return res;
}

layout(constant_id = 2) const bool FETCH_BUG = false;

void main()
{
    // Handles crop where we start scanning out at an offset.
    ivec2 coord = ivec2(gl_FragCoord.xy) + registers.frag_coord_offset;

    int info_index = coord.y >> registers.info_y_shift;
    ivec4 horiz_info0 = texelFetch(uHorizontalInfo, 2 * info_index + 0);
    ivec4 horiz_info1 = texelFetch(uHorizontalInfo, 2 * info_index + 1);

    int h_start = horiz_info0.x;
    int h_start_clamp = horiz_info0.y;
    int h_end_clamp = horiz_info0.z;
    int x_start = horiz_info0.w;
    int x_add = horiz_info1.x;
    int y_start = horiz_info1.y;
    int y_add = horiz_info1.z;
    int y_base = horiz_info1.w;

    // Rebase Y relative to YStart.
    coord.y -= registers.v_start;

    // Scissor against HStart/End, also handles serrate where we skip every other line.
    if (coord.x < h_start_clamp || coord.x >= h_end_clamp ||
            ((coord.y & registers.serrate_mask) != registers.serrate_select))
        discard;

    // Shift the X coord to be relative to sampling, this can change per scanline.
    coord.x -= h_start;

    // Rebase Y in terms of progressive scan.
    coord.y >>= registers.serrate_shift;

    if (GAMMA_DITHER)
        reseed_noise(coord.x, coord.y, registers.frame_count);

    int x = coord.x * x_add + x_start;
    int y = (coord.y - y_base) * y_add + y_start;
    ivec2 base_coord = ivec2(x, y) >> 10;
    uvec3 c00 = texelFetch(uDivotOutput, ivec3(base_coord, 0), 0).rgb;

    int bug_offset = 0;
    if (FETCH_BUG)
    {
        // This is super awkward.
        // Basically there seems to be some kind of issue where if we interpolate in Y,
        // we're going to get buggy output.
        // If we hit this case, the next line we filter against will come from the "buggy" array slice.
        // Why this makes sense, I have no idea.
        //
        // XXX: This assumes constant YAdd.
        // No idea how this is supposed to work if YAdd can vary per scanline.
        int prev_y = (y - registers.y_add) >> 10;
        int next_y = (y + registers.y_add) >> 10;
        if (coord.y != 0 && base_coord.y == prev_y && base_coord.y != next_y)
            bug_offset = 1;
    }

    if (SCALE_AA)
    {
        int x_frac = (x >> 5) & 31;
        int y_frac = (y >> 5) & 31;

        uvec3 c10 = texelFetchOffset(uDivotOutput, ivec3(base_coord, 0), 0, ivec2(1, 0)).rgb;
        uvec3 c01 = texelFetchOffset(uDivotOutput, ivec3(base_coord, bug_offset), 0, ivec2(0, 1)).rgb;
        uvec3 c11 = texelFetchOffset(uDivotOutput, ivec3(base_coord, bug_offset), 0, ivec2(1)).rgb;

        c00 = vi_lerp(c00, c01, y_frac);
        c10 = vi_lerp(c10, c11, y_frac);
        c00 = vi_lerp(c00, c10, x_frac);
    }

    if (GAMMA_ENABLE)
        c00 = integer_gamma(c00);
    else if (GAMMA_DITHER)
        c00 = min(c00 + noise_get_partial_gamma_dither(), uvec3(0xff));

    FragColor = vec4(vec3(c00) / 255.0, 1.0);
}