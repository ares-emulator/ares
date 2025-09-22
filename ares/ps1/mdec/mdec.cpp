#include <ps1/ps1.hpp>

namespace ares::PlayStation {

MDEC mdec;
#include "tables.cpp"
#include "decoder.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto MDEC::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("MDEC");
}

auto MDEC::unload() -> void {
  node.reset();
}

  auto MDEC::main() -> void {
  if(io.mode == Mode::Idle) { step(128); return; }

  if (io.mode == Mode::DecodeMacroblock) {
    if(!decodeMacroblock()) {
      io.mode = Mode::Idle;
    }
  }

  step(128);
}

auto MDEC::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize();
}

auto MDEC::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&MDEC::main, this));
  fifo.input.flush();
  fifo.output.flush();
  status = {};
  io.mode = Mode::Idle;
  io.offset = 0;
  for(auto& v : block.luma) v = 0;
  for(auto& v : block.chroma) v = 0;
  for(auto& v : block.scale) v = 0;
  for(auto& v : block.cr) v = 0;
  for(auto& v : block.cb) v = 0;
  for(auto& v : block.y0) v = 0;
  for(auto& v : block.y1) v = 0;
  for(auto& v : block.y2) v = 0;
  for(auto& v : block.y3) v = 0;
}

}
