#ifdef CORE_A26
namespace ares::Atari2600 {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
}
#include "atari-2600.cpp"
#endif

#ifdef CORE_CV
  namespace ares::ColecoVision {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "colecovision.cpp"
#endif

#ifdef CORE_MYVISION
  namespace ares::MyVision {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "myvision.cpp"
#endif

#ifdef CORE_FC
  namespace ares::Famicom {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "famicom.cpp"
  #include "famicom-disk-system.cpp"
#endif

#ifdef CORE_GB
  namespace ares::GameBoy {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "game-boy.cpp"
  #include "game-boy-color.cpp"
#endif

#ifdef CORE_GBA
  namespace ares::GameBoyAdvance {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "game-boy-advance.cpp"
#endif

#ifdef CORE_MD
  namespace ares::MegaDrive {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "mega-drive.cpp"
  #include "mega-32x.cpp"
  #include "mega-cd.cpp"
  #include "mega-cd-32x.cpp"
  #include "mega-ld.cpp"
#endif

#ifdef CORE_MS
  namespace ares::MasterSystem {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "master-system.cpp"
  #include "game-gear.cpp"
#endif

#ifdef CORE_MSX
  namespace ares::MSX {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "msx.cpp"
  #include "msx2.cpp"
#endif

#ifdef CORE_N64
  namespace ares::Nintendo64 {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "nintendo-64.cpp"
  #include "nintendo-64dd.cpp"
#endif

#ifdef CORE_NG
  namespace ares::NeoGeo {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "neo-geo-aes.cpp"
  #include "neo-geo-mvs.cpp"
#endif

#ifdef CORE_NGP
  namespace ares::NeoGeoPocket {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "neo-geo-pocket.cpp"
  #include "neo-geo-pocket-color.cpp"
#endif

#ifdef CORE_PCE
  namespace ares::PCEngine {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "pc-engine.cpp"
  #include "pc-engine-cd.cpp"
  #include "supergrafx.cpp"
  #include "supergrafx-cd.cpp"
#endif

#ifdef CORE_PS1
  namespace ares::PlayStation {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "playstation.cpp"
#endif

#ifdef CORE_SATURN
  namespace ares::Saturn {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "saturn.cpp"
#endif

#ifdef CORE_SFC
  namespace ares::SuperFamicom {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "super-famicom.cpp"
#endif

#ifdef CORE_SG
  namespace ares::SG1000 {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "sg-1000.cpp"
  #include "sc-3000.cpp"
#endif

#ifdef CORE_WS
  namespace ares::WonderSwan {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "wonderswan.cpp"
  #include "wonderswan-color.cpp"
  #include "pocket-challenge-v2.cpp"
#endif

#ifdef CORE_SPEC
namespace ares::ZXSpectrum {
    auto load(Node::System& node, string name) -> bool;
    auto option(string name, string value) -> bool;
  }
  #include "zx-spectrum.cpp"
  #include "zx-spectrum-128.cpp"
#endif

#include "arcade.cpp"

auto Emulator::construct() -> void {
  emulators.push_back(new Arcade);

  #ifdef CORE_A26
  emulators.push_back(new Atari2600);
  #endif

  #ifdef CORE_WS
  emulators.push_back(new WonderSwan);
  emulators.push_back(new WonderSwanColor);
  emulators.push_back(new PocketChallengeV2);
  #endif

  #ifdef CORE_CV
  emulators.push_back(new ColecoVision);
  #endif

  #ifdef CORE_MYVISION
  emulators.push_back(new MyVision);
  #endif

  #ifdef CORE_MSX
  emulators.push_back(new MSX);
  emulators.push_back(new MSX2);
  #endif

  #ifdef CORE_PCE
  emulators.push_back(new PCEngine);
  emulators.push_back(new PCEngineCD);
  emulators.push_back(new SuperGrafx);
  emulators.push_back(new SuperGrafxCD);
  #endif

  #ifdef CORE_FC
  emulators.push_back(new Famicom);
  emulators.push_back(new FamicomDiskSystem);
  #endif

  #ifdef CORE_SFC
  emulators.push_back(new SuperFamicom);
  #endif

  #ifdef CORE_N64
  emulators.push_back(new Nintendo64);
  emulators.push_back(new Nintendo64DD);
  #endif

  #ifdef CORE_GB
  emulators.push_back(new GameBoy);
  emulators.push_back(new GameBoyColor);
  #endif

  #ifdef CORE_GBA
  emulators.push_back(new GameBoyAdvance);
  #endif

  #ifdef CORE_SG
  emulators.push_back(new SG1000);
  emulators.push_back(new SC3000);
  #endif

  #ifdef CORE_MS
  emulators.push_back(new MasterSystem);
  emulators.push_back(new GameGear);
  #endif

  #ifdef CORE_MD
  emulators.push_back(new MegaDrive);
  emulators.push_back(new Mega32X);
  emulators.push_back(new MegaCD);
  emulators.push_back(new MegaCD32X);
  emulators.push_back(new MegaLD);
  #endif

  #ifdef CORE_SATURN
  emulators.push_back(new Saturn);
  #endif

  #ifdef CORE_NG
  emulators.push_back(new NeoGeoAES);
  emulators.push_back(new NeoGeoMVS);
  #endif

  #ifdef CORE_NGP
  emulators.push_back(new NeoGeoPocket);
  emulators.push_back(new NeoGeoPocketColor);
  #endif

  #ifdef CORE_PS1
  emulators.push_back(new PlayStation);
  #endif

  #ifdef CORE_SPEC
  emulators.push_back(new ZXSpectrum);
  emulators.push_back(new ZXSpectrum128);
  #endif
}
