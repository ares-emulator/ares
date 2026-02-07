//Audio Interface

struct AI : Thread {
    Node::Object node;
    Node::Audio::Stream stream;

    struct Debugger {
        auto load(Node::Object) -> void;
        auto io(bool mode, u32 address, u32 data) -> void;
        struct Tracer {
            Node::Debugger::Tracer::Notification io;
        } tracer;
    } debugger;

    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto main() -> void;
    auto sample(f64& left, f64& right) -> void;
    auto step(u32 clocks) -> void;
    auto power(bool reset) -> void;

    template<u32 Size> auto read(u32 address, Thread& thread) -> u32;
    template<u32 Size> auto write(u32 address, u32 data, Thread& thread) -> void;

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

    f64 outputLeft = 0.0;
    f64 outputRight = 0.0;

    f64 dac_clock = 0.0;
    bool isDecaying = false;
};

extern AI ai;