#include "emulators.hpp"

using namespace nall;

#ifdef CORE_A26
namespace ares::Atari2600 { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_CV
namespace ares::ColecoVision { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_FC
namespace ares::Famicom { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_GB
namespace ares::GameBoy { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_GBA
namespace ares::GameBoyAdvance { auto load(Node::System& node, string name) -> bool; }
namespace ares::GameBoyAdvance { auto option(string name, string value) -> bool; }
#endif
#ifdef CORE_MD
namespace ares::MegaDrive { auto load(Node::System& node, string name) -> bool; }
namespace ares::MegaDrive { auto option(string name, string value) -> bool; }
#endif
#ifdef CORE_MS
namespace ares::MasterSystem { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_MSX
namespace ares::MSX { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_MYVISION
namespace ares::MyVision { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_N64
namespace ares::Nintendo64 {
auto load(Node::System& node, string name) -> bool;
auto option(string name, string value) -> bool;
}
#endif
#ifdef CORE_NG
namespace ares::NeoGeo { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_NGP
namespace ares::NeoGeoPocket { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_PCE
namespace ares::PCEngine { auto load(Node::System& node, string name) -> bool; }
namespace ares::PCEngine { auto option(string name, string value) -> bool; }
#endif
#ifdef CORE_PS1
namespace ares::PlayStation { auto load(Node::System& node, string name) -> bool; }
namespace ares::PlayStation { auto option(string name, string value) -> bool; }
#endif
#ifdef CORE_SATURN
namespace ares::Saturn { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_SFC
namespace ares::SuperFamicom { auto load(Node::System& node, string name) -> bool; }
namespace ares::SuperFamicom { auto option(string name, string value) -> bool; }
#endif
#ifdef CORE_SG
namespace ares::SG1000 { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_SPEC
namespace ares::ZXSpectrum { auto load(Node::System& node, string name) -> bool; }
#endif
#ifdef CORE_WS
namespace ares::WonderSwan { auto load(Node::System& node, string name) -> bool; }
namespace ares::WonderSwan { auto option(string name, string value) -> bool; }
#endif

namespace {

auto tryConnectPort(ares::Node::System& root, const string& name, maybe<string> device = {}) -> void {
  if(auto port = root->find<ares::Node::Port>(name)) {
    if(device) port->allocate(*device);
    else port->allocate();
    port->connect();
  }
}

auto isFamicomFamily(const string& medium) -> bool {
  return medium == "Famicom" || medium == "Famicom Disk System";
}

auto isGameBoyFamily(const string& medium) -> bool {
  return medium == "Game Boy" || medium == "Game Boy Color";
}

auto isMasterSystemFamily(const string& medium) -> bool {
  return medium == "Master System" || medium == "Game Gear";
}

auto isMegaDriveFamily(const string& medium) -> bool {
  return medium == "Mega Drive" || medium == "Mega 32X" || medium == "Mega CD" || medium == "Mega LD" || medium == "Mega CD 32X";
}

auto isMSXFamily(const string& medium) -> bool {
  return medium == "MSX" || medium == "MSX2";
}

auto isNintendo64Family(const string& medium) -> bool {
  return medium == "Nintendo 64" || medium == "Nintendo 64DD";
}

auto isPCEFamily(const string& medium) -> bool {
  return medium == "PC Engine" || medium == "PC Engine CD" || medium == "PC Engine LD" || medium == "SuperGrafx";
}

auto isWonderSwanFamily(const string& medium) -> bool {
  return medium == "Pocket Challenge V2" || medium == "WonderSwan" || medium == "WonderSwan Color";
}

auto isNeoGeoPocketFamily(const string& medium) -> bool {
  return medium == "Neo Geo Pocket" || medium == "Neo Geo Pocket Color";
}

auto isSG1000Family(const string& medium) -> bool {
  return medium == "SG-1000" || medium == "SC-3000";
}

auto isZXSpectrumFamily(const string& medium) -> bool {
  return medium == "ZX Spectrum" || medium == "ZX Spectrum 128";
}

}

namespace headless {

auto normalizeRegion(const string& region) -> string {
  if(!region) return {};
  auto regions = split_and_strip(region, ",");
  if(regions.empty()) return string{region}.strip();
  return regions[0];
}

auto defaultSystemNameForMedium(const string& medium) -> string {
  if(isGameBoyFamily(medium)) return "Game Boy";
  if(isFamicomFamily(medium)) return "Famicom";
  if(isPCEFamily(medium)) return "PC Engine";
  if(isWonderSwanFamily(medium)) return "WonderSwan";
  if(isZXSpectrumFamily(medium)) return "ZX Spectrum";
  return medium;
}

auto defaultProfileForMedium(const string& medium, const string& region, bool gameBoyAdvancePlayer) -> string {
  auto effectiveRegion = region ? region : string{"NTSC-U"};
  if(medium == "Atari 2600") return string{"[Atari] Atari 2600 (", effectiveRegion, ")"};
  if(medium == "ColecoVision") return string{"[Coleco] ColecoVision (", effectiveRegion, ")"};
  if(medium == "MyVision") return string{"[Nichibutsu] My Vision (", region ? region : string{"NTSC-J"}, ")"};
  if(isFamicomFamily(medium)) return string{"[Nintendo] Famicom (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "Game Boy") return string{"[Nintendo] Game Boy"};
  if(medium == "Game Boy Color") return string{"[Nintendo] Game Boy Color"};
  if(medium == "Game Boy Advance") {
    if(gameBoyAdvancePlayer) return string{"[Nintendo] Game Boy Player"};
    return string{"[Nintendo] Game Boy Advance"};
  }
  if(medium == "Master System") return string{"[Sega] Master System (", effectiveRegion, ")"};
  if(medium == "Game Gear") return string{"[Sega] Game Gear (", effectiveRegion, ")"};
  if(medium == "Mega Drive") return string{"[Sega] Mega Drive (", effectiveRegion, ")"};
  if(medium == "Mega 32X") return string{"[Sega] Mega 32X (", effectiveRegion, ")"};
  if(medium == "Mega CD") return string{"[Sega] Mega CD (", effectiveRegion, ")"};
  if(medium == "Mega LD") return string{"[Pioneer] LaserActive (SEGA PAC) (", effectiveRegion, ")"};
  if(medium == "Mega CD 32X") return string{"[Sega] Mega CD 32X (", effectiveRegion, ")"};
  if(medium == "MSX") return string{"[Microsoft] MSX (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "MSX2") return string{"[Microsoft] MSX2 (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "Neo Geo") return string{"[SNK] Neo Geo AES"};
  if(medium == "Neo Geo Pocket") return string{"[SNK] Neo Geo Pocket"};
  if(medium == "Neo Geo Pocket Color") return string{"[SNK] Neo Geo Pocket Color"};
  if(medium == "Nintendo 64") return string{"[Nintendo] Nintendo 64 (", effectiveRegion, ")"};
  if(medium == "Nintendo 64DD") return string{"[Nintendo] Nintendo 64DD (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "PC Engine") return string{"[NEC] PC Engine (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "PC Engine CD") return string{"[NEC] PC Engine CD (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "PC Engine LD") return string{"[Pioneer] LaserActive (NEC PAC) (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "PlayStation") return string{"[Sony] PlayStation (", effectiveRegion, ")"};
  if(medium == "Pocket Challenge V2") return string{"[Benesse] Pocket Challenge V2"};
  if(medium == "Saturn") return string{"[Sega] Saturn (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "SG-1000") return string{"[Sega] SG-1000 (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "SC-3000") return string{"[Sega] SC-3000 (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "Super Famicom") return string{"[Nintendo] Super Famicom (", region ? region : string{"NTSC-J"}, ")"};
  if(medium == "SuperGrafx") return string{"[NEC] SuperGrafx (NTSC-J)"};
  if(medium == "WonderSwan") return string{"[Bandai] WonderSwan"};
  if(medium == "WonderSwan Color") return string{"[Bandai] WonderSwan Color"};
  if(medium == "ZX Spectrum") return string{"[Sinclair] ZX Spectrum"};
  if(medium == "ZX Spectrum 128") return string{"[Sinclair] ZX Spectrum 128"};
  return {};
}

auto configureCoreOptionsForMedium(const string& medium, const CoreOptions& options) -> void {
#ifdef CORE_GBA
  if(medium == "Game Boy Advance") {
    ares::GameBoyAdvance::option("Pixel Accuracy", options.videoPixelAccuracy);
  }
#endif
#ifdef CORE_MD
  if(isMegaDriveFamily(medium)) {
    ares::MegaDrive::option("TMSS", options.megadriveTMSS);
    ares::MegaDrive::option("Recompiler", !options.generalForceInterpreter);
  }
#endif
#ifdef CORE_PCE
  if(isPCEFamily(medium)) {
    ares::PCEngine::option("Pixel Accuracy", options.videoPixelAccuracy);
  }
#endif
#ifdef CORE_PS1
  if(medium == "PlayStation") {
    ares::PlayStation::option("Homebrew Mode", options.generalHomebrewMode);
    ares::PlayStation::option("Recompiler", !options.generalForceInterpreter);
  }
#endif
#ifdef CORE_SFC
  if(medium == "Super Famicom") {
    ares::SuperFamicom::option("Pixel Accuracy", options.videoPixelAccuracy);
  }
#endif
#ifdef CORE_WS
  if(isWonderSwanFamily(medium)) {
    ares::WonderSwan::option("Pixel Accuracy", options.videoPixelAccuracy);
  }
#endif
#ifdef CORE_N64
  if(isNintendo64Family(medium)) {
    ares::Nintendo64::option("Quality", options.videoQuality);
    ares::Nintendo64::option("Supersampling", options.videoSupersampling);
#if defined(VULKAN)
    ares::Nintendo64::option("Enable GPU acceleration", true);
#else
    ares::Nintendo64::option("Enable GPU acceleration", false);
#endif
    ares::Nintendo64::option("Disable Video Interface Processing", options.videoDisableVideoInterfaceProcessing);
    ares::Nintendo64::option("Weave Deinterlacing", options.videoWeaveDeinterlacing);
    ares::Nintendo64::option("Homebrew Mode", options.generalHomebrewMode);
    ares::Nintendo64::option("Recompiler", !options.generalForceInterpreter);
    ares::Nintendo64::option("Expansion Pak", options.nintendo64ExpansionPak);
    ares::Nintendo64::option("Controller Pak Banks", options.nintendo64ControllerPakBankString);
  }
#endif
}

auto loadCoreForMedium(const string& medium, ares::Node::System& root, const string& profile) -> bool {
#ifdef CORE_A26
  if(medium == "Atari 2600") return ares::Atari2600::load(root, profile);
#endif
#ifdef CORE_CV
  if(medium == "ColecoVision") return ares::ColecoVision::load(root, profile);
#endif
#ifdef CORE_MYVISION
  if(medium == "MyVision") return ares::MyVision::load(root, profile);
#endif
#ifdef CORE_FC
  if(isFamicomFamily(medium)) return ares::Famicom::load(root, profile);
#endif
#ifdef CORE_GB
  if(isGameBoyFamily(medium)) return ares::GameBoy::load(root, profile);
#endif
#ifdef CORE_GBA
  if(medium == "Game Boy Advance") return ares::GameBoyAdvance::load(root, profile);
#endif
#ifdef CORE_MS
  if(isMasterSystemFamily(medium)) return ares::MasterSystem::load(root, profile);
#endif
#ifdef CORE_MD
  if(isMegaDriveFamily(medium)) {
    return ares::MegaDrive::load(root, profile);
  }
#endif
#ifdef CORE_MSX
  if(isMSXFamily(medium)) return ares::MSX::load(root, profile);
#endif
#ifdef CORE_NG
  if(medium == "Neo Geo") return ares::NeoGeo::load(root, profile);
#endif
#ifdef CORE_NGP
  if(isNeoGeoPocketFamily(medium)) return ares::NeoGeoPocket::load(root, profile);
#endif
#ifdef CORE_N64
  if(isNintendo64Family(medium)) return ares::Nintendo64::load(root, profile);
#endif
#ifdef CORE_PCE
  if(isPCEFamily(medium)) {
    return ares::PCEngine::load(root, profile);
  }
#endif
#ifdef CORE_PS1
  if(medium == "PlayStation") return ares::PlayStation::load(root, profile);
#endif
#ifdef CORE_WS
  if(isWonderSwanFamily(medium)) {
    return ares::WonderSwan::load(root, profile);
  }
#endif
#ifdef CORE_SATURN
  if(medium == "Saturn") return ares::Saturn::load(root, profile);
#endif
#ifdef CORE_SG
  if(isSG1000Family(medium)) return ares::SG1000::load(root, profile);
#endif
#ifdef CORE_SFC
  if(medium == "Super Famicom") return ares::SuperFamicom::load(root, profile);
#endif
#ifdef CORE_SPEC
  if(isZXSpectrumFamily(medium)) return ares::ZXSpectrum::load(root, profile);
#endif
  return false;
}

auto connectDefaultPorts(ares::Node::System& root) -> void {
  tryConnectPort(root, "Cartridge Slot");
  tryConnectPort(root, "Disc Tray");
  tryConnectPort(root, "Disk Drive");
  tryConnectPort(root, "Tape Deck/Tray");
  tryConnectPort(root, "PC Engine LD/Disc Tray");
  tryConnectPort(root, "Nintendo 64DD/Disk Drive");
  for(u32 id : range(8)) {
    tryConnectPort(root, {"Controller Port ", id + 1}, string{"Gamepad"});
    tryConnectPort(root, {"Controller Port ", id + 1});
  }
  tryConnectPort(root, "Keyboard");
}

}
