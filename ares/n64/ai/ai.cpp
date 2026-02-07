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
            // Step 1 clock at a time for precise decay integration
            step(1);
        }
    }

    auto AI::sample(f64& left, f64& right) -> void {
        // 1. Check if it's time to process a new sample from DMA
        if (dac_clock <= 0) {
            dac_clock += (f64)dac.period; // Cast to f64 for math safety

            if (io.dmaCount > 0 && io.dmaLength[0] > 0) {
                // Read the 32-bit word from RDRAM
                auto data = rdram.ram.read<Word>(io.dmaAddress[0], "AI");

                // Convert s16 to f64 (-1.0 to +1.0)
                outputLeft = (f64)s16(data >> 16) / 32768.0;
                outputRight = (f64)s16(data >> 0) / 32768.0;

                io.dmaAddress[0] += 4;
                io.dmaLength[0] -= 4;

                if (io.dmaLength[0] == 0) {
                    // Shift DMA slots
                    io.dmaAddress[0] = io.dmaAddress[1];
                    io.dmaLength[0] = io.dmaLength[1];
                    io.dmaOriginPc[0] = io.dmaOriginPc[1];

                    if (--io.dmaCount) {
                        // There was another buffer waiting
                    }
                    mi.raise(MI::IRQ::AI);
                }
                isDecaying = false; // We have active audio
            }
            else {
                isDecaying = true;  // Buffer is empty, start the fade-out
            }
        }

        // 2. Apply Decay if we are in decay mode
        if (isDecaying) {
            // Pre-calculated constant (1 tick's worth of decay in an 8ms window)
            static const f64 tickDecay = std::exp(-(1.0 / 93750000.0) / 0.008);
            outputLeft *= tickDecay;
            outputRight *= tickDecay;

            // Prevent denormals (CPU performance killer)
            if (std::abs(outputLeft) < 1e-6) outputLeft = 0.0;
            if (std::abs(outputRight) < 1e-6) outputRight = 0.0;
        }

        // 3. Finalize state
        dac_clock -= 1.0;
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

        // Initialize new state variables
        dac_clock = 0.0;
        isDecaying = false;

        dac.frequency = 44100;
        dac.precision = 16;
        dac.period = system.frequency() / dac.frequency;
    }

}