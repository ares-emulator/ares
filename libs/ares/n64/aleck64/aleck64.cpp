#include <n64/n64.hpp>

namespace ares::Nintendo64 {
  Aleck64 aleck64;

  #include "io.cpp"
  #include "controls.cpp"
  #include "debugger.cpp"
  #include "vdp.cpp"
  #include "serialization.cpp"

  #include "game-config/11beat.hpp"
  #include "game-config/starsldr.hpp"
  #include "game-config/doncdoon.hpp"
  #include "game-config/kurufev.hpp"
  #include "game-config/mayjin3.hpp"
  #include "game-config/vivdolls.hpp"
  #include "game-config/twrshaft.hpp"
  #include "game-config/hipai.hpp"
  #include "game-config/hipai2.hpp"
  #include "game-config/srmvs.hpp"
  #include "game-config/mtetrisc.hpp"

  auto Aleck64::load(Node::Object parent) -> void {
    sdram.allocate(4_MiB);
    vram.allocate(4_KiB);
    pram.allocate(4_KiB);
    controls.load(parent);
    gameConfig.reset();
    dipSwitchNode = parent->append<Node::Object>("DIP Switches");

    debugger.load(parent);
  }

  auto Aleck64::unload() -> void {
    sdram.reset();
    debugger.unload();
  }

  auto Aleck64::save() -> void {

  }

  auto Aleck64::power(bool reset) -> void {
    if(!reset) {
      dipSwitch[0] = 0xffff'ffff;
      dipSwitch[1] = 0xffff'ffff;

      //NOTE: We can't do this at 'load' time because cartridges are not attached yet...
      auto name = cartridge.pak->attribute("name");

      if(name == "11beat"  ) gameConfig = std::make_shared<_11beat>();
      if(name == "starsldr") gameConfig = std::make_shared<starsldr>();
      if(name == "doncdoon") gameConfig = std::make_shared<doncdoon>();
      if(name == "kurufev" ) gameConfig = std::make_shared<kurufev>();
      if(name == "mayjin3" ) gameConfig = std::make_shared<mayjin3>();
      if(name == "vivdolls") gameConfig = std::make_shared<vivdolls>();
      if(name == "twrshaft") gameConfig = std::make_shared<twrshaft>();
      if(name == "hipai"   ) gameConfig = std::make_shared<hipai>();
      if(name == "hipai2"  ) gameConfig = std::make_shared<hipai2>();
      if(name == "srmvs"   ) gameConfig = std::make_shared<srmvs>();
      if(name == "srmvsa"  ) gameConfig = std::make_shared<srmvs>();
      if(name == "mtetrisc") gameConfig = std::make_shared<mtetrisc>();
      if(!gameConfig) gameConfig = std::make_shared<GameConfig>(); //Fallback to default implementation

      gameConfig->dipSwitches(dipSwitchNode);

      sdram.fill();
      vram.fill();
      pram.fill();
    }
  }
}
