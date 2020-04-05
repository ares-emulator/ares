#include <fc/fc.hpp>

namespace ares::Famicom {

APU apu;
#include "envelope.cpp"
#include "sweep.cpp"
#include "pulse.cpp"
#include "triangle.cpp"
#include "noise.cpp"
#include "dmc.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Component>(parent, from, "APU");
  from = Node::scan(parent = node, from);

  stream = Node::append<Node::Stream>(parent, from, "Stream");
  stream->setChannels(1);
  stream->setFrequency(uint(system.frequency() + 0.5) / rate());
  stream->addHighPassFilter(   90.0, 1);
  stream->addHighPassFilter(  440.0, 1);
  stream->addLowPassFilter (14000.0, 1);

  for(uint amp : range(32)) {
    if(amp == 0) {
      pulseDAC[amp] = 0;
    } else {
      pulseDAC[amp] = 16384.0 * 95.88 / (8128.0 / amp + 100.0);
    }
  }

  for(uint dmc_amp : range(128)) {
    for(uint triangle_amp : range(16)) {
      for(uint noise_amp : range(16)) {
        if(dmc_amp == 0 && triangle_amp == 0 && noise_amp == 0) {
          dmcTriangleNoiseDAC[dmc_amp][triangle_amp][noise_amp] = 0;
        } else {
          dmcTriangleNoiseDAC[dmc_amp][triangle_amp][noise_amp]
          = 16384.0 * 159.79 / (100.0 + 1.0 / (triangle_amp / 8227.0 + noise_amp / 12241.0 + dmc_amp / 22638.0));
        }
      }
    }
  }
}

auto APU::unload() -> void {
  node = {};
  stream = {};
}

auto APU::main() -> void {
  uint pulse_output, triangle_output, noise_output, dmc_output;

  pulse_output  = pulse[0].clock();
  pulse_output += pulse[1].clock();
  triangle_output = triangle.clock();
  noise_output = noise.clock();
  dmc_output = dmc.clock();

  clockFrameCounterDivider();

  int output = 0;
  output += pulseDAC[pulse_output];
  output += dmcTriangleNoiseDAC[dmc_output][triangle_output][noise_output];
  stream->sample(sclamp<16>(output) / 32768.0);

  tick();
}

auto APU::tick() -> void {
  Thread::step(rate());
  Thread::synchronize(cpu);
}

auto APU::setIRQ() -> void {
  cpu.apuLine(frame.irqPending | dmc.irqPending);
}

auto APU::power(bool reset) -> void {
  Thread::create(system.frequency(), {&APU::main, this});

  pulse[0].power();
  pulse[1].power();
  triangle.power();
  noise.power();
  dmc.power();

  frame.irqPending = 0;

  frame.mode = 0;
  frame.counter = 0;
  frame.divider = 1;

  enabledChannels = 0;

  setIRQ();
}

auto APU::readIO(uint16 address) -> uint8 {
  uint8 data = cpu.mdr();

  switch(address) {

  case 0x4015: {
    data.bit(0) = (bool)pulse[0].lengthCounter;
    data.bit(1) = (bool)pulse[1].lengthCounter;
    data.bit(2) = (bool)triangle.lengthCounter;
    data.bit(3) = (bool)noise.lengthCounter;
    data.bit(4) = (bool)dmc.lengthCounter;
    data.bit(5) = 0;
    data.bit(6) = frame.irqPending;
    data.bit(7) = dmc.irqPending;
    frame.irqPending = false;
    setIRQ();
    return data;
  }

  }

  return data;
}

auto APU::writeIO(uint16 address, uint8 data) -> void {
  const uint n = address.bit(2);  //pulse#

  switch(address) {

  case 0x4000: case 0x4004: {
    pulse[n].envelope.speed = data.bit(0,3);
    pulse[n].envelope.useSpeedAsVolume = data.bit(4);
    pulse[n].envelope.loopMode = data.bit(5);
    pulse[n].duty = data.bit(6,7);
    return;
  }

  case 0x4001: case 0x4005: {
    pulse[n].sweep.shift = data.bit(0,2);
    pulse[n].sweep.decrement = data.bit(3);
    pulse[n].sweep.period = data.bit(4,6);
    pulse[n].sweep.enable = data.bit(7);
    pulse[n].sweep.reload = true;
    return;
  }

  case 0x4002: case 0x4006: {
    pulse[n].period.bit(0,7) = data.bit(0,7);
    pulse[n].sweep.pulsePeriod.bit(0,7) = data.bit(0,7);
    return;
  }

  case 0x4003: case 0x4007: {
    pulse[n].period.bit(8,10) = data.bit(0,2);
    pulse[n].sweep.pulsePeriod.bit(8,10) = data.bit(0,2);

    pulse[n].dutyCounter = 0;
    pulse[n].envelope.reloadDecay = true;

    if(enabledChannels.bit(n)) {
      pulse[n].lengthCounter = lengthCounterTable[data.bit(3,7)];
    }
    return;
  }

  case 0x4008: {
    triangle.linearLength = data.bit(0,6);
    triangle.haltLengthCounter = data.bit(7);
    return;
  }

  case 0x400a: {
    triangle.period.bit(0,7) = data.bit(0,7);
    return;
  }

  case 0x400b: {
    triangle.period.bit(8,10) = data.bit(0,2);

    triangle.reloadLinear = true;

    if(enabledChannels.bit(2)) {
      triangle.lengthCounter = lengthCounterTable[data.bit(3,7)];
    }
    return;
  }

  case 0x400c: {
    noise.envelope.speed = data.bit(0,3);
    noise.envelope.useSpeedAsVolume = data.bit(4);
    noise.envelope.loopMode = data.bit(5);
    return;
  }

  case 0x400e: {
    noise.period = data.bit(0,3);
    noise.shortMode = data.bit(7);
    return;
  }

  case 0x400f: {
    noise.envelope.reloadDecay = true;

    if(enabledChannels.bit(3)) {
      noise.lengthCounter = lengthCounterTable[data.bit(3,7)];
    }
    return;
  }

  case 0x4010: {
    dmc.period = data.bit(0,3);
    dmc.loopMode = data.bit(6);
    dmc.irqEnable = data.bit(7);

    dmc.irqPending = dmc.irqPending && dmc.irqEnable && !dmc.loopMode;
    setIRQ();
    return;
  }

  case 0x4011: {
    dmc.dacLatch = data.bit(0,6);
    return;
  }

  case 0x4012: {
    dmc.addressLatch = data;
    return;
  }

  case 0x4013: {
    dmc.lengthLatch = data;
    return;
  }

  case 0x4015: {
    if(data.bit(0) == 0) pulse[0].lengthCounter = 0;
    if(data.bit(1) == 0) pulse[1].lengthCounter = 0;
    if(data.bit(2) == 0) triangle.lengthCounter = 0;
    if(data.bit(3) == 0)    noise.lengthCounter = 0;
    if(data.bit(4) == 0)      dmc.stop();
    if(data.bit(4) == 1)      dmc.start();

    dmc.irqPending = false;

    setIRQ();
    enabledChannels = data.bit(0,4);
    return;
  }

  case 0x4017: {
    frame.mode = data.bit(6,7);

    frame.counter = 0;
    if(frame.mode.bit(1)) {
      clockFrameCounter();
    }
    if(frame.mode.bit(0)) {
      frame.irqPending = false;
      setIRQ();
    }
    frame.divider = FrameCounter::NtscPeriod;
    return;
  }

  }
}

auto APU::clockFrameCounter() -> void {
  frame.counter++;

  if(frame.counter.bit(0)) {
    pulse[0].clockLength();
    pulse[0].sweep.clock(0);
    pulse[1].clockLength();
    pulse[1].sweep.clock(1);
    triangle.clockLength();
    noise.clockLength();
  }

  pulse[0].envelope.clock();
  pulse[1].envelope.clock();
  triangle.clockLinearLength();
  noise.envelope.clock();

  if(frame.counter == 0) {
    if(frame.mode.bit(1)) {
      frame.divider += FrameCounter::NtscPeriod;
    }
    if(frame.mode == 0) {
      frame.irqPending = true;
      setIRQ();
    }
  }
}

auto APU::clockFrameCounterDivider() -> void {
  frame.divider -= 2;
  if(frame.divider <= 0) {
    clockFrameCounter();
    frame.divider += FrameCounter::NtscPeriod;
  }
}

const uint8 APU::lengthCounterTable[32] = {
  0x0a, 0xfe, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06, 0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e,
  0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16, 0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1e,
};

const uint16 APU::noisePeriodTableNTSC[16] = {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

const uint16 APU::noisePeriodTablePAL[16] = {
  4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778,
};

const uint16 APU::dmcPeriodTableNTSC[16] = {
  428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};

const uint16 APU::dmcPeriodTablePAL[16] = {
  398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98, 78, 66, 50,
};

}
