struct FamicomDataRecorder : Tape, Thread {
    Node::Tape node;

    explicit FamicomDataRecorder(Node::Port);
    ~FamicomDataRecorder() override;

    auto read() -> n1 override;
    auto write(n1 data) -> void override;

    auto load() -> bool;
    auto unload() -> void;

    auto main() -> void;
    auto step() -> void;

private:
    VFS::Pak pak;

    n1 output;
    n1 input;
    u64 speed;
    u64 range;

    Memory::Writable<u64> data;
};
