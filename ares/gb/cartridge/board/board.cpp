namespace Board {

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

auto Interface::load(Memory::Readable<n8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural());
  auto name = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, name, File::Read, File::Required)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, Markup::Node node) -> bool {
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

auto Interface::save(Memory::Writable<n8>& memory, Markup::Node node) -> bool {
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
  step(cartridge.frequency());
}

auto Interface::step(u32 clocks) -> void {
  cartridge.step(clocks);
}

}
