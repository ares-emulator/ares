// Do a barrel distortion to a given texture coordinate to emulate a curved CRT screen.

#include "cathode-retro-util-language-helpers.inc"

vec2 ApproxAtan2(vec2 x, vec2 y)
{
  // A simple approximation of atan2 will suffice here, just a few terms of the taylor series are good enough for the
  //  angle ranges we're dealing with.
  x /= y;
  vec2 x2 = x*x;
  return x*(1.0 + x2*(x2*0.2 - 0.333333333));
}


vec2 DistortCRTCoordinates(
  // The original texture coordinate, intended to come straight from the full-render-target quad, in [-1..1] range (not
  //  standard [0..1])
  vec2 texCoord,

  // a [horizontal, vertical] distortion pair which describes the effective curvature of the virtual screen.
  vec2 distortion)
{
  if (distortion.x == 0 && distortion.y == 0)
  {
    return texCoord;
  }

  const float k_distance = 2.0;
  const float k_minDistortion = 0.0001;

  // We don't want to let the distortion drop below this minimum value because at 0, the whole technique falls apart.
  // $TODO: It's definitely possible to handle distortion.x or y == 0 as a special case, but I didn't do it for now.
  distortion = max(vec2(k_minDistortion, k_minDistortion), distortion);

  // We're going to cast a ray from a virtual camera position p0 (0, 0, -k_distance) and collide it with a unit sphere.
  //  The horizontal and vertical spread of the rays is determined by the distortion values.
  vec3 ray = vec3(texCoord * distortion, -k_distance);

  // Get the squared length of the ray.
  float rayLenSq = dot(ray, ray);

  // we have an originating point p0 and a ray direction, so we can treat that as a parametric equation:
  //  p = p0 + r*t
  // Since we have a non-translated unit sphere (1 = x^2 + y^2 + z^2) we can sub in our parametric values and solve for
  //  t as a quadratic. To simplify the math, we're going to use a slightly different form of quadratic equation:
  //  t^2 - 2b*t + c == 0
  float b = (k_distance * k_distance) / rayLenSq;
  float c = (k_distance * k_distance - 1.0) / rayLenSq;

  // Given our "t^2 - 2b*t + c == 0" quadratic form, the quadratic formula simplifies to -b +/- sqrt(b^2 - c).
  // Get the nearer of the two coordinates (and do a max inside the sqrt so we still get continuous values outside of
  //  the sphere, even if they're nonsensical, they'll get masked out by our mask value anyway).
  float t = b - sqrt(max(0.0, b*b - c));

  // Get our uv coordinates (Basically, latitude and longitude).
  vec2 uv = ApproxAtan2(ray.xy * t, k_distance + ray.zz * t);

  // maxUV could be calculated on the CPU and passed in for perf.
  // Do the same calculation as above, but for two additional rays: the x, 0, z ray pointing all the way to the right,
  //  and the 0, y, z ray pointing at the bottom (Both rays are packed into the same "diagonal" ray value, but the
  //  lengths are calculated separately). This gets us the uv extents for the rays that we cast, which we can use to
  //  scale our output UVs.
  vec2 maxUV;
  {
    vec3 maxRayDiagonal = vec3(distortion, -k_distance);
    vec2 maxRayLenSq = vec2(dot(maxRayDiagonal.xz, maxRayDiagonal.xz), dot(maxRayDiagonal.yz, maxRayDiagonal.yz));

    vec2 maxB = (k_distance * k_distance) / maxRayLenSq;
    vec2 maxC = (k_distance * k_distance - 1.0) / maxRayLenSq;
    vec2 maxT = maxB - sqrt(max(vec2(0.0, 0.0), maxB*maxB - maxC));
    maxUV = ApproxAtan2(maxRayDiagonal.xy * maxT, k_distance + maxRayDiagonal.zz * maxT);
  }

  // Scale our UVs by the max value.
  return uv / maxUV;
}
