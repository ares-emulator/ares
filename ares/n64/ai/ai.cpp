#include <n64/n64.hpp>
#include <cmath>

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

            // Safety: Ensure we always step at least 1 cycle
            step(dac.period > 0 ? dac.period : 2125);
        }
    }

    auto AI::sample(f64& left, f64& right) -> void {
        // 1. DATA PRESENT: Process as normal
        if (io.dmaCount > 0 && io.dmaLength[0] > 0) {
            auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");
            outputLeft = (f64)s16(data >> 16) / 32768.0;
            outputRight = (f64)s16(data >> 0) / 32768.0;

            io.dmaAddress[0] += 4;
            io.dmaLength[0] -= 4;

            if (io.dmaLength[0] == 0) {
                // Shift the DMA registers to the next slot
                io.dmaAddress[0] = io.dmaAddress[1];
                io.dmaLength[0] = io.dmaLength[1];
                io.dmaOriginPc[0] = io.dmaOriginPc[1]; // Restored

                io.dmaCount--;
                mi.raise(MI::IRQ::AI);
            }
        }
        // 2. DATA ABSENT: Apply the 8ms Tau decay at 44.1kHz rate
        else {
            outputLeft *= 0.997;
            outputRight *= 0.997;

            // Floor it at zero once it's quiet enough
            if (std::abs(outputLeft) < 0.0001) outputLeft = 0.0;
            if (std::abs(outputRight) < 0.0001) outputRight = 0.0;
        }

        left = outputLeft;
        right = outputRight;
    }

    auto AI::step(u32 clocks) -> void {
        Thread::step(clocks);
    }

    auto AI::power(bool reset) -> void {
        Thread::reset();
        io = {};
        outputLeft = 0.0;
        outputRight = 0.0;
        dac_clock = 0.0;
        dac.frequency = 44100;
        dac.precision = 16;
        dac.period = system.frequency() / dac.frequency;
    }

}