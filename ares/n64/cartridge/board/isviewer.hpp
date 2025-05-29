struct ISViewer : Memory::PI<ISViewer> {
  Memory::Writable ram;  //unserialized
  Node::Debugger::Tracer::Notification tracer;

  auto enabled() -> bool { return ram.size; }

  //isviewer.cpp
  auto messageChar(char c) -> void;
  auto readHalf(u32 address) -> u16;
  auto writeHalf(u32 address, u16 data) -> void;
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;
} isviewer;
