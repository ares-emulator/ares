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

  while(io.mode == Mode::DecodeMacroblock) {
    if(io.phaseClocks) {
      u32 clocks = io.phaseClocks;
      io.phaseClocks = 0;
      return step(clocks);
    }

    DecodePhase phase = io.decodePhase;
    advanceDecode();
    if(io.mode != Mode::DecodeMacroblock) break;

    bool decodedBlock = phase >= DecodeCr && phase <= DecodeY3 && io.decodePhase != phase;
    bool convertedBlock = phase == Convert && io.decodePhase == Publish;
    if(decodedBlock || convertedBlock) continue;
    break;
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
  io.decodePhase = DecodeIdle;
  io.blockPhase = BlockStart;
  io.offset = 0;
  io.outputOffset = 0;
  io.outputBlock = 0;
  io.outputWriteOffset = 0;
  io.coefficient = 0;
  io.qfactor = 0;
  io.phaseClocks = 0;
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
