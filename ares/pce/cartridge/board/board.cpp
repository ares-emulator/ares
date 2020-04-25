namespace Board {

#include "linear.cpp"
#include "split.cpp"
#include "banked.cpp"
#include "ram.cpp"
#include "system-card.cpp"
#include "super-system-card.cpp"
#include "arcade-card-duo.cpp"
#include "arcade-card-pro.cpp"
#include "debugger.cpp"

auto Interface::load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural());
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, name, File::Read, File::Required)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural());
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, name, File::Read)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, name, File::Write)) {
    memory.save(fp);
    return true;
  }
  return false;
}

}
