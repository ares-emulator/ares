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
  #include "pc-engine-ld.cpp"
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
  emulators.push_back(std::make_shared<Arcade>());

  #ifdef CORE_A26
  emulators.push_back(std::make_shared<Atari2600>());
  #endif

  #ifdef CORE_WS
  emulators.push_back(std::make_shared<WonderSwan>());
  emulators.push_back(std::make_shared<WonderSwanColor>());
  emulators.push_back(std::make_shared<PocketChallengeV2>());
  #endif

  #ifdef CORE_CV
  emulators.push_back(std::make_shared<ColecoVision>());
  #endif

  #ifdef CORE_MYVISION
  emulators.push_back(std::make_shared<MyVision>());
  #endif

  #ifdef CORE_MSX
  emulators.push_back(std::make_shared<MSX>());
  emulators.push_back(std::make_shared<MSX2>());
  #endif

  #ifdef CORE_PCE
  emulators.push_back(std::make_shared<PCEngine>());
  emulators.push_back(std::make_shared<PCEngineCD>());
  emulators.push_back(std::make_shared<PCEngineLD>());
  emulators.push_back(std::make_shared<SuperGrafx>());
  emulators.push_back(std::make_shared<SuperGrafxCD>());
  #endif

  #ifdef CORE_FC
  emulators.push_back(std::make_shared<Famicom>());
  emulators.push_back(std::make_shared<FamicomDiskSystem>());
  #endif

  #ifdef CORE_SFC
  emulators.push_back(std::make_shared<SuperFamicom>());
  #endif

  #ifdef CORE_N64
  emulators.push_back(std::make_shared<Nintendo64>());
  emulators.push_back(std::make_shared<Nintendo64DD>());
  #endif

  #ifdef CORE_GB
  emulators.push_back(std::make_shared<GameBoy>());
  emulators.push_back(std::make_shared<GameBoyColor>());
  #endif

  #ifdef CORE_GBA
  emulators.push_back(std::make_shared<GameBoyAdvance>());
  #endif

  #ifdef CORE_SG
  emulators.push_back(std::make_shared<SG1000>());
  emulators.push_back(std::make_shared<SC3000>());
  #endif

  #ifdef CORE_MS
  emulators.push_back(std::make_shared<MasterSystem>());
  emulators.push_back(std::make_shared<GameGear>());
  #endif

  #ifdef CORE_MD
  emulators.push_back(std::make_shared<MegaDrive>());
  emulators.push_back(std::make_shared<Mega32X>());
  emulators.push_back(std::make_shared<MegaCD>());
  emulators.push_back(std::make_shared<MegaCD32X>());
  emulators.push_back(std::make_shared<MegaLD>());
  #endif

  #ifdef CORE_SATURN
  emulators.push_back(std::make_shared<Saturn>());
  #endif

  #ifdef CORE_NG
  emulators.push_back(std::make_shared<NeoGeoAES>());
  emulators.push_back(std::make_shared<NeoGeoMVS>());
  #endif

  #ifdef CORE_NGP
  emulators.push_back(std::make_shared<NeoGeoPocket>());
  emulators.push_back(std::make_shared<NeoGeoPocketColor>());
  #endif

  #ifdef CORE_PS1
  emulators.push_back(std::make_shared<PlayStation>());
  #endif

  #ifdef CORE_SPEC
  emulators.push_back(std::make_shared<ZXSpectrum>());
  emulators.push_back(std::make_shared<ZXSpectrum128>());
  #endif
}
