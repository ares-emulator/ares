#ifndef CURV
#define CURV

#pragma parameter warpX "Warp X" 0.031 0.0 0.125 0.01
#pragma parameter warpY "Warp Y" 0.041 0.0 0.125 0.01

// Distortion of scanlines, and end of screen alpha.
vec2 warp(vec2 pos)
{
    pos  = pos*2.0-1.0;    
    pos *= vec2(1.0 + (pos.y*pos.y)*warpX, 1.0 + (pos.x*pos.x)*warpY);
    
    return pos*0.5 + 0.5;
}

#endif
