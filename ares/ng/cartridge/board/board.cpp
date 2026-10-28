namespace Board {

#include "rom.cpp"
#include "mslugx.cpp"
#include "cmc.cpp"
#include "sma.cpp"
#include "pcm2.cpp"
#include "pvc.cpp"
#include "k2k2.cpp"
#include "fatfury2.cpp"
#include "kof98.cpp"
#include "jockey-gp.cpp"

auto Interface::load(Memory::Readable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size());
    if(fp->size()) memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Readable<n16>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size() >> 1);
    for(auto address : range(memory.size())) {
      memory.program(address, fp->readm(2L));
    }
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
