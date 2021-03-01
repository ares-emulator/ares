namespace Board {

#include "asc16.cpp"
#include "asc8.cpp"
#include "cross-blaim.cpp"
#include "konami.cpp"
#include "konami-scc.cpp"
#include "linear.cpp"
#include "super-lode-runner.cpp"
#include "super-pierrot.cpp"

auto Interface::main() -> void {
  cartridge.step(system.colorburst());
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
