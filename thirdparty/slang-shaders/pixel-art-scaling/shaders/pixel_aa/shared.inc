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
INSTANTIATE_SLOPESTEP(vec2)

float to_lin(float x) { return pow(x, 2.2); }
vec3 to_lin(vec3 x) { return pow(x, vec3(2.2)); }

float to_srgb(float x) { return pow(x, 1.0 / 2.2); }
vec3 to_srgb(vec3 x) { return pow(x, vec3(1.0 / 2.2)); }

// Function to get a pixel value, taking into consideration possible subpixel
// interpolation.
vec4 pixel_aa(sampler2D tex, vec2 tx_per_px, vec2 tx_to_uv, vec2 tx_coord,
              float sharpness, bool gamma_correct, bool sample_subpx,
              uint subpx_orientation, uint screen_rotation) {
    const float sharpness_upper = min(1.0, sharpness);
    const vec2 sharp_lb = sharpness_upper * (0.5 - 0.5 * tx_per_px);
    const vec2 sharp_ub =
        1.0 - sharpness_upper * (1.0 - (0.5 + 0.5 * tx_per_px));
    const float sharpness_lower = max(1.0, sharpness);

    if (sample_subpx) {
        // Subpixel sampling: Shift the sampling by 1/3rd of an output pixel for
        // each subpixel, assuming that the output size is at monitor
        // resolution.
        // Compensate for possible rotation of the screen in certain cores.
        const vec4 rot_corr = vec4(1.0, 0.0, -1.0, 0.0);
        const vec2 sub_tx_offset =
            tx_per_px / 3.0 *
            vec2(rot_corr[(screen_rotation + subpx_orientation) % 4],
                 rot_corr[(screen_rotation + subpx_orientation + 3) % 4]);

        vec3 res;
        vec2 period, phase, offset;

        if (gamma_correct) {
            // Red
            period = floor(tx_coord - sub_tx_offset - 0.5);
            phase = tx_coord - sub_tx_offset - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.r = to_srgb(mix(
                mix(to_lin(texture(tex, (period + 0.5) * tx_to_uv).r),
                    to_lin(
                        texture(tex, (period + vec2(1.5, 0.5)) * tx_to_uv).r),
                    offset.x),
                mix(to_lin(
                        texture(tex, (period + vec2(0.5, 1.5)) * tx_to_uv).r),
                    to_lin(texture(tex, (period + 1.5) * tx_to_uv).r),
                    offset.x),
                offset.y));
            // Green
            period = floor(tx_coord - 0.5);
            phase = tx_coord - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.g = to_srgb(mix(
                mix(to_lin(texture(tex, (period + 0.5) * tx_to_uv).g),
                    to_lin(
                        texture(tex, (period + vec2(1.5, 0.5)) * tx_to_uv).g),
                    offset.x),
                mix(to_lin(
                        texture(tex, (period + vec2(0.5, 1.5)) * tx_to_uv).g),
                    to_lin(texture(tex, (period + 1.5) * tx_to_uv).g),
                    offset.x),
                offset.y));
            // Blue
            period = floor(tx_coord + sub_tx_offset - 0.5);
            phase = tx_coord + sub_tx_offset - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.b = to_srgb(mix(
                mix(to_lin(texture(tex, (period + 0.5) * tx_to_uv).b),
                    to_lin(
                        texture(tex, (period + vec2(1.5, 0.5)) * tx_to_uv).b),
                    offset.x),
                mix(to_lin(
                        texture(tex, (period + vec2(0.5, 1.5)) * tx_to_uv).b),
                    to_lin(texture(tex, (period + 1.5) * tx_to_uv).b),
                    offset.x),
                offset.y));
        } else {
            // Red
            period = floor(tx_coord - sub_tx_offset - 0.5);
            phase = tx_coord - sub_tx_offset - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.r = texture(tex, (period + 0.5 + offset) * tx_to_uv).r;
            // Green
            period = floor(tx_coord - 0.5);
            phase = tx_coord - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.g = texture(tex, (period + 0.5 + offset) * tx_to_uv).g;
            // Blue
            period = floor(tx_coord + sub_tx_offset - 0.5);
            phase = tx_coord + sub_tx_offset - 0.5 - period;
            offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);
            res.b = texture(tex, (period + 0.5 + offset) * tx_to_uv).b;
        }
        return vec4(res, 1.0);
    } else {
        // The offset for interpolation is a periodic function with
        // a period length of 1 texel.
        // The input coordinate is shifted so that the center of the texel
        // aligns with the start of the period.
        // First, get the period and phase.
        vec2 period = floor(tx_coord - 0.5);
        vec2 phase = tx_coord - 0.5 - period;
        // The function starts at 0, then starts transitioning at
        // 0.5 - 0.5 / pixels_per_texel, then reaches 0.5 at 0.5,
        // Then reaches 1 at 0.5 + 0.5 / pixels_per_texel.
        // For sharpness values < 1.0, blend to bilinear filtering.
        vec2 offset = slopestep(sharp_lb, sharp_ub, phase, sharpness_lower);

        // With gamma correct blending, we have to do 4 taps and interpolate
        // manually. Without it, we can make use of a single tap using bilinear
        // interpolation. The offsets are shifted back to the texel center
        // before sampling.
        if (gamma_correct) {
            return vec4(
                to_srgb(mix(
                    mix(to_lin(texture(tex, (period + 0.5) * tx_to_uv).rgb),
                        to_lin(
                            texture(tex, (period + vec2(1.5, 0.5)) * tx_to_uv)
                                .rgb),
                        offset.x),
                    mix(to_lin(
                            texture(tex, (period + vec2(0.5, 1.5)) * tx_to_uv)
                                .rgb),
                        to_lin(texture(tex, (period + 1.5) * tx_to_uv).rgb),
                        offset.x),
                    offset.y)),
                1.0);
        } else {
            return texture(tex, (period + 0.5 + offset) * tx_to_uv);
        }
    }
}
