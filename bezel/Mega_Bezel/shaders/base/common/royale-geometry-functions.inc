/////////////////////////////  GPL LICENSE NOTICE  /////////////////////////////

//  royale-geometry-functions, extracted from crt-royale
//  crt-royale: A full-featured CRT shader, with cheese.
//  GPL & Copyright (C) 2014 TroggleMonkey <trogglemonkey@gmx.com>


////////////////////////////  MACROS AND CONSTANTS  ////////////////////////////

//  Curvature-related constants:
#define HRG_MAX_POINT_CLOUD_SIZE 9

/////////////////////////////  CURVATURE FUNCTIONS /////////////////////////////

vec2 hrg_quadratic_solve(float a, float b_over_2, float c)
{
    //  Requires:   1.) a, b, and c are quadratic formula coefficients
    //              2.) b_over_2 = b/2.0 (simplifies terms to factor 2 out)
    //              3.) b_over_2 must be guaranteed < 0 (avoids a branch)
    //  Returns:    Returns vec2(first_solution, discriminant), so the caller
    //              can choose how to handle the "no intersection" case.  The
    //              Kahan or Citardauq formula is used for numerical robustness.
    float discriminant = b_over_2 * b_over_2 - a * c;
    float solution0 = c / (-b_over_2 + sqrt(discriminant));
    return vec2(solution0, discriminant);
}

vec2 hrg_intersect_sphere(vec3 view_vec, vec3 eye_pos_vec, float in_geom_radius)
{
    //  Requires:   1.) view_vec and eye_pos_vec are 3D vectors in the sphere's
    //                  local coordinate frame (eye_pos_vec is a position, i.e.
    //                  a vector from the origin to the eye/camera)
    //              2.) in_geom_radius is a global containing the sphere's radius
    //  Returns:    Cast a ray of direction view_vec from eye_pos_vec at a
    //              sphere of radius in_geom_radius, and return the distance to
    //              the first intersection in units of length(view_vec).
    //              http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection
    //  Quadratic formula coefficients (b_over_2 is guaranteed negative):
    float a = dot(view_vec, view_vec);
    float b_over_2 = dot(view_vec, eye_pos_vec);  //  * 2.0 factored out
    float c = dot(eye_pos_vec, eye_pos_vec) - in_geom_radius * in_geom_radius;
    return hrg_quadratic_solve(a, b_over_2, c);
}

vec2 hrg_intersect_cylinder(vec3 view_vec, vec3 eye_pos_vec, float in_geom_radius)
{
    //  Requires:   1.) view_vec and eye_pos_vec are 3D vectors in the sphere's
    //                  local coordinate frame (eye_pos_vec is a position, i.e.
    //                  a vector from the origin to the eye/camera)
    //              2.) in_geom_radius is a global containing the cylinder's radius
    //  Returns:    Cast a ray of direction view_vec from eye_pos_vec at a
    //              cylinder of radius in_geom_radius, and return the distance to
    //              the first intersection in units of length(view_vec).  The
    //              derivation of the coefficients is in Christer Ericson's
    //              Real-Time Collision Detection, p. 195-196, and this version
    //              uses LaGrange's identity to reduce operations.
    //  Arbitrary "cylinder top" reference point for an infinite cylinder:
    vec3 cylinder_top_vec = vec3(0, in_geom_radius, 0);
    vec3 cylinder_axis_vec = vec3(0, 1, 0);//vec3(0, 2.0*in_geom_radius, 0);
    vec3 top_to_eye_vec = eye_pos_vec - cylinder_top_vec;
    vec3 axis_x_view = cross(cylinder_axis_vec, view_vec);
    vec3 axis_x_top_to_eye = cross(cylinder_axis_vec, top_to_eye_vec);
    //  Quadratic formula coefficients (b_over_2 is guaranteed negative):
    float a = dot(axis_x_view, axis_x_view);
    float b_over_2 = dot(axis_x_top_to_eye, axis_x_view);
    float c = dot(axis_x_top_to_eye, axis_x_top_to_eye) - in_geom_radius * in_geom_radius; //*dot(cylinder_axis_vec, cylinder_axis_vec);
    return hrg_quadratic_solve(a, b_over_2, c);
}

vec2 hrg_cylinder_xyz_to_uv( vec3 intersection_pos_local, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   An xyz intersection position on a cylinder.
    //  Returns:    video_uv coords mapped to range [-0.5, 0.5]
    //  Mapping:    Define square_uv.x to be the signed arc length in xz-space,
    //              and define square_uv.y = -intersection_pos_local.y (+v = -y).
    //  Start with a numerically robust arc length calculation.
    float angle_from_image_center = atan(intersection_pos_local.x, intersection_pos_local.z);
    float signed_arc_len = angle_from_image_center * in_geom_radius;
    //  Get a uv-mapping where [-0.5, 0.5] maps to a "square" area, then divide
    //  by the aspect ratio to stretch the mapping appropriately:
    vec2 square_uv = vec2(signed_arc_len, -intersection_pos_local.y);
    vec2 video_uv = square_uv / output_aspect;
    return video_uv;
}

vec3 hrg_cylinder_uv_to_xyz(vec2 video_uv, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   video_uv coords mapped to range [-0.5, 0.5]
    //  Returns:    An xyz intersection position on a cylinder.  This is the
    //              inverse of hrg_cylinder_xyz_to_uv().
    //  Expand video_uv by the aspect ratio to get proportionate x/y lengths,
    //  then calculate an xyz position for the cylindrical mapping above.
    vec2 square_uv = video_uv * output_aspect;
    float arc_len = square_uv.x;
    float angle_from_image_center = arc_len / in_geom_radius;
    float x_pos = sin(angle_from_image_center) * in_geom_radius;
    float z_pos = cos(angle_from_image_center) * in_geom_radius;
    //  Or: z = sqrt(in_geom_radius**2 - x**2)
    //  Or: z = in_geom_radius/sqrt(1 + tan(angle)**2), x = z * tan(angle)
    vec3 intersection_pos_local = vec3(x_pos, -square_uv.y, z_pos);
    return intersection_pos_local;
}

vec2 hrg_sphere_xyz_to_uv(vec3 intersection_pos_local, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   An xyz intersection position on a sphere.
    //  Returns:    video_uv coords mapped to range [-0.5, 0.5]
    //  Mapping:    First define square_uv.x/square_uv.y ==
    //              intersection_pos_local.x/intersection_pos_local.y.  Then,
    //              length(square_uv) is the arc length from the image center
    //              at (0, 0, in_geom_radius) along the tangent great circle.
    //              Credit for this mapping goes to cgwg: I never managed to
    //              understand his code, but he told me his mapping was based on
    //              great circle distances when I asked him about it, which
    //              informed this very similar (almost identical) mapping.
    //  Start with a numerically robust arc length calculation between the ray-
    //  sphere intersection point and the image center using a method posted by
    //  Roger Stafford on comp.soft-sys.matlab:
    //  https://groups.google.com/d/msg/comp.soft-sys.matlab/zNbUui3bjcA/c0HV_bHSx9cJ
    vec3 image_center_pos_local = vec3(0, 0, in_geom_radius);
    float cp_len =
        length(cross(intersection_pos_local, image_center_pos_local));
    float dp = dot(intersection_pos_local, image_center_pos_local);
    float angle_from_image_center = atan(cp_len, dp);
    float arc_len = angle_from_image_center * in_geom_radius;
    //  Get a uv-mapping where [-0.5, 0.5] maps to a "square" area, then divide
    //  by the aspect ratio to stretch the mapping appropriately:
    vec2 square_uv_unit = normalize(vec2(intersection_pos_local.x, -intersection_pos_local.y));
    vec2 square_uv = arc_len * square_uv_unit;
    vec2 video_uv = square_uv / output_aspect;
    return video_uv;
}

vec3 hrg_sphere_uv_to_xyz(vec2 video_uv, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   video_uv coords mapped to range [-0.5, 0.5]
    //  Returns:    An xyz intersection position on a sphere.  This is the
    //              inverse of hrg_sphere_xyz_to_uv().
    //  Expand video_uv by the aspect ratio to get proportionate x/y lengths,
    //  then calculate an xyz position for the spherical mapping above.
    vec2 square_uv = video_uv * output_aspect;
    //  Using length or sqrt here butchers the framerate on my 8800GTS if
    //  this function is called too many times, and so does taking the max
    //  component of square_uv/square_uv_unit (program length threshold?).
    //float arc_len = length(square_uv);
    vec2 square_uv_unit = normalize(square_uv);
    float arc_len = square_uv.y/square_uv_unit.y;
    float angle_from_image_center = arc_len / in_geom_radius;
    float xy_dist_from_sphere_center = sin(angle_from_image_center) * in_geom_radius;
    //vec2 xy_pos = xy_dist_from_sphere_center * (square_uv/FIX_ZERO(arc_len));
    vec2 xy_pos = xy_dist_from_sphere_center * square_uv_unit;
    float z_pos = cos(angle_from_image_center) * in_geom_radius;
    vec3 intersection_pos_local = vec3(xy_pos.x, -xy_pos.y, z_pos);
    return intersection_pos_local;
}

vec2 hrg_sphere_alt_xyz_to_uv(vec3 intersection_pos_local, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   An xyz intersection position on a cylinder.
    //  Returns:    video_uv coords mapped to range [-0.5, 0.5]
    //  Mapping:    Define square_uv.x to be the signed arc length in xz-space,
    //              and define square_uv.y == signed arc length in yz-space.
    //  See hrg_cylinder_xyz_to_uv() for implementation details (very similar).
    vec2 angle_from_image_center = atan( vec2(intersection_pos_local.x, -intersection_pos_local.y),
                                                intersection_pos_local.zz);
    vec2 signed_arc_len = angle_from_image_center * in_geom_radius;
    vec2 video_uv = signed_arc_len / output_aspect;
    return video_uv;
}

vec3 hrg_sphere_alt_uv_to_xyz(vec2 video_uv, vec2 output_aspect, float in_geom_radius)
{
    //  Requires:   video_uv coords mapped to range [-0.5, 0.5]
    //  Returns:    An xyz intersection position on a sphere.  This is the
    //              inverse of hrg_sphere_alt_xyz_to_uv().
    //  See hrg_cylinder_uv_to_xyz() for implementation details (very similar).
    vec2 square_uv = video_uv * output_aspect;
    vec2 arc_len = square_uv;
    vec2 angle_from_image_center = arc_len / in_geom_radius;
    vec2 xy_pos = sin(angle_from_image_center) * in_geom_radius;
    float z_pos = sqrt(in_geom_radius * in_geom_radius - dot(xy_pos, xy_pos));
    return vec3(xy_pos.x, -xy_pos.y, z_pos);
}

vec2 hrg_intersect(vec3 view_vec_local, vec3 eye_pos_local, float in_geom_mode, float in_geom_radius)
{
    return in_geom_mode < 2.5 ?    hrg_intersect_sphere(view_vec_local, eye_pos_local, in_geom_radius) :
                                hrg_intersect_cylinder(view_vec_local, eye_pos_local, in_geom_radius);
}

vec2 hrg_xyz_to_uv( vec3 intersection_pos_local, vec2 output_aspect, float in_geom_mode, float in_geom_radius)
{
    return in_geom_mode < 1.5 ?    hrg_sphere_xyz_to_uv(intersection_pos_local, output_aspect, in_geom_radius) :
                                in_geom_mode < 2.5 ?   hrg_sphere_alt_xyz_to_uv(intersection_pos_local, output_aspect, in_geom_radius) :
                                                    hrg_cylinder_xyz_to_uv(intersection_pos_local, output_aspect, in_geom_radius);
}

vec3 hrg_uv_to_xyz(vec2 uv, vec2 output_aspect, float in_geom_mode, float in_geom_radius)
{
    return in_geom_mode < 1.5 ? hrg_sphere_uv_to_xyz(uv, output_aspect, in_geom_radius) :
                             in_geom_mode < 2.5 ? hrg_sphere_alt_uv_to_xyz(uv, output_aspect, in_geom_radius) :
                                               hrg_cylinder_uv_to_xyz(uv, output_aspect, in_geom_radius);
}

vec2 hrg_view_vec_to_uv(vec3 view_vec_local, 
                    vec3 eye_pos_local,
                    vec2 output_aspect, 
                    float in_geom_mode,
                    float in_geom_radius, 
                    out vec3 intersection_pos)
{
    //  Get the intersection point on the primitive, given an eye position
    //  and view vector already in its local coordinate frame:
    vec2 intersect_dist_and_discriminant = hrg_intersect(view_vec_local, eye_pos_local, in_geom_mode, in_geom_radius);
    vec3 intersection_pos_local = eye_pos_local + view_vec_local * intersect_dist_and_discriminant.x;
    //  Save the intersection position to an output parameter:
    intersection_pos = intersection_pos_local;
    //  Transform into uv coords, but give out-of-range coords if the
    //  view ray doesn't hrg_intersect the primitive in the first place:
    return intersect_dist_and_discriminant.y > 0.005 ?  hrg_xyz_to_uv(intersection_pos_local, output_aspect, in_geom_mode, in_geom_radius) : 
                                                        vec2(1);
}

vec3 hrg_get_ideal_global_eye_pos_for_points(   vec3 eye_pos,
                                            vec2 output_aspect, 
                                            vec3 global_coords[HRG_MAX_POINT_CLOUD_SIZE],
                                            int num_points,
                                            float in_geom_radius,
                                            float in_geom_view_dist)
{
    //  Requires:   Parameters:
    //              1.) Starting eye_pos is a global 3D position at which the
    //                  camera contains all points in global_coords[] in its FOV
    //              2.) output_aspect = hrg_get_aspect_vector(
    //                      IN.OutputSize.xy.x / IN.OutputSize.xy.y);
    //              3.) global_coords is a point cloud containing global xyz
    //                  coords of extreme points on the simulated CRT screen.
    //              Globals:
    //              1.) in_geom_view_dist must be > 0.  It controls the "near
    //                  plane" used to interpret flat_video_uv as a view
    //                  vector, which controls the field of view (FOV).
    //              Eyespace coordinate frame: +x = right, +y = up, +z = back
    //  Returns:    Return an eye position at which the point cloud spans as
    //              much of the screen as possible (given the FOV controlled by
    //              in_geom_view_dist) without being cropped or sheared.
    //  Algorithm:
    //  1.) Move the eye laterally to a point which attempts to maximize the
    //      the amount we can move forward without clipping the CRT screen.
    //  2.) Move forward by as much as possible without clipping the CRT.
    //  Get the allowed movement range by solving for the eye_pos offsets
    //  that result in each point being projected to a screen edge/corner in
    //  pseudo-normalized device coords (where xy ranges from [-0.5, 0.5]
    //  and z = eyespace z):
    //      pndc_coord = vec3(vec2(eyespace_xyz.x, -eyespace_xyz.y)*
    //      in_geom_view_dist / (output_aspect * -eyespace_xyz.z), eyespace_xyz.z);
    //  Notes:
    //  The field of view is controlled by in_geom_view_dist's magnitude relative to
    //  the view vector's x and y components:
    //      view_vec.xy ranges from [-0.5, 0.5] * output_aspect
    //      view_vec.z = -in_geom_view_dist
    //  But for the purposes of perspective divide, it should be considered:
    //      view_vec.xy ranges from [-0.5, 0.5] * output_aspect / in_geom_view_dist
    //      view_vec.z = -1
    int max_centering_iters = 1;  //  Keep for easy testing.
    for(int iter = 0; iter < max_centering_iters; iter++)
    {
        //  0.) Get the eyespace coordinates of our point cloud:
        vec3 eyespace_coords[HRG_MAX_POINT_CLOUD_SIZE];
        for(int i = 0; i < num_points; i++)
        {
            eyespace_coords[i] = global_coords[i] - eye_pos;
        }
        //  1a.)For each point, find out how far we can move eye_pos in each
        //      lateral direction without the point clipping the frustum.
        //      Eyespace +y = up, screenspace +y = down, so flip y after
        //      applying the eyespace offset (on the way to "clip space").
        //  Solve for two offsets per point based on:
        //      (eyespace_xyz.xy - offset_dr) * vec2(1, -1) *
        //      in_geom_view_dist / (output_aspect * -eyespace_xyz.z) = vec2(-0.5)
        //      (eyespace_xyz.xy - offset_dr) * vec2(1, -1) *
        //      in_geom_view_dist / (output_aspect * -eyespace_xyz.z) = vec2(0.5)
        //  offset_ul and offset_dr represent the farthest we can move the
        //  eye_pos up-left and down-right.  Save the min of all offset_dr's
        //  and the max of all offset_ul's (since it's negative).
        float abs_radius = abs(in_geom_radius);  //  In case anyone gets ideas. ;)
        vec2 offset_dr_min = vec2(10.0 * abs_radius, 10.0 * abs_radius);
        vec2 offset_ul_max = vec2(-10.0 * abs_radius, -10.0 * abs_radius);
        for(int i = 0; i < num_points; i++)
        {
            vec2 flipy = vec2(1, -1);
            vec3 eyespace_xyz = eyespace_coords[i];
            vec2 offset_dr = eyespace_xyz.xy - vec2(-0.5) *
                            (output_aspect * -eyespace_xyz.z) / 
                            (in_geom_view_dist * flipy);
            vec2 offset_ul = eyespace_xyz.xy - vec2(0.5) *
                            (output_aspect * -eyespace_xyz.z) / 
                            (in_geom_view_dist * flipy);
            offset_dr_min = min(offset_dr_min, offset_dr);
            offset_ul_max = max(offset_ul_max, offset_ul);
        }
        //  1b.)Update eye_pos: Adding the average of offset_ul_max and
        //      offset_dr_min gives it equal leeway on the top vs. bottom
        //      and left vs. right.  Recalculate eyespace_coords accordingly.
        vec2 center_offset = 0.5 * (offset_ul_max + offset_dr_min);
        eye_pos.xy += center_offset;
        for(int i = 0; i < num_points; i++)
        {
            eyespace_coords[i] = global_coords[i] - eye_pos;
        }
        //  2a.)For each point, find out how far we can move eye_pos forward
        //      without the point clipping the frustum.  Flip the y
        //      direction in advance (matters for a later step, not here).
        //      Solve for four offsets per point based on:
        //      eyespace_xyz_flipy.x * in_geom_view_dist /
        //          (output_aspect.x * (offset_z - eyespace_xyz_flipy.z)) =-0.5
        //      eyespace_xyz_flipy.y * in_geom_view_dist /
        //          (output_aspect.y * (offset_z - eyespace_xyz_flipy.z)) =-0.5
        //      eyespace_xyz_flipy.x * in_geom_view_dist /
        //          (output_aspect.x * (offset_z - eyespace_xyz_flipy.z)) = 0.5
        //      eyespace_xyz_flipy.y * in_geom_view_dist /
        //          (output_aspect.y * (offset_z - eyespace_xyz_flipy.z)) = 0.5
        //      We'll vectorize the actual computation.  Take the maximum of
        //      these four for a single offset, and continue taking the max
        //      for every point (use max because offset.z is negative).
        float offset_z_max = -10.0 * in_geom_radius * in_geom_view_dist;
        for(int i = 0; i < num_points; i++)
        {
            vec3 eyespace_xyz_flipy = eyespace_coords[i] * vec3(1, -1, 1);
            vec4 offset_zzzz = eyespace_xyz_flipy.zzzz +  
                                (eyespace_xyz_flipy.xyxy * in_geom_view_dist) /
                                (vec4(-0.5, -0.5, 0.5, 0.5) * vec4(output_aspect, output_aspect));
            //  Ignore offsets that push positive x/y values to opposite
            //  boundaries, and vice versa, and don't let the camera move
            //  past a point in the dead center of the screen:
            offset_z_max = (eyespace_xyz_flipy.x < 0) ? max(offset_z_max, offset_zzzz.x) : offset_z_max;
            offset_z_max = (eyespace_xyz_flipy.y < 0) ? max(offset_z_max, offset_zzzz.y) : offset_z_max;
            offset_z_max = (eyespace_xyz_flipy.x > 0) ? max(offset_z_max, offset_zzzz.z) : offset_z_max;
            offset_z_max = (eyespace_xyz_flipy.y > 0) ? max(offset_z_max, offset_zzzz.w) : offset_z_max;
            offset_z_max = max(offset_z_max, eyespace_xyz_flipy.z);
        }
        //  2b.)Update eye_pos: Add the maximum (smallest negative) z offset.
        eye_pos.z += offset_z_max;
    }
    return eye_pos;
}

vec3 hrg_get_ideal_global_eye_pos(  mat3x3 local_to_global,
                                    vec2 output_aspect, 
                                    float in_geom_mode, 
                                    float in_geom_radius, 
                                    float in_geom_view_dist)
{
    //  Start with an initial eye_pos that includes the entire primitive
    //  (sphere or cylinder) in its field-of-view:
    vec3 high_view = vec3(0, output_aspect.y, -in_geom_view_dist);
    vec3 low_view = high_view * vec3(1, -1, 1);
    float len_sq = dot(high_view, high_view);
    float fov = abs(acos(dot(high_view, low_view)/len_sq));
    //  Trigonometry/similar triangles say distance = in_geom_radius/sin(fov/2):
    float eye_z_spherical = in_geom_radius / sin(fov*0.5);
    vec3 eye_pos = in_geom_mode < 2.5 ?  vec3(0, 0, eye_z_spherical) :
                                            vec3(0, 0, max(in_geom_view_dist, eye_z_spherical));

    //  Get global xyz coords of extreme sample points on the simulated CRT
    //  screen.  Start with the center, edge centers, and corners of the
    //  video image.  We can't ignore backfacing points: They're occluded
    //  by closer points on the primitive, but they may NOT be occluded by
    //  the convex hull of the remaining samples (i.e. the remaining convex
    //  hull might not envelope points that do occlude a back-facing point.)
    int num_points = HRG_MAX_POINT_CLOUD_SIZE;
    vec3 global_coords[HRG_MAX_POINT_CLOUD_SIZE];
    global_coords[0] = hrg_uv_to_xyz(vec2(0,     0),   output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[1] = hrg_uv_to_xyz(vec2(0,    -0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[2] = hrg_uv_to_xyz(vec2(0,     0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[3] = hrg_uv_to_xyz(vec2(-0.5,  0),   output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[4] = hrg_uv_to_xyz(vec2(0.5,   0),   output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[5] = hrg_uv_to_xyz(vec2(-0.5, -0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[6] = hrg_uv_to_xyz(vec2(0.5,  -0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[7] = hrg_uv_to_xyz(vec2(-0.5,  0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    global_coords[8] = hrg_uv_to_xyz(vec2(0.5,   0.5), output_aspect, in_geom_mode, in_geom_radius) * local_to_global;
    //  Adding more inner image points could help in extreme cases, but too many
    //  points will kille the framerate.  For safety, default to the initial
    //  eye_pos if any z coords are negative:
    float num_negative_z_coords = 0;
    for(int i = 0; i < num_points; i++)
    {
        num_negative_z_coords += float(global_coords[0].z < 0);
    }
    //  Outsource the optimized eye_pos calculation:
    return num_negative_z_coords > 0.5 ?    eye_pos :
                                            hrg_get_ideal_global_eye_pos_for_points(eye_pos, 
                                                                                    output_aspect,
                                                                                    global_coords, 
                                                                                    num_points, 
                                                                                    in_geom_radius, 
                                                                                    in_geom_view_dist);
}

mat3x3 hrg_get_pixel_to_object_matrix(  mat3x3 global_to_local,
                                    vec3 eye_pos_local, 
                                    vec3 view_vec_global,
                                    vec3 intersection_pos_local, 
                                    vec3 normal,
                                    vec2 output_pixel_size)
{
    //  Requires:   See hrg_get_curved_video_uv_coords_and_tangent_matrix for
    //              descriptions of each parameter.
    //  Returns:    Return a transformation matrix from 2D pixel-space vectors
    //              (where (+1, +1) is a vector to one pixel down-right,
    //              i.e. same directionality as uv texels) to 3D object-space
    //              vectors in the CRT's local coordinate frame (right-handed)
    //              ***which are tangent to the CRT surface at the intersection
    //              position.***  (Basically, we want to convert pixel-space
    //              vectors to 3D vectors along the CRT's surface, for later
    //              conversion to uv vectors.)
    //  Shorthand inputs:
    vec3 pos = intersection_pos_local;
    vec3 eye_pos = eye_pos_local;
    //  Get a piecewise-linear matrix transforming from "pixelspace" offset
    //  vectors (1 = one pixel) to object space vectors in the tangent
    //  plane (faster than finding 3 view-object intersections).
    //  1.) Get the local view vecs for the pixels to the right and down:
    vec3 view_vec_right_global =  view_vec_global + vec3(output_pixel_size.x, 0, 0);
    vec3 view_vec_down_global =   view_vec_global + vec3(0, -output_pixel_size.y, 0);
    vec3 view_vec_right_local =   view_vec_right_global * global_to_local;
    vec3 view_vec_down_local =    view_vec_down_global * global_to_local;
    //  2.) Using the true intersection point, hrg_intersect the neighboring
    //      view vectors with the tangent plane:
    vec3 intersection_vec_dot_normal = vec3(dot(pos - eye_pos, normal));
    vec3 right_pos =    eye_pos + 
                        (intersection_vec_dot_normal / dot(view_vec_right_local, normal)) *
                        view_vec_right_local;
    vec3 down_pos =     eye_pos + 
                        (intersection_vec_dot_normal / dot(view_vec_down_local, normal)) * 
                        view_vec_down_local;
    //  3.) Subtract the original intersection pos from its neighbors; the
    //      resulting vectors are object-space vectors tangent to the plane.
    //      These vectors are the object-space transformations of (1, 0)
    //      and (0, 1) pixel offsets, so they form the first two basis
    //      vectors of a pixelspace to object space transformation.  This
    //      transformation is 2D to 3D, so use (0, 0, 0) for the third vector.
    vec3 object_right_vec = right_pos - pos;
    vec3 object_down_vec = down_pos - pos;
    mat3x3 pixel_to_object = mat3x3(
                                    object_right_vec.x, object_down_vec.x, 0,
                                    object_right_vec.y, object_down_vec.y, 0,
                                    object_right_vec.z, object_down_vec.z, 0
                                    );
    return pixel_to_object;
}

mat3x3 hrg_get_object_to_tangent_matrix(vec3 intersection_pos_local,
                                    vec3 normal, 
                                    vec2 output_aspect, 
                                    float in_geom_mode)
{
    //  Requires:   See hrg_get_curved_video_uv_coords_and_tangent_matrix for
    //              descriptions of each parameter.
    //  Returns:    Return a transformation matrix from 3D object-space vectors
    //              in the CRT's local coordinate frame (right-handed, +y = up)
    //              to 2D video_uv vectors (+v = down).
    //  Description:
    //  The TBN matrix formed by the [tangent, bitangent, normal] basis
    //  vectors transforms ordinary vectors from tangent->object space.
    //  The cotangent matrix formed by the [cotangent, cobitangent, normal]
    //  basis vectors transforms normal vectors (covectors) from
    //  tangent->object space.  It's the inverse-transpose of the TBN matrix.
    //  We want the inverse of the TBN matrix (transpose of the cotangent
    //  matrix), which transforms ordinary vectors from object->tangent space.
    //  Start by calculating the relevant basis vectors in accordance with
    //  Christian Schüler's blog post "Followup: Normal Mapping Without
    //  Precomputed Tangents":  http://www.thetenthplanet.de/archives/1180
    //  With our particular uv mapping, the scale of the u and v directions
    //  is determined entirely by the aspect ratio for cylindrical and ordinary
    //  spherical mappings, and so tangent and bitangent lengths are also
    //  determined by it (the alternate mapping is more complex).  Therefore, we
    //  must ensure appropriate cotangent and cobitangent lengths as well.
    //  Base these off the uv<=>xyz mappings for each primitive.
    vec3 pos = intersection_pos_local;
    vec3 x_vec = vec3(1, 0, 0);
    vec3 y_vec = vec3(0, 1, 0);
    //  The tangent and bitangent vectors correspond with increasing u and v,
    //  respectively.  Mathematically we'd base the cotangent/cobitangent on
    //  those, but we'll compute the cotangent/cobitangent directly when we can.
    vec3 cotangent_unscaled;
    vec3 cobitangent_unscaled;
    //  in_geom_mode should be constant-folded without RUNTIME_GEOMETRY_MODE.
    if(in_geom_mode < 1.5)
    {
        //  Sphere:
        //  tangent = normalize(cross(normal, cross(x_vec, pos))) * output_aspect.x
        //  bitangent = normalize(cross(cross(y_vec, pos), normal)) * output_aspect.y
        //  inv_determinant = 1/length(cross(bitangent, tangent))
        //  cotangent = cross(normal, bitangent) * inv_determinant
        //            == normalize(cross(y_vec, pos)) * output_aspect.y * inv_determinant
        //  cobitangent = cross(tangent, normal) * inv_determinant
        //            == normalize(cross(x_vec, pos)) * output_aspect.x * inv_determinant
        //  Simplified (scale by inv_determinant below):
        cotangent_unscaled = normalize(cross(y_vec, pos)) * output_aspect.y;
        cobitangent_unscaled = normalize(cross(x_vec, pos)) * output_aspect.x;
    }
    else if(in_geom_mode < 2.5)
    {
        //  Sphere, alternate mapping:
        //  This mapping works a bit like the cylindrical mapping in two
        //  directions, which makes the lengths and directions more complex.
        //  Unfortunately, I can't find much of a shortcut:
        vec3 tangent = normalize(cross(y_vec, vec3(pos.x, 0, pos.z))) * output_aspect.x;
        vec3 bitangent = normalize(cross(x_vec, vec3(0, pos.yz))) * output_aspect.y;
        cotangent_unscaled = cross(normal, bitangent);
        cobitangent_unscaled = cross(tangent, normal);
    }
    else
    {
        //  Cylinder:
        //  tangent = normalize(cross(y_vec, normal)) * output_aspect.x;
        //  bitangent = vec3(0, -output_aspect.y, 0);
        //  inv_determinant = 1/length(cross(bitangent, tangent))
        //  cotangent = cross(normal, bitangent) * inv_determinant
        //            == normalize(cross(y_vec, pos)) * output_aspect.y * inv_determinant
        //  cobitangent = cross(tangent, normal) * inv_determinant
        //            == vec3(0, -output_aspect.x, 0) * inv_determinant
        cotangent_unscaled = cross(y_vec, normal) * output_aspect.y;
        cobitangent_unscaled = vec3(0, -output_aspect.x, 0);
    }
    vec3 computed_normal = cross(cobitangent_unscaled, cotangent_unscaled);
    float inv_determinant = inversesqrt(dot(computed_normal, computed_normal));
    vec3 cotangent = cotangent_unscaled * inv_determinant;
    vec3 cobitangent = cobitangent_unscaled * inv_determinant;
    //  The [cotangent, cobitangent, normal] column vecs form the cotangent
    //  frame, i.e. the inverse-transpose TBN matrix.  Get its transpose:
    mat3x3 object_to_tangent = mat3x3(  cotangent, 
                                        cobitangent, 
                                        normal);
    return object_to_tangent;
}

vec2 hrg_get_curved_video_uv_coords_and_tangent_matrix( vec2 flat_video_uv, 
                                                    vec3 eye_pos_local,
                                                    vec2 output_pixel_size, 
                                                    vec2 output_aspect,
                                                    float in_geom_mode, 
                                                    float in_geom_radius, 
                                                    float in_geom_view_dist,
                                                    mat3x3 global_to_local,
                                                    out mat2x2 pixel_to_tangent_video_uv)
{
    //  Requires:   Parameters:
    //              1.) flat_video_uv coords are in range [0, 1], where
    //                  (0, 0) is the top-left corner of the screen and
    //                  (1, 1) is the bottom-right corner.
    //              2.) eye_pos_local is the 3D camera position in the simulated
    //                  CRT's local coordinate frame.  For best results, it must
    //                  be computed based on the same in_geom_view_dist used here.
    //              3.) output_pixel_size = vec2(1)/IN.OutputSize.xy
    //              4.) output_aspect = hrg_get_aspect_vector(
    //                      IN.OutputSize.xy.x / IN.OutputSize.xy.y);
    //              5.) in_geom_mode is a static or runtime mode setting:
    //                  0 = off, 1 = sphere, 2 = sphere alt., 3 = cylinder
    //              6.) global_to_local is a 3x3 matrix transforming (ordinary)
    //                  worldspace vectors to the CRT's local coordinate frame
    //              Globals:
    //              1.) in_geom_view_dist must be > 0.  It controls the "near
    //                  plane" used to interpret flat_video_uv as a view
    //                  vector, which controls the field of view (FOV).
    //  Returns:    Return final uv coords in [0, 1], and return a pixel-
    //              space to video_uv tangent-space matrix in the out parameter.
    //              (This matrix assumes pixel-space +y = down, like +v = down.)
    //              We'll transform flat_video_uv into a view vector, project
    //              the view vector from the camera/eye, hrg_intersect with a sphere
    //              or cylinder representing the simulated CRT, and convert the
    //              intersection position into final uv coords and a local
    //              transformation matrix.
    //  First get the 3D view vector (output_aspect and in_geom_view_dist are globals):
    //  1.) Center uv around (0, 0) and make (-0.5, -0.5) and (0.5, 0.5)
    //      correspond to the top-left/bottom-right output screen corners.
    //  2.) Multiply by output_aspect to preemptively "undo" Retroarch's screen-
    //      space 2D aspect correction.  We'll reapply it in uv-space.
    //  3.) (x, y) = (u, -v), because +v is down in 2D screenspace, but +y
    //      is up in 3D worldspace (enforce a right-handed system).
    //  4.) The view vector z controls the "near plane" distance and FOV.
    //      For the effect of "looking through a window" at a CRT, it should be
    //      set equal to the user's distance from their physical screen, in
    //      units of the viewport's physical diagonal size.
    vec2 view_uv = (flat_video_uv - vec2(0.5)) * output_aspect;
    vec3 view_vec_global = vec3(view_uv.x, -view_uv.y, -in_geom_view_dist);
    //  Transform the view vector into the CRT's local coordinate frame, convert
    //  to video_uv coords, and get the local 3D intersection position:
    vec3 view_vec_local = view_vec_global * global_to_local;
    vec3 pos;
    vec2 centered_uv = hrg_view_vec_to_uv(  view_vec_local, 
                                        eye_pos_local, 
                                        output_aspect, 
                                        in_geom_mode, 
                                        in_geom_radius, 
                                        pos);
    vec2 video_uv = centered_uv + vec2(0.5);
    //  Get a pixel-to-tangent-video-uv matrix.  The caller could deal with
    //  all but one of these cases, but that would be more complicated.
    //#ifdef DRIVERS_ALLOW_DERIVATIVES
        //  Derivatives obtain a matrix very fast, but the direction of pixel-
        //  space +y seems to depend on the pass.  Enforce the correct direction
        //  on a best-effort basis (but it shouldn't matter for antialiasing).
        // vec2 duv_dx = dFdx(video_uv);
        // vec2 duv_dy = dFdy(video_uv);
        // // #ifdef LAST_PASS
        //     pixel_to_tangent_video_uv = mat2x2(    duv_dx.x,   duv_dy.x,
        //                                             -duv_dx.y,  -duv_dy.y);
    //     #else
    //         pixel_to_tangent_video_uv = mat2x2(   duv_dx.x,   duv_dy.x,
    //                                                 duv_dx.y,   duv_dy.y);
    //     #endif
    // #else
        //  Manually define a transformation matrix.  We'll assume pixel-space
        //  +y = down, just like +v = down.
        bool geom_force_correct_tangent_matrix = true;
        if(geom_force_correct_tangent_matrix)
        {
            //  Get the surface normal based on the local intersection position:
            vec3 normal_base = in_geom_mode < 2.5 ?    pos :
                                                    vec3(pos.x, 0, pos.z);
            vec3 normal = normalize(normal_base);
            //  Get pixel-to-object and object-to-tangent matrices and combine
            //  them into a 2x2 pixel-to-tangent matrix for video_uv offsets:
            mat3x3 pixel_to_object = hrg_get_pixel_to_object_matrix(global_to_local, 
                                                                eye_pos_local, 
                                                                view_vec_global, 
                                                                pos, 
                                                                normal,
                                                                output_pixel_size);
            mat3x3 object_to_tangent = hrg_get_object_to_tangent_matrix(pos, normal, output_aspect, in_geom_mode);
            mat3x3 pixel_to_tangent3x3 = pixel_to_object * object_to_tangent;
            pixel_to_tangent_video_uv = mat2x2(   pixel_to_tangent3x3[0][0], pixel_to_tangent3x3[0][1], 
                                                    pixel_to_tangent3x3[1][0], pixel_to_tangent3x3[1][1]);//._m00_m01_m10_m11);
        }
        else
        {
            //  Ignore curvature, and just consider flat scaling.  The
            //  difference is only apparent with strong curvature:
            pixel_to_tangent_video_uv = mat2x2(   output_pixel_size.x,    0, 
                                                    0,                      output_pixel_size.y);
        }
    //#endif
    return video_uv;
}

float HRG_GetBorderDimFactor(vec2 video_uv, vec2 output_aspect, float in_border_size, float in_border_darkness, float in_border_compress)
{
    //  COPYRIGHT NOTE FOR THIS FUNCTION:
    //  Copyright (C) 2010-2012 cgwg, 2014 TroggleMonkey
    //  This function uses an algorithm first coded in several of cgwg's GPL-
    //  licensed lines in crt-geom-curved.cg and its ancestors.

    //  Calculate border_dim_factor from the proximity to uv-space image
    //  borders; output_aspect/in_border_size/border/darkness/in_border_compress are globals:
    vec2 edge_dists = min(video_uv, vec2(1) - video_uv) * output_aspect;
    vec2 border_penetration = max(vec2(in_border_size) - edge_dists, vec2(0));
    float penetration_ratio = length(border_penetration)/in_border_size;
    float border_escape_ratio = max(1 - penetration_ratio, 0);
    float border_dim_factor = pow(border_escape_ratio, in_border_darkness) * max(1, in_border_compress);
    return min(border_dim_factor, 1);
}

//  Provide accessors for vector constants that pack scalar uniforms:
vec2 hrg_get_aspect_vector(float geom_aspect_ratio)
{
    //  Get an aspect ratio vector.  Enforce geom_max_aspect_ratio, and prevent
    //  the absolute scale from affecting the uv-mapping for curvature:
    float geom_max_aspect_ratio = 4/3;
    float geom_clamped_aspect_ratio = min(geom_aspect_ratio, geom_max_aspect_ratio);
    vec2 output_aspect = normalize(vec2(geom_clamped_aspect_ratio, 1));
    return output_aspect;
}

vec2 HRG_GetGeomCurvedCoord(    vec2 in_coord, 
                                float in_geom_mode, 
                                float in_geom_radius, 
                                float in_geom_view_dist,
                                float in_geom_tilt_angle_x, 
                                float in_geom_tilt_angle_y,
                                float in_screen_aspect,
                                float pin_inner_edge,
                                vec2 in_source_size,
                                vec2 in_output_size,
                                out mat2x2 pixel_to_video_uv)
{
    vec2 output_pixel_size = vec2(1.0, 1.0) / in_output_size;

    float geom_radius_scaled = in_geom_radius;

    vec2 output_aspect = hrg_get_aspect_vector(in_screen_aspect);

    //  Create a local-to-global rotation matrix for the CRT's coordinate
    //  frame and its global-to-local inverse.  Rotate around the x axis
    //  first (pitch) and then the y axis (yaw) with yucky Euler angles.
    //  Positive angles go clockwise around the right-vec and up-vec.
    vec2 geom_tilt_angle = vec2(in_geom_tilt_angle_x, in_geom_tilt_angle_y);
    vec2 sin_tilt = sin(geom_tilt_angle);
    vec2 cos_tilt = cos(geom_tilt_angle);

    //  Conceptual breakdown:
    mat3x3 rot_x_matrix = mat3x3(   1,              0,              0,
                                    0,              cos_tilt.y,     -sin_tilt.y,
                                    0,              sin_tilt.y,     cos_tilt.y);
                                
    mat3x3 rot_y_matrix = mat3x3(   cos_tilt.x,     0,              sin_tilt.x,
                                    0,              1,              0,
                                    -sin_tilt.x,    0,              cos_tilt.x);

    mat3x3 local_to_global = rot_x_matrix * rot_y_matrix;
    //  This is a pure rotation, so transpose = inverse:
    mat3x3 global_to_local = transpose(local_to_global);

    //  Get an optimal eye position based on in_geom_view_dist, viewport_aspect,
    //  and CRT radius/rotation:
    vec3 eye_pos_global = hrg_get_ideal_global_eye_pos( local_to_global, 
                                                        output_aspect, 
                                                        in_geom_mode, 
                                                        geom_radius_scaled, 
                                                        in_geom_view_dist);
    vec3 eye_pos_local = eye_pos_global * global_to_local;


    vec2 curved_coord;

    if(in_geom_mode > 0.5)
    {
        // Put in a test for the projection with a flat plane to compare 
        // with the distorted coordinate to scale out to the edges of the flat plane
        // Also helps with cyndrilical projection where the sides shift in towards the center
        vec2 ctr_curved_coord =  hrg_get_curved_video_uv_coords_and_tangent_matrix( in_coord,
                                                                                        eye_pos_local, 
                                                                                        output_pixel_size, 
                                                                                        output_aspect,
                                                                                        in_geom_mode, 
                                                                                        geom_radius_scaled,
                                                                                        in_geom_view_dist,
                                                                                        global_to_local, 
                                                                                        pixel_to_video_uv) - 0.5;

        // Curvature can cause the screen to shrink so we want to scale it back out so it is the same width & height
        // Especially helps with cylindrical projection which shrinks a lot 
        // Right Edge should end up at 1, we scale it back out so it hits 1
        // Only do this when not using tilt so we don't mess up what the perspective is doing
        if (in_geom_tilt_angle_x == 0 && in_geom_tilt_angle_y == 0)
        {
            vec2 right_edge_curved_ctr_coord =  hrg_get_curved_video_uv_coords_and_tangent_matrix(vec2(1, 0.5),
                                                                                            eye_pos_local, 
                                                                                            output_pixel_size, 
                                                                                            output_aspect,
                                                                                            in_geom_mode, 
                                                                                            geom_radius_scaled,
                                                                                            in_geom_view_dist,
                                                                                            global_to_local, 
                                                                                            pixel_to_video_uv) - 0.5;

            vec2 bottom_edge_curved_ctr_coord =  hrg_get_curved_video_uv_coords_and_tangent_matrix(vec2(0.5, 1),
                                                                                            eye_pos_local, 
                                                                                            output_pixel_size, 
                                                                                            output_aspect,
                                                                                            in_geom_mode, 
                                                                                            geom_radius_scaled,
                                                                                            in_geom_view_dist,
                                                                                            global_to_local, 
                                                                                            pixel_to_video_uv) - 0.5;

            ctr_curved_coord.x = ctr_curved_coord.x * 0.5 / right_edge_curved_ctr_coord.x;
            ctr_curved_coord.y = ctr_curved_coord.y * 0.5 / bottom_edge_curved_ctr_coord.y;
        }
        if (pin_inner_edge == 1)
        {
            if (in_geom_tilt_angle_y != 0)
            {
                vec2 top_edge_curved_ctr_coord =  hrg_get_curved_video_uv_coords_and_tangent_matrix(vec2(0.5, 0),
                                                                                                    eye_pos_local, 
                                                                                                    output_pixel_size, 
                                                                                                    output_aspect,
                                                                                                    in_geom_mode, 
                                                                                                    geom_radius_scaled,
                                                                                                    in_geom_view_dist,
                                                                                                    global_to_local, 
                                                                                                    pixel_to_video_uv);
                ctr_curved_coord.y = ctr_curved_coord.y - top_edge_curved_ctr_coord.y;
            }
            if (in_geom_tilt_angle_x != 0)
            {
                vec2 left_edge_curved_ctr_coord =  hrg_get_curved_video_uv_coords_and_tangent_matrix(vec2(0, 0.5),
                                                                                                    eye_pos_local, 
                                                                                                    output_pixel_size, 
                                                                                                    output_aspect,
                                                                                                    in_geom_mode, 
                                                                                                    geom_radius_scaled,
                                                                                                    in_geom_view_dist,
                                                                                                    global_to_local, 
                                                                                                    pixel_to_video_uv);
                ctr_curved_coord.x = ctr_curved_coord.x - left_edge_curved_ctr_coord.x;
            }
        }

        curved_coord = ctr_curved_coord + 0.5;
    }
    else
    {
        curved_coord = in_coord;
        pixel_to_video_uv = mat2x2( output_pixel_size.x,    0, 
                                    0,                      output_pixel_size.y);
    }

    return curved_coord;
}


