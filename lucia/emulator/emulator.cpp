#include "../lucia.hpp"

#ifdef CORE_CV
  #include "colecovision.cpp"
#endif

#ifdef CORE_FC
  #include "famicom.cpp"
#endif

#ifdef CORE_GB
  #include "game-boy.cpp"
#endif

#ifdef CORE_GBA
  #include "game-boy-advance.cpp"
#endif

#ifdef CORE_MD
  #include "mega-drive.cpp"
#endif

#ifdef CORE_MS
  #include "master-system.cpp"
#endif

#ifdef CORE_MSX
  #include "msx.cpp"
#endif

#ifdef CORE_N64
  #include "nintendo-64.cpp"
#endif

#ifdef CORE_NGP
  #include "neo-geo-pocket.cpp"
#endif

#ifdef CORE_PCE
  #include "pc-engine.cpp"
#endif

#ifdef CORE_PS1
  #include "playstation.cpp"
#endif

#ifdef CORE_SFC
  #include "super-famicom.cpp"
#endif

#ifdef CORE_SG
  #include "sg-1000.cpp"
#endif

#ifdef CORE_WS
  #include "wonderswan.cpp"
#endif

vector<shared_pointer<Emulator>> emulators;
shared_pointer<Emulator> emulator;

auto Emulator::construct() -> void {
  mia::construct();

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
  emulators.append(new MegaCD);
  #endif

  #ifdef CORE_PS1
  emulators.append(new PlayStation);
  #endif

  #ifdef CORE_PCE
  emulators.append(new PCEngine);
  emulators.append(new PCEngineCD);
  emulators.append(new SuperGrafx);
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

auto Emulator::locate(const string& location, const string& suffix, const string& path) -> string {
  //game path
  if(!path) return {Location::notsuffix(location), suffix};

  //path override
  string pathname = {path, root->name(), "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
}

//this is used to load manifests for cartridges
auto Emulator::manifest() -> shared_pointer<vfs::file> {
  //use a user-provided manifest file if it exists
  auto location = locate(game.location, ".bml");
  if(file::exists(location)) {
    game.manifest = file::read(location);
    return vfs::memory::open(game.manifest.data<u8>(), game.manifest.size());
  }

  //generate the manifest dynamically if it does not exist
  if(auto cartridge = medium.cast<mia::Cartridge>()) {
    game.manifest = cartridge->manifest(game.image, game.location);
    return vfs::memory::open(game.manifest.data<u8>(), game.manifest.size());
  }
  if(auto floppyDisk = medium.cast<mia::FloppyDisk>()) {
    game.manifest = floppyDisk->manifest(game.image, game.location);
    return vfs::memory::open(game.manifest.data<u8>(), game.manifest.size());
  }
  if(auto compactDisc = medium.cast<mia::CompactDisc>()) {
    game.manifest = compactDisc->manifest(game.location);
    return vfs::memory::open(game.manifest.data<u8>(), game.manifest.size());
  }

  return {};
}

//this is used to load manifests for audio CD-ROMs
//it is a fallback when mia is unable to generate manifests for game CD-ROMs
auto Emulator::manifest(const string& location) -> shared_pointer<vfs::file> {
  string manifest;
  manifest.append("game\n");
  manifest.append("  name:  ", Location::prefix(location), "\n");
  manifest.append("  label: ", Location::prefix(location), "\n");
  manifest.append("  audio\n");
  return vfs::memory::open(manifest.data<u8>(), manifest.size());
}

//this is used to load manifests for system BIOSes
auto Emulator::manifest(const string& type, const string& location) -> shared_pointer<vfs::file> {
  if(auto medium = mia::medium(type)) {
    if(location.iendsWith(".zip")) {
      Decode::ZIP archive;
      if(archive.open(location) && archive.file) {
        auto image = archive.extract(archive.file.first());
        if(auto cartridge = medium.cast<mia::Cartridge>()) {
          auto manifest = cartridge->manifest(image, location);
          return vfs::memory::open(manifest.data<u8>(), manifest.size());
        }
      }
    }
    auto manifest = medium->manifest(location);
    return vfs::memory::open(manifest.data<u8>(), manifest.size());
  }
  return {};
}

auto Emulator::region() -> string {
  auto document = BML::unserialize(game.manifest);
  auto regions = document["game/region"].string().split(",").strip();
  if(!regions) return {};
  if(settings.boot.prefer == "NTSC-U" && regions.find("NTSC-U")) return "NTSC-U";
  if(settings.boot.prefer == "NTSC-J" && regions.find("NTSC-J")) return "NTSC-J";
  if(settings.boot.prefer == "NTSC-U" && regions.find("NTSC"  )) return "NTSC";
  if(settings.boot.prefer == "NTSC-J" && regions.find("NTSC"  )) return "NTSC";
  if(settings.boot.prefer == "PAL"    && regions.find("PAL"   )) return "PAL";
  if(regions.first()) return regions.first();
  if(settings.boot.prefer == "NTSC-J") return "NTSC-J";
  if(settings.boot.prefer == "NTSC-U") return "NTSC-U";
  if(settings.boot.prefer == "PAL"   ) return "PAL";
  return {};
}

auto Emulator::load(const string& location, const vector<u8>& image) -> bool {
  configuration.game = Location::dir(location);

  game.location = location;
  game.image = image;
  if(!manifest()) return false;

  latch = {};

  if(!load()) return false;
  setBoolean("Color Bleed", settings.video.colorBleed);
  setBoolean("Color Emulation", settings.video.colorEmulation);
  setBoolean("Interframe Blending", settings.video.interframeBlending);
  setOverscan(settings.video.overscan);

  root->power();
  return true;
}

auto Emulator::loadFirmware(const Firmware& firmware) -> shared_pointer<vfs::file> {
  if(firmware.location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(firmware.location) && archive.file) {
      auto image = archive.extract(archive.file.first());
      return vfs::memory::open(image.data(), image.size());
    }
  } else if(auto image = file::read(firmware.location)) {
    return vfs::memory::open(image.data(), image.size());
  }
  return {};
}

auto Emulator::save() -> void {
  root->save();
}

auto Emulator::unload() -> void {
  root->save();
  root->unload();
  root.reset();
}

auto Emulator::refresh() -> void {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->refresh();
  }
}

auto Emulator::setBoolean(const string& name, bool value) -> bool {
  if(auto node = root->scan<ares::Node::Setting::Boolean>(name)) {
    node->setValue(value);  //setValue() will not call modify() if value has not changed;
    node->modify(value);    //but that may prevent the initial setValue() from working
    return true;
  }
  return false;
}

auto Emulator::setOverscan(bool value) -> bool {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    if(auto overscan = screen->find<ares::Node::Setting::Boolean>("Overscan")) {
      overscan->setValue(value);
      return true;
    }
  }
  return false;
}

auto Emulator::error(const string& text) -> void {
  MessageDialog().setTitle("Error").setText(text).setAlignment(presentation).error();
}

auto Emulator::errorFirmwareRequired(const Firmware& firmware) -> void {
  if(MessageDialog().setText({
    emulator->name, " - ", firmware.type, " (", firmware.region, ") is required to play this game.\n"
    "Would you like to configure firmware settings now?"
  }).question() == "Yes") {
    settingsWindow.show("Firmware");
    firmwareSettings.select(emulator->name, firmware.type, firmware.region);
  }
}
