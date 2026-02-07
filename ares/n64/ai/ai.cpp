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

            step(dac.period);
        }
    }

    auto AI::sample(f64& left, f64& right) -> void {
        if (io.dmaCount > 0 && io.dmaLength[0] > 0) {
            auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");
            outputLeft = (f64)s16(data >> 16) / 32768.0;
            outputRight = (f64)s16(data >> 0) / 32768.0;

            io.dmaAddress[0] += 4;
            io.dmaLength[0] -= 4;

            if (io.dmaLength[0] == 0) {
                io.dmaAddress[0] = io.dmaAddress[1];
                io.dmaLength[0] = io.dmaLength[1];
                io.dmaOriginPc[0] = io.dmaOriginPc[1];
                io.dmaCount--;
                mi.raise(MI::IRQ::AI);
            }
        }
        else {
            //decay instead of snap
            outputLeft *= 0.997;
            outputRight *= 0.997;
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

        // Initialize to safe defaults - will be properly set when game writes AI_DACRATE
        dac.frequency = 44100;
        dac.precision = 16;
        // Calculate initial period based on typical N64 CPU frequency (93.75 MHz)
        // This will be recalculated when AI_DACRATE is written
        // Using system.frequency() / 44100 as the default
        // Typical N64: 93750000 / 44100 ≈ 2125.85
        dac.period = 2125;
    }

}
