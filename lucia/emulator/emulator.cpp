#include "../lucia.hpp"
#include "emulators.cpp"

vector<shared_pointer<Emulator>> emulators;
shared_pointer<Emulator> emulator;

auto Emulator::locate(const string& location, const string& suffix, const string& path, maybe<string> system) -> string {
  if(!system) system = root->name();

  //game path
  if(!path) return {Location::notsuffix(location), suffix};

  //path override
  string pathname = {path, *system, "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
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
          return vfs::memory::open(manifest);
        }
      }
    }
    auto manifest = medium->manifest(location);
    return vfs::memory::open(manifest);
  }
  return {};
}

auto Emulator::region() -> string {
  auto regions = game.pak->attribute("region").split(",").strip();
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

auto Emulator::load(const string& location) -> bool {
  system.location = {Path::userData(), "lucia/Saves/", name, "/"};
  system.pak = shared_pointer{new vfs::directory};
  directory::create(system.location);

  game.location = location;
  game.pak = medium->load(location);
  if(!game.pak) return false;

  latch = {};
  if(!load()) return false;
  configuration.game = Location::dir(location);
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
      return vfs::memory::open(image);
    }
  } else if(auto image = file::read(firmware.location)) {
    return vfs::memory::open(image);
  }
  return {};
}

auto Emulator::unload() -> void {
  save();
  root->unload();
  root.reset();
  system = {};
  game = {};
}

auto Emulator::load(Emulator::Pak& node, string name) -> bool {
  if(auto fp = node.pak->read(name)) {
    if(auto memory = file::read({node.location, name})) {
      fp->read(memory);
      return true;
    }
  }
  return false;
}

auto Emulator::save(Emulator::Pak& node, string name) -> bool {
  if(auto memory = node.pak->write(name)) {
    return file::write({node.location, name}, {memory->data(), memory->size()});
  }
  return false;
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
