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
    while (Thread::clock < 0) {
        f64 left = 0, right = 0;
        sample(left, right);
        stream->frame(left, right);
        step(dac.period);
    }
}

auto AI::sample(f64& left, f64& right) -> void {
    bool hasData = false;

    if (io.dmaCount > 0 && io.dmaLength[0] > 0 && io.dmaEnable) {
        auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");
        outputLeft = (f64)s16(data >> 16) / 32768.0;
        outputRight = (f64)s16(data >> 0) / 32768.0;

        io.dmaAddress[0] += 4;
        io.dmaLength[0] -= 4;
        hasData = true;

        if (!io.dmaLength[0]) {
            if (--io.dmaCount) {
                io.dmaAddress[0] = io.dmaAddress[1];
                io.dmaLength[0] = io.dmaLength[1];
                io.dmaOriginPc[0] = io.dmaOriginPc[1];
                mi.raise(MI::IRQ::AI);
            }
        }
    }

    // analog decay only when DMA buffer is empty
    if (!hasData) {
        f64 freq = dac.frequency > 0 ? (f64)dac.frequency : 44100.0;
        f64 k = std::exp(-1.0 / (0.008 * freq));
        outputLeft *= k;
        outputRight *= k;

        // prevent denormals
        if (std::abs(outputLeft) < 1e-6) outputLeft = 0.0;
        if (std::abs(outputRight) < 1e-6) outputRight = 0.0;
    }

    // report persistent state
    left = outputLeft;
    right = outputRight;
}

auto AI::step(u32 clocks) -> void {
    Thread::step(clocks);
    Thread::synchronize();
}

auto AI::power(bool reset) -> void {
    Thread::reset();
    io = {};
    outputLeft = 0.0;
    outputRight = 0.0;
    dac.frequency = 44100;
    dac.precision = 16;
    dac.period = system.frequency() / dac.frequency;
}

}
