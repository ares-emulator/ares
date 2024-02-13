#ifndef CURV
#define CURV

#pragma parameter Radius "cgwg Curvature Radius" 2.0 0.1 10.0 0.1
#pragma parameter Distance "cgwg Viewing Distance" 1.5 0.1 3.0 0.1
#pragma parameter x_tilt "Horizontal Tilt" 0.0 -0.5 0.5 0.05
#pragma parameter y_tilt "Vertical Tilt" 0.0 -0.5 0.5 0.05

// cgwg's geom
// license: GPLv2

vec2 sinangle = sin(vec2(x_tilt, y_tilt));
vec2 cosangle = cos(vec2(x_tilt, y_tilt));
const vec2 aspect   = vec2(1.0,  0.75);
#define FIX(c) max(abs(c), 1e-5);

float intersect(vec2 xy)
{
    float A = dot(xy,xy) + Distance*Distance;
    float B = 2.0*(Radius*(dot(xy,sinangle) - Distance*cosangle.x*cosangle.y) - Distance*Distance);
    float C = Distance*Distance + 2.0*Radius*Distance*cosangle.x*cosangle.y;
    
    return (-B-sqrt(B*B - 4.0*A*C))/(2.0*A);
}

vec2 fwtrans(vec2 uv)
{
    float r = FIX(sqrt(dot(uv,uv)));
    uv *= sin(r/Radius)/r;
    float x = 1.0-cos(r/Radius);
    float D = Distance/Radius + x*cosangle.x*cosangle.y+dot(uv,sinangle);
    
    return Distance*(uv*cosangle-x*sinangle)/D;
}

vec2 bkwtrans(vec2 xy)
{
    float c     = intersect(xy);
    vec2 point  = (vec2(c, c)*xy - vec2(-Radius, -Radius)*sinangle) / vec2(Radius, Radius);
    vec2 poc    = point/cosangle;
    vec2 tang   = sinangle/cosangle;

    float A     = dot(tang, tang) + 1.0;
    float B     = -2.0*dot(poc, tang);
    float C     = dot(poc, poc) - 1.0;

    float a     = (-B + sqrt(B*B - 4.0*A*C)) / (2.0*A);
    vec2 uv     = (point - a*sinangle) / cosangle;
    float r     = Radius*acos(a);
    r           = FIX(r);
    
    return uv*r/sin(r/Radius);
}

vec3 maxscale()
{
    vec2 c  = bkwtrans(-Radius * sinangle / (1.0 + Radius/Distance*cosangle.x*cosangle.y));
    vec2 a  = vec2(0.5,0.5)*aspect;
    
    vec2 lo = vec2(fwtrans(vec2(-a.x,  c.y)).x,
                   fwtrans(vec2( c.x, -a.y)).y)/aspect;

    vec2 hi = vec2(fwtrans(vec2(+a.x,  c.y)).x,
                   fwtrans(vec2( c.x, +a.y)).y)/aspect;
    
    return vec3((hi+lo)*aspect*0.5,max(hi.x-lo.x,hi.y-lo.y));
}

vec2 warp(vec2 coord)
{
    vec3 stretch = maxscale();
    coord = (coord - vec2(0.5, 0.5))*aspect*stretch.z + stretch.xy;
    
    return (bkwtrans(coord) /
        vec2(1.0, 1.0)/aspect + vec2(0.5, 0.5));
}

#endif
