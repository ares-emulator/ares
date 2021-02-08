#include <ares/ares.hpp>
#include "msm5205.hpp"

namespace ares {

#include "serialization.cpp"

auto MSM5205::sample() const -> i12 {
  //note: technically this should return io.sample & ~3;
  //the DAC output is only 10-bits
  return io.sample;
}

auto MSM5205::scaler() const -> u32 {
  //0 = 4khz (384khz / 96);  8khz (768khz / 96)
  //1 = 6khz (384khz / 64); 12khz (768khz / 64)
  //2 = 8khz (384khz / 48); 16khz (768khz / 48)
  //3 = prohibited (unknown behavior: treat as 8khz)
  static const u32 scaler[4] = {96, 64, 48, 48};
  return scaler[io.scaler];
}

auto MSM5205::setReset(n1 reset) -> void {
  io.reset = reset;
}

auto MSM5205::setWidth(n1 width) -> void {
  io.width = width;
}

auto MSM5205::setScaler(n2 scaler) -> void {
  io.scaler = scaler;
}

auto MSM5205::setData(n4 data) -> void {
  if(io.width == 0) io.data = (n3)data << 1;
  if(io.width == 1) io.data = (n4)data << 0;
}

auto MSM5205::clock() -> void {
  if(io.reset) {
    io.sample = 0;
    io.step = 0;
    return;
  }

  s32 sample = io.sample + lookup[io.step * 16 + io.data];
  if(sample >  2047) sample =  2047;
  if(sample < -2048) sample = -2048;
  io.sample = sample;

  static const s32 shift[8] = {-1, -1, -1, -1, 2, 4, 6, 8};
  io.step += shift[(n3)io.data];
  if(io.step > 48) io.step = 48;
  if(io.step <  0) io.step =  0;
}

auto MSM5205::power() -> void {
  io = {};

  static const s32 map[16][4] = {
    { 1,0,0,0}, { 1,0,0,1}, { 1,0,1,0}, { 1,0,1,1},
    { 1,1,0,0}, { 1,1,0,1}, { 1,1,1,0}, { 1,1,1,1},
    {-1,0,0,0}, {-1,0,0,1}, {-1,0,1,0}, {-1,0,1,1},
    {-1,1,0,0}, {-1,1,0,1}, {-1,1,1,0}, {-1,1,1,1},
  };

  for(s32 step : range(49)) {
    s32 scale = floor(16.0 * pow(11.0 / 10.0, (f64)step));
    for(s32 nibble : range(16)) {
      s32 a = map[nibble][0];
      s32 b = map[nibble][1] * scale / 1;
      s32 c = map[nibble][2] * scale / 2;
      s32 d = map[nibble][3] * scale / 4;
      s32 e = scale / 8;
      lookup[step * 16 + nibble] = a * (b + c + d + e);
    }
  }
}

}
