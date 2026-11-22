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
  io.resetPending = 0;
  processInput();
  if(io.mode == Mode::DrainOutput) {
    if(fifo.output.empty() && fifo.pendingOutput.empty()) {
      io.mode = Mode::Idle;
      processInput();
    } else { step(128); return; }
  }
  if(io.mode == Mode::Idle) { step(128); return; }

  if(io.mode == Mode::DecodeMacroblock) {
    bool decoded = decodeMacroblock();
    if(io.resetPending) return;
    if(!decoded || (io.remaining == 0 && !prefetch.ready)) {
      u32 halfBlock = status.outputDepth == 3 ? 16 : 24;
      while(!io.resetPending && fifo.pendingOutput.size() > halfBlock + 1) step(128);
      if(io.resetPending) return;
      status.currentBlock = 4;
      io.mode = Mode::DrainOutput;
    }
  }

  step(128);
}

auto MDEC::step(u32 clocks) -> void {
  prefetchInput();
  notifyDMA();
  u32 halfBlock = status.outputDepth == 3 ? 16 : 24;
  if(io.mode == Mode::DecodeMacroblock && fifo.pendingOutput.size() <= halfBlock + 1)
    status.currentBlock = io.decodeBlock;
  Thread::step(clocks);
  Thread::synchronize();
}

auto MDEC::power(bool reset) -> void {
  Thread::create(system.frequency(), std::bind_front(&MDEC::main, this));
  fifo.input.flush();
  fifo.inputBuffer.flush();
  fifo.output.flush();
  fifo.pendingOutput.flush();
  status = {};
  io.mode = Mode::Idle;
  io.resetPending = 0;
  io.offset = 0;
  io.outputWord = 0;
  io.remaining = 0;
  io.decodeBlock = 4;
  prefetch = {};
  prefetch.block = 4;
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
