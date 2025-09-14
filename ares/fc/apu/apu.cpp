#include <fc/fc.hpp>

namespace ares::Famicom {

APU apu;
#include "length.cpp"
#include "envelope.cpp"
#include "sweep.cpp"
#include "pulse.cpp"
#include "triangle.cpp"
#include "noise.cpp"
#include "dmc.cpp"
#include "framecounter.cpp"
#include "serialization.cpp"

auto APU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("APU");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(1);
  stream->setFrequency(u32(system.frequency() + 0.5) / rate());
  stream->addHighPassFilter(   90.0, 1);
  stream->addHighPassFilter(  440.0, 1);
  stream->addLowPassFilter (14000.0, 1);

  for(u32 amp : range(32)) {
    if(amp == 0) {
      pulseDAC[amp] = 0;
    } else {
      pulseDAC[amp] = 32768.0 * 95.88 / (8128.0 / amp + 100.0);
    }
  }

  for(u32 dmcAmp : range(128)) {
    for(u32 triangleAmp : range(16)) {
      for(u32 noiseAmp : range(16)) {
        if(dmcAmp == 0 && triangleAmp == 0 && noiseAmp == 0) {
          dmcTriangleNoiseDAC[dmcAmp][triangleAmp][noiseAmp] = 0;
        } else {
          dmcTriangleNoiseDAC[dmcAmp][triangleAmp][noiseAmp]
          = 32768.0 * 159.79 / (100.0 + 1.0 / (triangleAmp / 8227.0 + noiseAmp / 12241.0 + dmcAmp / 22638.0));
        }
      }
    }
  }
}

auto APU::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto APU::main() -> void {
  u32 pulseOutput = pulse1.clock() + pulse2.clock();
  u32 triangleOutput = triangle.clock();
  u32 noiseOutput = noise.clock();
  u32 dmcOutput = dmc.clock();
  frame.main();

  s32 output = 0;
  output += pulseDAC[pulseOutput];
  output += dmcTriangleNoiseDAC[dmcOutput][triangleOutput][noiseOutput];

  stream->frame(sclamp<16>(output) / 32768.0);

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
  Thread::create(system.frequency(), std::bind_front(&APU::main, this));

  pulse1.power(reset);
  pulse2.power(reset);
  triangle.power(reset);
  noise.power(reset);
  dmc.power(reset);
  frame.power(reset);

  setIRQ();
}

auto APU::readIO(n16 address) -> n8 {
  n8 data = cpu.io.openBus;

  switch(address) {

  case 0x4015: {
    data.bit(0) = (bool)pulse1.length.counter;
    data.bit(1) = (bool)pulse2.length.counter;
    data.bit(2) = (bool)triangle.length.counter;
    data.bit(3) = (bool)noise.length.counter;
    data.bit(4) = (bool)dmc.lengthCounter;
    //bit 5 is open bus
    data.bit(6) = frame.irqPending;
    data.bit(7) = dmc.irqPending;
    frame.irqPending = false;
    setIRQ();
    return data;
  }

  }

  return data;
}

auto APU::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x4000: {
    pulse1.envelope.speed = data.bit(0,3);
    pulse1.envelope.useSpeedAsVolume = data.bit(4);
    pulse1.envelope.loopMode = data.bit(5);
    pulse1.length.setHalt(frame.lengthClocking(), data.bit(5));
    pulse1.duty = data.bit(6,7);
    return;
  }

  case 0x4001: {
    pulse1.sweep.shift = data.bit(0,2);
    pulse1.sweep.decrement = data.bit(3);
    pulse1.sweep.period = data.bit(4,6);
    pulse1.sweep.enable = data.bit(7);
    pulse1.sweep.reload = true;
    return;
  }

  case 0x4002: {
    pulse1.period.bit(0,7) = data.bit(0,7);
    pulse1.sweep.pulsePeriod.bit(0,7) = data.bit(0,7);
    return;
  }

  case 0x4003: {
    pulse1.period.bit(8,10) = data.bit(0,2);
    pulse1.sweep.pulsePeriod.bit(8,10) = data.bit(0,2);

    pulse1.dutyCounter = 0;
    pulse1.envelope.reloadDecay = true;

    pulse1.length.setCounter(frame.lengthClocking(), data.bit(3,7));
    return;
  }

  case 0x4004: {
    pulse2.envelope.speed = data.bit(0,3);
    pulse2.envelope.useSpeedAsVolume = data.bit(4);
    pulse2.envelope.loopMode = data.bit(5);
    pulse2.length.setHalt(frame.lengthClocking(), data.bit(5));
    pulse2.duty = data.bit(6,7);
    return;
  }

  case 0x4005: {
    pulse2.sweep.shift = data.bit(0,2);
    pulse2.sweep.decrement = data.bit(3);
    pulse2.sweep.period = data.bit(4,6);
    pulse2.sweep.enable = data.bit(7);
    pulse2.sweep.reload = true;
    return;
  }

  case 0x4006: {
    pulse2.period.bit(0,7) = data.bit(0,7);
    pulse2.sweep.pulsePeriod.bit(0,7) = data.bit(0,7);
    return;
  }

  case 0x4007: {
    pulse2.period.bit(8,10) = data.bit(0,2);
    pulse2.sweep.pulsePeriod.bit(8,10) = data.bit(0,2);

    pulse2.dutyCounter = 0;
    pulse2.envelope.reloadDecay = true;

    pulse2.length.setCounter(frame.lengthClocking(), data.bit(3,7));
    return;
  }

  case 0x4008: {
    triangle.linearLength = data.bit(0,6);
    triangle.length.setHalt(frame.lengthClocking(), data.bit(7));
    return;
  }

  case 0x400a: {
    triangle.period.bit(0,7) = data.bit(0,7);
    return;
  }

  case 0x400b: {
    triangle.period.bit(8,10) = data.bit(0,2);

    triangle.reloadLinear = true;

    triangle.length.setCounter(frame.lengthClocking(), data.bit(3,7));
    return;
  }

  case 0x400c: {
    noise.envelope.speed = data.bit(0,3);
    noise.envelope.useSpeedAsVolume = data.bit(4);
    noise.length.setHalt(frame.lengthClocking(), data.bit(5));
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

    noise.length.setCounter(frame.lengthClocking(), data.bit(3, 7));
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
    pulse1.length.setEnable(data.bit(0));
    pulse2.length.setEnable(data.bit(1));
    triangle.length.setEnable(data.bit(2));
    noise.length.setEnable(data.bit(3));
    if (data.bit(4) == 0) dmc.stop();
    if (data.bit(4) == 1) dmc.start();

    dmc.irqPending = false;
    setIRQ();
    return;
  }

  case 0x4017: return frame.write(data);
  }
}

auto APU::clockQuarterFrame() -> void {
  pulse1.envelope.clock();
  pulse2.envelope.clock();
  triangle.clockLinearLength();
  noise.envelope.clock();
}

auto APU::clockHalfFrame() -> void {
  pulse1.length.main();
  pulse1.sweep.clock(0);
  pulse2.length.main();
  pulse2.sweep.clock(1);
  triangle.length.main();
  noise.length.main();

  pulse1.envelope.clock();
  pulse2.envelope.clock();
  triangle.clockLinearLength();
  noise.envelope.clock();
}

const n16 APU::noisePeriodTableNTSC[16] = {
  4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068,
};

const n16 APU::noisePeriodTablePAL[16] = {
  4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778,
};

const n16 APU::dmcPeriodTableNTSC[16] = {
  428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54,
};

const n16 APU::dmcPeriodTablePAL[16] = {
  398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98, 78, 66, 50,
};

}
