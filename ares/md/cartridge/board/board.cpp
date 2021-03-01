namespace Board {

#include "linear.cpp"
#include "banked.cpp"
#include "lock-on.cpp"
#include "game-genie.cpp"

auto Interface::load(Memory::Readable<n16>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size() >> 1);
    for(u32 address : range(memory.size())) memory.program(address, fp->readm(2));
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n16>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    if(pak->attribute("mode") != "word") return false;
    memory.allocate(fp->size() >> 1);
    for(u32 address : range(memory.size())) memory.write(address, fp->readm(2));
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    if(pak->attribute("node") == "word") return false;
    memory.allocate(fp->size());
    for(u32 address : range(memory.size())) memory.write(address, fp->readm(1));
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n16>& memory, string name) -> bool {
  if(auto fp = pak->write(name)) {
    if(pak->attribute("mode") != "word") return false;
    for(u32 address : range(memory.size())) fp->writem(memory[address], 2);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->write(name)) {
    if(pak->attribute("mode") == "word") return false;
    for(u32 address : range(memory.size())) fp->writem(memory[address], 1);
    return true;
  }
  return false;
}

}
