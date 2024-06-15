// See the main shader file for copyright and other information.

// Intersects 2D lines, defined as normal vector (.x and .y) and offset (.z).
vec2 line_intersection(vec3 l1, vec3 l2) {
    // Simplification: Assume lines are not parallel.
    const float inv_det = 1.0 / (l1.x * l2.y - l2.x * l1.y);
    return vec2((l2.y * l1.z - l1.y * l2.z) * inv_det,
                (l1.x * l2.z - l2.x * l1.z) * inv_det);
}

void generate_ray(vec2 tex_coord, vec2 input_aspect, vec2 output_aspect,
                  vec3 plane_o, vec3 plane_u, vec3 plane_v, float curv, float f,
                  float shape, float zoom, inout vec3 prim_ray_o,
                  inout vec3 prim_ray_d) {
    // Figure out optimal camera position from 9 points sampled across the
    // frame. We want to find the camera position that is as close as possible
    // to the points, maximizing the points in the frustum view.
    vec3 half_spaces[4] = {vec3(f, 0.5 * output_aspect.x, 1.0e7),
                           vec3(-f, 0.5 * output_aspect.x, 1.0e7),
                           vec3(f, 0.5 * output_aspect.y, 1.0e7),
                           vec3(-f, 0.5 * output_aspect.y, 1.0e7)};
    vec3 p_min = vec3(1.0e7);
    vec3 p_max = vec3(-1.0e7);
    for (int i = -1; i < 2; ++i) {
        for (int j = -1; j < 2; ++j) {
            const vec2 uv = vec2(i * 0.5, j * 0.5) * input_aspect;
            vec3 p = plane_o + uv.x * plane_u + uv.y * plane_v;
            if (curv > 1.0e-3) {
                // Simplification: Assume shape = 0 for sphere, = 1
                // for cylinder. This allows multiplication instead of
                // branching.
                // Simplification: Assume cylinder axis == plane_v.
                const vec3 p_on_ax = shape * dot(p, plane_v) * plane_v;
                p = p_on_ax + normalize(p - p_on_ax) / curv;
            }
            half_spaces[0].z =
                min(half_spaces[0].z, dot(half_spaces[0].xy, p.xz));
            half_spaces[1].z =
                min(half_spaces[1].z, dot(half_spaces[1].xy, p.xz));
            half_spaces[2].z =
                min(half_spaces[2].z, dot(half_spaces[2].xy, p.yz));
            half_spaces[3].z =
                min(half_spaces[3].z, dot(half_spaces[3].xy, p.yz));
            p_min = min(p_min, p);
            p_max = max(p_max, p);
        }
    }

    // Generate camera ray.
    if (f < RT_CURV_F_MAX) {
        // Perspective camera.
        const vec2 i_xz = line_intersection(half_spaces[0], half_spaces[1]);
        const vec2 i_yz = line_intersection(half_spaces[2], half_spaces[3]);
        const float ideal_cam_z = min(i_xz[1], i_yz[1]);
        prim_ray_o =
            vec3(i_xz[0], i_yz[0], p_min.z + (ideal_cam_z - p_min.z) / zoom);
        prim_ray_d = vec3((tex_coord - 0.5) * output_aspect, f);
    } else {
        // Orthographic camera.
        const vec3 p_extent = p_max - p_min;
        const vec2 p_center = 0.5 * (p_min.xy + p_max.xy);
        prim_ray_o = vec3(p_center + (tex_coord - 0.5) * output_aspect *
                                         max(p_extent.x / output_aspect.x,
                                             p_extent.y / output_aspect.y) /
                                         zoom,
                          p_min.z - 1.0);
        prim_ray_d = vec3(0.0, 0.0, 1.0);
    }
}

vec2 trace_ray(vec2 input_aspect, vec3 prim_ray_o, vec3 prim_ray_d,
               vec3 plane_n, vec3 plane_u, vec3 plane_v, float curv,
               float shape) {
    vec3 sec_ray_o = prim_ray_o;
    vec3 sec_ray_d = prim_ray_d;

    if (curv > 1.0e-3) {
        // Intersect sphere / cylinder.
        // Simplification: Assume shape = 0 for sphere, = 1 for
        // cylinder. This allows multiplication instead of branching.
        // Simplification: Assume cylinder axis == plane_v.
        const vec3 alpha =
            prim_ray_d - shape * dot(prim_ray_d, plane_v) * plane_v;
        const vec3 beta =
            prim_ray_o - shape * dot(prim_ray_o, plane_v) * plane_v;
        const float half_b = dot(alpha, beta);
        const float c = dot(beta, beta) - 1.0 / (curv * curv);
        // Simplification: a = dot(alpha, alpha).
        const float discriminant = half_b * half_b - dot(alpha, alpha) * c;
        if (discriminant < 0.0) {
            // Ray misses screen surface entirely.
            return vec2(-1.0);
        }

        // We only need the smaller root of the two solutions for the ray-object
        // intersection. The smaller root can be found as c / q, according to:
        // https://www.av8n.com/physics/quadratic-formula.htm
        // Simplification: Assume the solution is positive.
        // Simplification: Assume half_b < 0.
        // Simplification: p_screen = sec_ray_o.
        sec_ray_o = prim_ray_o + c / (sqrt(discriminant) - half_b) * prim_ray_d;
        // Simplification: Assume shape = 0 for sphere, = 1 for
        // cylinder. This allows multiplication instead of branching.
        sec_ray_d = sec_ray_o - shape * dot(sec_ray_o, plane_v) * plane_v;
    }

    // Intersect plane.
    // Simplification:
    // t = dot(plane_o - sec_ray_o, plane_n) / dot(plane_n, sec_ray_d).
    // Simplification: Assume t > 0.
    // Simplification: Assume denominator is not close to zero.
    // Simplification: p_plane = sec_ray_o + dot(plane_o - sec_ray_o, plane_n) /
    //  dot(plane_n, sec_ray_d) * sec_ray_d;
    const vec3 op = sec_ray_o +
                    dot(plane_n - sec_ray_o, plane_n) /
                        dot(plane_n, sec_ray_d) * sec_ray_d -
                    plane_n;
    // Convert plane intersection to input UV.
    return vec2(dot(op, plane_u / input_aspect.x),
                dot(op, plane_v / input_aspect.y)) +
           0.5;
}
