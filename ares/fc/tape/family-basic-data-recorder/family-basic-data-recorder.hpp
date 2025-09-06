struct FamilyBasicDataRecorder : Tape, Thread {
  Node::Tape node;

  FamilyBasicDataRecorder(Node::Port parent);
  ~FamilyBasicDataRecorder() override;

  // family-basic-data-recorder.cpp
  auto read() -> n1 override;
  auto write(n3 data) -> void override;

  auto load() -> bool;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

private:
  const string name;

  VFS::Pak pak;

  bool enable;
  u1 output;
  u1 input;
  u64 speed;
  u64 range;

  Memory::Writable<u64> data;
};