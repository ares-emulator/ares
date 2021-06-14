#ifdef CORE_CV
  namespace ares::ColecoVision {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "colecovision.cpp"
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
  }
  #include "game-boy-advance.cpp"
#endif

#ifdef CORE_MD
  namespace ares::MegaDrive {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "mega-drive.cpp"
  #include "mega-32x.cpp"
  #include "mega-cd.cpp"
  #include "mega-cd-32x.cpp"
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
  }
  #include "pc-engine.cpp"
  #include "pc-engine-cd.cpp"
  #include "supergrafx.cpp"
#endif

#ifdef CORE_PS1
  namespace ares::PlayStation {
    auto load(Node::System& node, string name) -> bool;
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
  }
  #include "super-famicom.cpp"
#endif

#ifdef CORE_SG
  namespace ares::SG1000 {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "sg-1000.cpp"
#endif

#ifdef CORE_WS
  namespace ares::WonderSwan {
    auto load(Node::System& node, string name) -> bool;
  }
  #include "wonderswan.cpp"
  #include "wonderswan-color.cpp"
  #include "pocket-challenge-v2.cpp"
#endif

auto Emulator::construct() -> void {
  #ifdef CORE_FC
  emulators.append(new Famicom);
  emulators.append(new FamicomDiskSystem);
  #endif

  #ifdef CORE_SFC
  emulators.append(new SuperFamicom);
  #endif

  #ifdef CORE_N64
  emulators.append(new Nintendo64);
//emulators.append(new Nintendo64DD);
  #endif

  #ifdef CORE_SG
  emulators.append(new SG1000);
  #endif

  #ifdef CORE_MS
  emulators.append(new MasterSystem);
  #endif

  #ifdef CORE_MD
  emulators.append(new MegaDrive);
  emulators.append(new Mega32X);
  emulators.append(new MegaCD);
  emulators.append(new MegaCD32X);
  #endif

  #ifdef CORE_SATURN
  emulators.append(new Saturn);
  #endif

  #ifdef CORE_PS1
  emulators.append(new PlayStation);
  #endif

  #ifdef CORE_PCE
  emulators.append(new PCEngine);
  emulators.append(new PCEngineCD);
  emulators.append(new SuperGrafx);
  #endif

  #ifdef CORE_NG
  emulators.append(new NeoGeoAES);
  emulators.append(new NeoGeoMVS);
  #endif

  #ifdef CORE_MSX
  emulators.append(new MSX);
  emulators.append(new MSX2);
  #endif

  #ifdef CORE_CV
  emulators.append(new ColecoVision);
  #endif

  #ifdef CORE_GB
  emulators.append(new GameBoy);
  emulators.append(new GameBoyColor);
  #endif

  #ifdef CORE_GBA
  emulators.append(new GameBoyAdvance);
  #endif

  #ifdef CORE_MS
  emulators.append(new GameGear);
  #endif

  #ifdef CORE_WS
  emulators.append(new WonderSwan);
  emulators.append(new WonderSwanColor);
  emulators.append(new PocketChallengeV2);
  #endif

  #ifdef CORE_NGP
  emulators.append(new NeoGeoPocket);
  emulators.append(new NeoGeoPocketColor);
  #endif
}
