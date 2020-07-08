namespace Board {

#include "asc16.cpp"
#include "asc8.cpp"
#include "cross-blaim.cpp"
#include "konami.cpp"
#include "konami-scc.cpp"
#include "linear.cpp"
#include "super-lode-runner.cpp"
#include "super-pierrot.cpp"

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

auto Interface::main() -> void {
  cartridge.step(system.colorburst());
}

}
