#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Virage virage0(0, 0x40);
Virage virage1(1, 0x40);
Virage virage2(2, 0x100);
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Virage::load(Node::Object parent) -> void {
  string name = {"Virage", num};
  node = parent->append<Node::Object>(name);

  sram.allocate(memSize);
  flash.allocate(memSize);

  debugger.load(node);
}

auto Virage::unload() -> void {
  debugger.unload(node);
  sram.reset();
  flash.reset();
  node.reset();
}

auto Virage::save() -> void {
  if(!node) return;

  string fileName = { "virage", num, ".flash" };
  if(auto fp = system.pak->write(fileName))
    flash.save(fp);
}

auto Virage::power(bool reset) -> void {
  string fileName = { "virage", num, ".flash" };
  if(auto fp = system.pak->write(fileName))
    flash.load(fp);

  printf("virage power\n");

  io.unk30 = 1;
  io.busy = 0;

  //Load flash -> sram
  command(0x03000000);
}

}
