#include <n64/n64.hpp>

namespace ares::Nintendo64 {

AI ai;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto AI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("AI");

  stream = node->append<Node::Audio::Stream>("AI");
  stream->setChannels(2);
  stream->setFrequency(44100.0);

  debugger.load(node);
}

auto AI::unload() -> void {
  debugger = {};
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto AI::main() -> void {
  while(Thread::clock < 0) {
    f64 left = 0, right = 0;
    sample(left, right);
    stream->frame(left, right);
    step(dac.period);
  }
}

auto AI::sample(f64& left, f64& right) -> void {
    bool active = false;

    // 1. BUFFER MANAGEMENT & DATA READING
    if (io.dmaCount > 0) {
        // A: READ DATA (Only if length > 0 and Enabled)
        if (io.dmaLength[0] > 0 && io.dmaEnable) {
            io.dmaAddress[0].bit(13, 23) += io.dmaAddressCarry;
            auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");

            outputLeft = (f64)((s16)(data >> 16)) / 32768.0;
            outputRight = (f64)((s16)(data >> 0)) / 32768.0;

            io.dmaAddress[0].bit(0, 12) += 4;
            io.dmaAddressCarry = io.dmaAddress[0].bit(0, 12) == 0;
            io.dmaLength[0] -= 4;

            active = true; // We successfully read a sample
        }

        // B: SWAP BUFFER (Must run even if we didn't read data this cycle!)
        if (io.dmaLength[0] == 0) {
            if (--io.dmaCount) {
                io.dmaAddress[0] = io.dmaAddress[1];
                io.dmaLength[0] = io.dmaLength[1];
                io.dmaOriginPc[0] = io.dmaOriginPc[1];
            }
            mi.raise(MI::IRQ::AI);
        }
    }

    // 2. THE RAMP (If we didn't read a sample, decay the old one)
    if (!active) {
        outputLeft *= 0.997;
        outputRight *= 0.997;
    }

    // 3. OUTPUT
    left = outputLeft;
    right = outputRight;
}

auto AI::power(bool reset) -> void {
  Thread::reset();
  fifo[0] = {};
  fifo[1] = {};
  io = {};
  outputLeft = 0.0;
  outputRight = 0.0;
  dac.frequency = 44100;
  dac.precision = 16;
  dac.period = system.frequency() / dac.frequency;
}

}
