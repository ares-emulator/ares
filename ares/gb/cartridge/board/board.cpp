namespace Board {

#include "none.cpp"
#include "huc1.cpp"
#include "huc3.cpp"
#include "linear.cpp"
#include "mbc1.cpp"
#include "mbc1m.cpp"
#include "mbc2.cpp"
#include "mbc3.cpp"
#include "mbc5.cpp"
#include "mbc6.cpp"
#include "mbc7.cpp"
#include "mmm01.cpp"
#include "tama.cpp"

auto Interface::main() -> void {
  step(cartridge.frequency());
}

auto Interface::step(u32 clocks) -> void {
  cartridge.step(clocks);
}

auto Interface::load(Memory::Readable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size());
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size());
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->write(name)) {
    memory.save(fp);
    return true;
  }
  return false;
}

}
