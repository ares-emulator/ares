#include <ares/ares.hpp>
#include "msm5205.hpp"

namespace ares {

#include "serialization.cpp"

auto MSM5205::sample() const -> int12 {
  //note: technically this should return io.sample & ~3;
  //the DAC output is only 10-bits
  return io.sample;
}

auto MSM5205::scaler() const -> uint {
  //0 = 4khz (384khz / 96);  8khz (768khz / 96)
  //1 = 6khz (384khz / 64); 12khz (768khz / 64)
  //2 = 8khz (384khz / 48); 16khz (768khz / 48)
  //3 = prohibited (unknown behavior: treat as 8khz)
  static const uint scaler[4] = {96, 64, 48, 48};
  return scaler[io.scaler];
}

auto MSM5205::setReset(uint1 reset) -> void {
  io.reset = reset;
}

auto MSM5205::setWidth(uint1 width) -> void {
  io.width = width;
}

auto MSM5205::setScaler(uint2 scaler) -> void {
  io.scaler = scaler;
}

auto MSM5205::setData(uint4 data) -> void {
  if(io.width == 0) io.data = (uint3)data << 1;
  if(io.width == 1) io.data = (uint4)data << 0;
}

auto MSM5205::clock() -> void {
  if(io.reset) {
    io.sample = 0;
    io.step = 0;
    return;
  }

  int sample = io.sample + lookup[io.step * 16 + io.data];
  if(sample >  2047) sample =  2047;
  if(sample < -2048) sample = -2048;
  io.sample = sample;

  static const int shift[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
  io.step += shift[(uint3)io.data];
  if(io.step > 48) io.step = 48;
  if(io.step <  0) io.step =  0;
}

auto MSM5205::power() -> void {
  io = {};

  static const int map[16][4] = {
    { 1,0,0,0}, { 1,0,0,1}, { 1,0,1,0}, { 1,0,1,1},
    { 1,1,0,0}, { 1,1,0,1}, { 1,1,1,0}, { 1,1,1,1},
    {-1,0,0,0}, {-1,0,0,1}, {-1,0,1,0}, {-1,0,1,1},
    {-1,1,0,0}, {-1,1,0,1}, {-1,1,1,0}, {-1,1,1,1},
  };

  for(int step : range(49)) {
    int scale = floor(16.0 * pow(11.0 / 10.0, (double)step));
    for(int nibble : range(16)) {
      int a = map[nibble][0];
      int b = map[nibble][1] * scale / 1;
      int c = map[nibble][2] * scale / 2;
      int d = map[nibble][3] * scale / 4;
      int e = scale / 8;
      lookup[step * 16 + nibble] = a * (b + c + d + e);
    }
  }
}

}
