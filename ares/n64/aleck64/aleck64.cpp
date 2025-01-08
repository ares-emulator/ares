#include <n64/n64.hpp>

namespace ares::Nintendo64 {
  Aleck64 aleck64;

  #include "io.cpp"
  #include "controls.cpp"
  #include "game-config/starsldr.hpp"

  auto Aleck64::load(Node::Object parent) -> void {
    sdram.allocate(4_MiB);
    controls.load(parent);
    gameConfig.reset();
    dipSwitchNode = parent->append<Node::Object>("DIP Switches");
  }

  auto Aleck64::unload() -> void {
    sdram.reset();
  }

  auto Aleck64::save() -> void {

  }

  auto Aleck64::power(bool reset) -> void {
    if(!reset) {
      dipSwitch[0] = 0xffff'ffff;
      dipSwitch[1] = 0xffff'ffff;

      //NOTE: We can't do this at 'load' time because cartridges are not attached yet...
      auto name = cartridge.pak->attribute("name");
      if(name == "starsldr") gameConfig = new starsldr();
      gameConfig->dipSwitches(dipSwitchNode);

      sdram.fill();
    }
  }
}
