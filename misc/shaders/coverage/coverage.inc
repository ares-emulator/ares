// Computes intersection area between pixels and geometric shapes for perfect
// (analytical) anti-aliasing.
// Assumes that the pixel rectangle has area 1 to avoid normalization.

float rect_coverage(vec4 px_rect, vec4 rect) {
    const vec2 bl = max(rect.xy, px_rect.xy);
    const vec2 tr = min(rect.zw, px_rect.zw);
    const vec2 coverage = max(tr - bl, 0.0);
    return coverage.x * coverage.y;
}
