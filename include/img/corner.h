#ifndef CORNER
#define CORNER

#pragma parameter cornersize "Corner Size" 0.03 0.001 1.0 0.005
#pragma parameter cornersmooth "Corner Smoothness" 1000.0 80.0 2000.0 100.0

const vec2 corner_aspect   = vec2(1.0,  0.75);

float corner(vec2 coord)
{
    coord = (coord - vec2(0.5)) + vec2(0.5, 0.5);
    coord = min(coord, vec2(1.0) - coord) * corner_aspect;
    vec2 cdist = vec2(cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    
    return clamp((cdist.x - dist)*cornersmooth, 0.0, 1.0);
}

#endif
