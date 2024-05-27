// See dual_filter.inc for copyright and other information.

// Tonemapping following
// http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/
// Parameters:
// toeStrength       = 0.5
// toeLength         = 0.5
// shoulderStrength  = 2.0
// shoulderLength    = 0.5
// shoulderAngle     = 1.0
float tonemap(float x) {
    x = clamp(x, 0.0, 16.0);
    x *= 0.246645;
    if (x < 0.026840) {
        // Toe
        const float y0 = exp(4.324533 + 2.0 * log(x + 1.e-8));
        return y0 * 0.529110;
    } else if (x < 0.143452) {
        // Linear
        const float x0 = x - 0.013420;
        const float y0 = 4.054409 * x0;
        return y0 * 0.529110;
    } else {
        // Shoulder
        const float x0 = 5.0 - x;
        const float y0 = exp(-20.740919 + 13.369429 * log(x0 + 1.e-8));
        return y0 * -0.529110 + 1.058221;
    }
}
vec3 tonemap(vec3 x) {
    return vec3(tonemap(x.r), tonemap(x.g), tonemap(x.b));
}

float ambient_to_intensity(vec3 ambient, float min_exp_intensity, float max_exp_intensity,
                           float min_exp, float max_exp) {
    // Moving average luminance sampled from respective pass.
    float exposure = dot(vec3(0.30, 0.59, 0.11), ambient);
    // Lift up exposure value for better control.
    exposure = sqrt(exposure);
    // Map from exposure value to bloom intensity, and then to final value
    return mix(min_exp_intensity, max_exp_intensity, smoothstep(min_exp, max_exp, exposure));
}

vec3 blend_screen(vec3 a, vec3 b) {
    return 1.0 - (1.0 - a) * (1.0 - b);
}
