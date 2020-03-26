#include <ares/ares.hpp>
#include "t6w28.hpp"

namespace ares {

#include "serialization.cpp"

auto T6W28::clock() -> array<uint4[8]> {
  tone0.clock();
  tone1.clock();
  tone2.clock();
  noise.clock();

  array<uint4[8]> output{15, 15, 15, 15, 15, 15, 15, 15};
  if(tone0.output) output[0] = tone0.volume.left, output[4] = tone0.volume.right;
  if(tone1.output) output[1] = tone1.volume.left, output[5] = tone1.volume.right;
  if(tone2.output) output[2] = tone2.volume.left, output[6] = tone2.volume.right;
  if(noise.output) output[3] = noise.volume.left, output[7] = noise.volume.right;
  return output;
}

auto T6W28::Tone::clock() -> void {
  if(!counter--) {
    counter = pitch;
    output ^= 1;
  }
}

auto T6W28::Noise::clock() -> void {
  if(!counter--) {
    counter = array<uint10[4]>{0x10, 0x20, 0x40, pitch}[rate];
    if(flip ^= 1) {
      output = !lfsr.bit(0);
      lfsr = (lfsr.bit(0) ^ lfsr.bit(2) & enable) << 14 | lfsr >> 1;
    }
  }
}

auto T6W28::writeLeft(uint8 data) -> void {
  if(data.bit(7)) io.register = data.bit(4,6);

  if(io.register.bit(0)) switch(io.register.bit(1,2)) {
  case 0: tone0.volume.left = data.bit(0,3); break;
  case 1: tone1.volume.left = data.bit(0,3); break;
  case 2: tone2.volume.left = data.bit(0,3); break;
  case 3: noise.volume.left = data.bit(0,3); break;
  }

  else if(data.bit(7)) switch(io.register.bit(1,2)) {
  case 0: tone0.pitch.bit(0,3) = data.bit(0,3); break;
  case 1: tone1.pitch.bit(0,3) = data.bit(0,3); break;
  case 2: tone2.pitch.bit(0,3) = data.bit(0,3); break;
  }

  else switch(io.register.bit(1,2)) {
  case 0: tone0.pitch.bit(4,9) = data.bit(0,5); break;
  case 1: tone1.pitch.bit(4,9) = data.bit(0,5); break;
  case 2: tone2.pitch.bit(4,9) = data.bit(0,5); break;
  }
}

auto T6W28::writeRight(uint8 data) -> void {
  if(data.bit(7)) io.register = data.bit(4,6);

  if(io.register.bit(0)) switch(io.register.bit(1,2)) {
  case 0: tone0.volume.right = data.bit(0,3); break;
  case 1: tone1.volume.right = data.bit(0,3); break;
  case 2: tone2.volume.right = data.bit(0,3); break;
  case 3: noise.volume.right = data.bit(0,3); break;
  }

  else if(data.bit(7)) switch(io.register.bit(1,2)) {
  case 2: noise.pitch.bit(0,3) = data.bit(0,3); break;
  case 3: noise.rate   = data.bit(0,1);
          noise.enable = data.bit(2);
          noise.lfsr   = 0x4000;
          break;
  }

  else switch(io.register.bit(1,2)) {
  case 2: noise.pitch.bit(4,9) = data.bit(0,5); break;
  }
}

auto T6W28::power() -> void {
  tone0 = {};
  tone1 = {};
  tone2 = {};
  noise = {};
  io = {};
}

}
