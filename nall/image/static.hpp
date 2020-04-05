#pragma once

namespace nall {

inline auto image::bitDepth(uint64_t color) -> uint {
  uint depth = 0;
  if(color) while((color & 1) == 0) color >>= 1;
  while((color & 1) == 1) { color >>= 1; depth++; }
  return depth;
}

inline auto image::bitShift(uint64_t color) -> uint {
  uint shift = 0;
  if(color) while((color & 1) == 0) { color >>= 1; shift++; }
  return shift;
}

inline auto image::normalize(uint64_t color, uint sourceDepth, uint targetDepth) -> uint64_t {
  if(sourceDepth == 0 || targetDepth == 0) return 0;
  while(sourceDepth < targetDepth) {
    color = (color << sourceDepth) | color;
    sourceDepth += sourceDepth;
  }
  if(targetDepth < sourceDepth) color >>= (sourceDepth - targetDepth);
  return color;
}

}
