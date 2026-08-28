namespace Board {

#include "sara-ram.cpp"
#include "linear.cpp"
#include "activision.cpp"
#include "atari8k.cpp"
#include "atari16k.cpp"
#include "atari32k.cpp"
#include "commavid.cpp"
#include "parker-bros.cpp"
#include "parker-bros-03e0.cpp"
#include "jvp.cpp"
#include "tigervision.cpp"
#include "ua.cpp"
#include "cbs-ram-plus.cpp"
#include "m-network.cpp"
#include "amiga-fc.cpp"
#include "wickstead.cpp"
#include "jane.cpp"
#include "mega-boy.cpp"
#include "atari-32-in-1.cpp"

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
