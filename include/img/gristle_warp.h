#ifndef CURV
#define CURV

#pragma parameter warpX "Warp X" 0.031 0.0 0.125 0.01
#pragma parameter warpY "Warp Y" 0.041 0.0 0.125 0.01
vec2 Distortion = vec2(warpX, warpY) * 15.;

vec2 warp(vec2 texCoord){
  vec2 Distortion = vec2(warpX, warpY) * 15.;
  vec2 curvedCoords = texCoord * 2.0 - 1.0;
  float curvedCoordsDistance = sqrt(curvedCoords.x*curvedCoords.x+curvedCoords.y*curvedCoords.y);

  curvedCoords = curvedCoords / curvedCoordsDistance;

  curvedCoords = curvedCoords * (1.0-pow(vec2(1.0-(curvedCoordsDistance/1.4142135623730950488016887242097)),(1.0/(1.0+Distortion*0.2))));

  curvedCoords = curvedCoords / (1.0-pow(vec2(0.29289321881345247559915563789515),(1.0/(vec2(1.0)+Distortion*0.2))));

  curvedCoords = curvedCoords * 0.5 + 0.5;
  return curvedCoords;
}

#endif
