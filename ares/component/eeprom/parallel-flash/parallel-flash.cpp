#include <ares/ares.hpp>
#include "parallel-flash.hpp"

namespace ares {

ParallelFlash::ParallelFlash(Model model) {
  switch(model) {
  case Model::SST29EE010: capacity = 128_KiB; pageSize = 128; manufacturerID = 0xbf; deviceID = 0x07; break;
  case Model::SST29EE020: capacity = 256_KiB; pageSize = 128; manufacturerID = 0xbf; deviceID = 0x10; break;
  case Model::W29C040:   capacity = 512_KiB; pageSize = 256; manufacturerID = 0xda; deviceID = 0x46; break;
  case Model::AT29C040A: capacity = 512_KiB; pageSize = 256; manufacturerID = 0x1f; deviceID = 0xa4; break;
  }
}

auto ParallelFlash::power() -> void {
  phase = Phase::Read;
  command = 0;
  remaining = 0;
  id = false;
  toggle = false;
}

auto ParallelFlash::read(u32 address) -> u8 {
  if(memory.size() != capacity) return 0xff;
  address &= capacity - 1;
  if(phase != Phase::Read) {
    toggle = !toggle;
    return (lastData ^ 0x80) & 0x80 | (toggle ? 0x40 : 0);
  }
  if(id) {
    if((address & 0x7fff) == 0) return manufacturerID;
    if((address & 0x7fff) == 1) return deviceID;
    return 0xff;
  }
  return memory[address];
}

auto ParallelFlash::write(u32 address, u8 data) -> void {
  if(memory.size() != capacity || phase == Phase::Program || phase == Phase::Erase) return;
  address &= capacity - 1;
  auto load = [&] {
    if(phase != Phase::Load) nall::memory::fill<u8>(page, pageSize, 0xff);
    phase = Phase::Load;
    pageAddress = address & ~(pageSize - 1);
    page[address & (pageSize - 1)] = lastData = data;
    remaining = 200;
    command = 0;
  };
  if(phase == Phase::Load || command == 3) return load();
  u32 low = address & 0x7fff;
  switch(command) {
  case 0:
    if(low == 0x5555 && data == 0xaa) command = 1;
    else if(!protection && !id) load();
    break;
  case 1:
    command = low == 0x2aaa && data == 0x55 ? 2 : 0;
    break;
  case 2:
    command = 0;
    if(low != 0x5555) break;
    if(data == 0xa0) {
      protection = true;
      id = false;
      command = 3;
    }
    if(data == 0x90) id = true;
    if(data == 0xf0) id = false;
    if(data == 0x80) command = 4;
    break;
  case 4:
    command = low == 0x5555 && data == 0xaa ? 5 : 0;
    break;
  case 5:
    command = low == 0x2aaa && data == 0x55 ? 6 : 0;
    break;
  case 6:
    command = 0;
    if(low != 0x5555) break;
    if(data == 0x10) {
      phase = Phase::Erase;
      remaining = 20'000;
      lastData = 0xff;
      id = false;
    }
    if(data == 0x20) {
      protection = false;
      id = false;
    }
    if(data == 0x60) id = true;
    if(data == 0xf0) id = false;
    break;
  }
}

auto ParallelFlash::step(u32 clocks) -> void {
  while(remaining && clocks >= remaining) {
    clocks -= remaining;
    if(phase == Phase::Load) {
      phase = Phase::Program;
      remaining = 5'000;
    } else {
      if(phase == Phase::Program) nall::memory::copy(memory.data() + pageAddress, page, pageSize);
      if(phase == Phase::Erase) nall::memory::fill<u8>(memory.data(), memory.size(), 0xff);
      phase = Phase::Read;
      remaining = 0;
    }
  }
  if(remaining) remaining -= clocks;
}

auto ParallelFlash::serialize(serializer& s) -> void {
  if(!memory.empty()) s(std::span<u8>{memory.data(), memory.size()});
  s(page);
  s(phase);
  s(command);
  s(pageAddress);
  s(remaining);
  s(lastData);
  s(id);
  s(protection);
  s(toggle);
}

}
