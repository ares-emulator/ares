#include <ares/ares.hpp>
#include "ym2149.hpp"

namespace ares {

#include "serialization.cpp"

auto YM2149::clock() -> array<uint5[3]> {
  toneA.clock();
  toneB.clock();
  toneC.clock();
  noise.clock();
  envelope.clock();

  array<uint5[3]> output;
  if((toneA.output | channelA.tone) & (noise.output | channelA.noise)) {
    output[0] = channelA.envelope ? envelope.output : uint5(channelA.volume << 1 | 1);
  }
  if((toneB.output | channelB.tone) & (noise.output | channelB.noise)) {
    output[1] = channelB.envelope ? envelope.output : uint5(channelB.volume << 1 | 1);
  }
  if((toneC.output | channelC.tone) & (noise.output | channelC.noise)) {
    output[2] = channelC.envelope ? envelope.output : uint5(channelC.volume << 1 | 1);
  }
  return output;
}

auto YM2149::Tone::clock() -> void {
  if(++counter >= period) {
    counter = 0;
    output ^= 1;
  }
}

auto YM2149::Noise::clock() -> void {
  if(++counter >= period) {
    counter = 0;
    if(flip ^= 1) {
      output = !lfsr.bit(0);
      lfsr = (lfsr.bit(0) ^ lfsr.bit(3)) << 16 | lfsr >> 1;
    }
  }
}

auto YM2149::Envelope::clock() -> void {
  if(holding) return;
  counter += 2;
  if(counter >= period) {
    counter = 0;
    if(!attacking) {
      if(output !=  0) return (void)output--;
    } else {
      if(output != 31) return (void)output++;
    }

    if(!repeat) {
      output = 0;
      holding = 1;
    } else if(hold) {
      if(alternate) output ^= 31;
      holding = 1;
    } else if(alternate) {
      attacking ^= 1;
    } else {
      output = !attacking ? 31 : 0;
    }
  }
}

auto YM2149::read() -> uint8 {
  uint8 data;

  switch(io.register) {
  case  0:
    data.bit(0,7) = toneA.period.bit(0, 7);
    break;
  case  1:
    data.bit(0,3) = toneA.period.bit(8,11);
    data.bit(4,7) = toneA.unused.bit(0, 3);
    break;
  case  2:
    data.bit(0,7) = toneB.period.bit(0, 7);
    break;
  case  3:
    data.bit(0,3) = toneB.period.bit(8,11);
    data.bit(4,7) = toneB.unused.bit(0, 3);
    break;
  case  4:
    data.bit(0,7) = toneC.period.bit(0, 7);
    break;
  case  5:
    data.bit(0,3) = toneC.period.bit(8,11);
    data.bit(4,7) = toneC.unused.bit(0, 3);
    break;
  case  6:
    data.bit(0,4) = noise.period;
    data.bit(5,7) = noise.unused;
    break;
  case  7:
    data.bit(0) = channelA.tone;
    data.bit(1) = channelB.tone;
    data.bit(2) = channelC.tone;
    data.bit(3) = channelA.noise;
    data.bit(4) = channelB.noise;
    data.bit(5) = channelC.noise;
    data.bit(6) = portA.direction;
    data.bit(7) = portB.direction;
    break;
  case  8:
    data.bit(0,3) = channelA.volume;
    data.bit(4)   = channelA.envelope;
    data.bit(5,7) = channelA.unused;
    break;
  case  9:
    data.bit(0,3) = channelB.volume;
    data.bit(4)   = channelB.envelope;
    data.bit(5,7) = channelB.unused;
    break;
  case 10:
    data.bit(0,3) = channelC.volume;
    data.bit(4)   = channelC.envelope;
    data.bit(5,7) = channelC.unused;
    break;
  case 11:
    data.bit(0,7) = envelope.period.bit(0, 7);
    break;
  case 12:
    data.bit(0,7) = envelope.period.bit(8,15);
    break;
  case 13:
    data.bit(0)   = envelope.hold;
    data.bit(1)   = envelope.alternate;
    data.bit(2)   = envelope.attack;
    data.bit(3)   = envelope.repeat;
    data.bit(4,7) = envelope.unused;
    break;
  case 14:
    data.bit(0,7) = portA.data;
    break;
  case 15:
    data.bit(0,7) = portB.data;
    break;
  }

  return data;
}

auto YM2149::write(uint8 data) -> void {
  switch(io.register) {
  case  0:
    toneA.period.bit(0, 7) = data.bit(0,7);
    break;
  case  1:
    toneA.period.bit(8,11) = data.bit(0,3);
    toneA.unused.bit(0, 3) = data.bit(4,7);
    break;
  case  2:
    toneB.period.bit(0, 7) = data.bit(0,7);
    break;
  case  3:
    toneB.period.bit(8,11) = data.bit(0,3);
    toneB.unused.bit(0, 3) = data.bit(4,7);
    break;
  case  4:
    toneC.period.bit(0, 7) = data.bit(0,7);
    break;
  case  5:
    toneC.period.bit(8,11) = data.bit(0,3);
    toneC.unused.bit(0, 3) = data.bit(4,7);
    break;
  case  6:
    noise.period = data.bit(0,4);
    noise.unused = data.bit(5,7);
    break;
  case  7:
    channelA.tone   = data.bit(0);
    channelB.tone   = data.bit(1);
    channelC.tone   = data.bit(2);
    channelA.noise  = data.bit(3);
    channelB.noise  = data.bit(4);
    channelC.noise  = data.bit(5);
    portA.direction = data.bit(6);
    portB.direction = data.bit(7);
    break;
  case  8:
    channelA.volume   = data.bit(0,3);
    channelA.envelope = data.bit(4);
    channelA.unused   = data.bit(5,7);
    break;
  case  9:
    channelB.volume   = data.bit(0,3);
    channelB.envelope = data.bit(4);
    channelB.unused   = data.bit(5,7);
    break;
  case 10:
    channelC.volume   = data.bit(0,3);
    channelC.envelope = data.bit(4);
    channelC.unused   = data.bit(5,7);
    break;
  case 11:
    envelope.period.bit(0, 7) = data.bit(0,7);
    break;
  case 12:
    envelope.period.bit(8,15) = data.bit(0,7);
    break;
  case 13:
    envelope.hold      = data.bit(0);
    envelope.alternate = data.bit(1);
    envelope.attack    = data.bit(2);
    envelope.repeat    = data.bit(3);
    envelope.unused    = data.bit(4,7);
    envelope.holding   = 0;
    envelope.attacking = envelope.attack;
    envelope.output    = !envelope.attacking ? 31 : 0;
    break;
  case 14:
    portA.data = data;
    break;
  case 15:
    portB.data = data;
    break;
  }
}

auto YM2149::select(uint4 data) -> void {
  io.register = data;
}

auto YM2149::power() -> void {
  toneA = {};
  toneB = {};
  toneC = {};
  noise = {};
  envelope = {};
  channelA = {};
  channelB = {};
  channelC = {};
  portA = {};
  portB = {};
  io = {};
}

}
