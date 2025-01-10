#include <n64/n64.hpp>

namespace ares::Nintendo64 {
  Aleck64 aleck64;

  #include "io.cpp"
  #include "controls.cpp"

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

      if(name == "11beat"  ) gameConfig = new _11beat();
      if(name == "starsldr") gameConfig = new starsldr();
      if(name == "doncdoon") gameConfig = new doncdoon();
      if(name == "kurufev" ) gameConfig = new kurufev();
      if(name == "mayjin3" ) gameConfig = new mayjin3();
      if(name == "vivdolls") gameConfig = new vivdolls();
      if(name == "twrshaft") gameConfig = new twrshaft();
      if(name == "hipai"   ) gameConfig = new hipai();
      if(name == "hipai2"  ) gameConfig = new hipai2();
      if(name == "srmvs"   ) gameConfig = new srmvs();
      if(name == "srmvsa"  ) gameConfig = new srmvs();

      if(gameConfig) gameConfig->dipSwitches(dipSwitchNode);

      sdram.fill();
    }
  }
}
