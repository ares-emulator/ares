#include <n64/n64.hpp>

namespace ares::Nintendo64 {
  Aleck64 aleck64;

  #include "io.cpp"
  #include "controls.cpp"
  #include "debugger.cpp"
  #include "vdp.cpp"
  #include "serialization.cpp"

  #include "game-config/11beat.hpp"
  #include "game-config/hipai.hpp"
  #include "game-config/hipai2.hpp"
  #include "game-config/srmvs.hpp"
  #include "game-config/mtetrisc.hpp"

  auto Aleck64::load(Node::Object parent) -> void {
    sdram.allocate(4_MiB);
    vram.allocate(4_KiB);
    pram.allocate(4_KiB);
    controls.load(parent);
    mahjong.load(parent);
    gameConfig.reset();
    dipSwitchNode = parent->append<Node::Object>("DIP Switches");

    debugger.load(parent);
  }

  auto Aleck64::unload() -> void {
    dips.unload();
    sdram.reset();
    debugger.unload();
  }

  auto Aleck64::save() -> void {

  }

  auto Aleck64::power(bool reset) -> void {
    if(!reset) {
      //NOTE: We can't do this at 'load' time because cartridges are not attached yet...
      auto name = cartridge.pak->attribute("name");

      if(name == "11beat"  ) gameConfig = std::make_shared<_11beat>();
      if(name == "hipai"   ) gameConfig = std::make_shared<hipai>();
      if(name == "hipai2"  ) gameConfig = std::make_shared<hipai2>();
      if(name == "srmvs"   ) gameConfig = std::make_shared<srmvs>();
      if(name == "srmvsa"  ) gameConfig = std::make_shared<srmvs>();
      if(name == "mtetrisc") gameConfig = std::make_shared<mtetrisc>();
      if(!gameConfig) gameConfig = std::make_shared<GameConfig>(); //Fallback to default implementation

      dips.load(dipSwitchNode, cartridge.pak, {"IN0"}, 0, true);

      sdram.fill();
      vram.fill();
      pram.fill();
    }
  }
}
