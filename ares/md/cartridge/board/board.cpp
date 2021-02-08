namespace Board {

#include "linear.cpp"
#include "banked.cpp"
#include "lock-on.cpp"
#include "game-genie.cpp"

auto Interface::load(Memory::Readable<n16>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural() >> 1);
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge->node, name, File::Read, File::Required)) {
    for(u32 address : range(memory.size())) memory.program(address, fp->readm(2));
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n16>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["mode"].string() != "word") return false;
  memory.allocate(node["size"].natural() >> 1);
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge->node, name, File::Read)) {
    for(u32 address : range(memory.size())) memory.write(address, fp->readm(2));
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["mode"].string() == "word") return false;
  memory.allocate(node["size"].natural());
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge->node, name, File::Read)) {
    for(u32 address : range(memory.size())) memory.write(address, fp->readm(1));
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n16>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["mode"].string() != "word") return false;
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge->node, name, File::Write)) {
    for(u32 address : range(memory.size())) fp->writem(memory[address], 2);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["mode"].string() == "word") return false;
  if(node["volatile"]) return true;
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge->node, name, File::Write)) {
    for(u32 address : range(memory.size())) fp->writem(memory[address], 1);
    return true;
  }
  return false;
}

}
