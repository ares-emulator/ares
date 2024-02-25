// See pixel_aa.slang for copyright and other information.

// Similar to smoothstep, but has a configurable slope at x = 0.5.
// Original smoothstep has a slope of 1.5 at x = 0.5
#define INSTANTIATE_SLOPESTEP(T)                               \
    T slopestep(T edge0, T edge1, T x, float slope) {          \
        x = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);    \
        const T s = sign(x - 0.5);                             \
        const T o = (1.0 + s) * 0.5;                           \
        return o - 0.5 * s * pow(2.0 * (o - s * x), T(slope)); \
    }
INSTANTIATE_SLOPESTEP(float)
INSTANTIATE_SLOPESTEP(vec2)

vec3 to_lin(vec3 x) { return pow(x, vec3(2.2)); }

vec3 to_srgb(vec3 x) { return pow(x, vec3(1.0 / 2.2)); }

// Function to get a single sample using the "pixel AA" method.
// Params:
// tx_coord: Coordinate in source pixel (texel) coordinates
vec3 sample_aa(sampler2D tex, vec2 tx_per_px, vec2 tx_to_uv, vec2 tx_coord,
               float sharpness, bool gamma_correct) {
    // The offset for interpolation is a periodic function with
    // a period length of 1 texel.
    // The input coordinate is shifted so that the center of the texel
    // aligns with the start of the period.
    // First, get the period and phase.
    vec2 period;
    const vec2 phase = modf(tx_coord - 0.5, period);
    // The function starts at 0, then starts transitioning at
    // 0.5 - 0.5 / pixels_per_texel, then reaches 0.5 at 0.5,
    // Then reaches 1 at 0.5 + 0.5 / pixels_per_texel.
    // For sharpness values < 1.0, blend to bilinear filtering.
    const vec2 offset =
        slopestep(min(1.0, sharpness) * (0.5 - 0.5 * tx_per_px),
                  1.0 - min(1.0, sharpness) * (1.0 - (0.5 + 0.5 * tx_per_px)),
                  phase, max(1.0, sharpness));

    // With gamma correct blending, we have to do 4 taps and interpolate
    // manually. Without it, we can make use of a single tap using bilinear
    // interpolation. The offsets are shifted back to the texel center before
    // sampling.
    if (gamma_correct) {
        const vec3 samples[] = {
            to_lin(texture(tex, (period + 0.5) * tx_to_uv).rgb),
            to_lin(texture(tex, (period + vec2(1.5, 0.5)) * tx_to_uv).rgb),
            to_lin(texture(tex, (period + vec2(0.5, 1.5)) * tx_to_uv).rgb),
            to_lin(texture(tex, (period + 1.5) * tx_to_uv).rgb)};
        return to_srgb(mix(mix(samples[0], samples[1], offset.x),
                           mix(samples[2], samples[3], offset.x), offset.y));
    } else {
        return texture(tex, (period + 0.5 + offset) * tx_to_uv).rgb;
    }
}

// Function to get a pixel value, taking into consideration possible subpixel
// interpolation.
vec4 pixel_aa(sampler2D tex, vec2 tx_per_px, vec2 tx_to_uv, vec2 tx_coord,
              float sharpness, bool gamma_correct, bool sample_subpx,
              uint subpx_orientation, uint screen_rotation) {
    if (sample_subpx) {
        // Subpixel sampling: Shift the sampling by 1/3rd of an output pixel for
        // each subpixel, assuming that the output size is at monitor
        // resolution.
        // Account for different subpixel orientations and also for a possible
        // rotation of the screen in certain cores.
        const vec2 rotation_correction[] = {vec2(1.0, 0.0), vec2(0.0, 1.0),
                                            vec2(-1.0, 0.0), vec2(0.0, -1.0)};
        const vec2 sub_tx_offset =
            tx_per_px / 3.0 *
            rotation_correction[(screen_rotation + subpx_orientation) % 4];
        vec3 res;
        for (int i = -1; i < 2; ++i) {
            res[i + 1] = sample_aa(tex, tx_per_px, tx_to_uv,
                                   tx_coord + sub_tx_offset * float(i),
                                   sharpness, gamma_correct)[i + 1];
        }
        return vec4(res, 1.0);
    } else {
        return vec4(sample_aa(tex, tx_per_px, tx_to_uv, tx_coord, sharpness,
                              gamma_correct),
                    1.0);
    }
}
