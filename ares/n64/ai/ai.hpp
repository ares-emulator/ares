//Audio Interface

struct AI : Thread {
    Node::Object node;
    Node::Audio::Stream stream;
    Debugger debugger;

    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto main() -> void;
    auto sample(f64& left, f64& right) -> void;
    auto step(u32 clocks) -> void;
    auto power(bool reset) -> void;

    // io.cpp
    auto readWord(u32 address, Thread& thread) -> u32;
    auto writeWord(u32 address, u32 data, Thread& thread) -> void;

    // serialization.cpp
    auto serialization(serializer&) -> void;

    struct IO {
        u32 dacRate;
        u32 bitRate;
        u32 dmaEnable;
        u32 dmaCount;
        u32 dmaAddress[2];
        u32 dmaLength[2];
        u32 dmaOriginPc[2];
    } io;

    struct DAC {
        u32 frequency;
        u32 precision;
        u32 period;
    } dac;

    // analog decay registers 
    f64 outputLeft = 0.0;
    f64 outputRight = 0.0;
};

extern AI ai;
