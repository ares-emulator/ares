#include "mia.hpp"

namespace mia {

vector<shared_pointer<Media>> media;

auto locate(string name) -> string {
  string location = {Path::program(), name};
  if(inode::exists(location)) return location;

  directory::create({Path::userData(), "mia/"});
  return {Path::userData(), "mia/", name};
}

auto operator+=(string& lhs, const string& rhs) -> string& {
  lhs.append(rhs);
  return lhs;
}

#include "settings/settings.cpp"
#include "media/media.cpp"
#include "cartridge/cartridge.cpp"
#include "compact-disc/compact-disc.cpp"
#include "floppy-disk/floppy-disk.cpp"
#include "program/program.cpp"

auto construct() -> void {
  media.append(new BSMemory);
  media.append(new ColecoVision);
  media.append(new Famicom);
  media.append(new FamicomDisk);
  media.append(new GameBoy);
  media.append(new GameBoyAdvance);
  media.append(new GameBoyColor);
  media.append(new GameGear);
  media.append(new MasterSystem);
  media.append(new MegaCD);
  media.append(new MegaDrive);
  media.append(new MSX);
  media.append(new MSX2);
  media.append(new NeoGeoPocket);
  media.append(new NeoGeoPocketColor);
  media.append(new Nintendo64);
  media.append(new Nintendo64DD);
  media.append(new PCEngine);
  media.append(new PCEngineCD);
  media.append(new PlayStation);
  media.append(new PocketChallengeV2);
  media.append(new SC3000);
  media.append(new SG1000);
  media.append(new SufamiTurbo);
  media.append(new SuperFamicom);
  media.append(new SuperGrafx);
  media.append(new WonderSwan);
  media.append(new WonderSwanColor);
  for(auto& medium : media) medium->construct();
}

auto medium(const string& name) -> shared_pointer<Media> {
  for(auto& medium : media) {
    if(medium->name() == name) return medium;
  }
  return {};
}

auto identify(const string& filename) -> string {
  auto extension = Location::suffix(filename).trimLeft(".", 1L).downcase();

  if(extension == "zip") {
    Decode::ZIP archive;
    if(archive.open(filename)) {
      for(auto& file : archive.file) {
        auto match = Location::suffix(file.name).trimLeft(".", 1L).downcase();
        for(auto& medium : media) {
          if(medium->extensions().find(match)) {
            extension = match;
          }
        }
      }
    }
  }

  for(auto& medium : media) {
    if(medium->extensions().find(extension)) {
      if(auto manifest = medium->manifest(filename)) {
        return medium->name();
      }
    }
  }

  return {};  //unable to identify
}

auto main(Arguments arguments) -> void {
  Application::setName("mia");

  construct();

  if(auto document = file::read({Path::userSettings(), "mia/settings.bml"})) {
    settings.unserialize(document);
  }

  if(arguments.take("--name")) {
    return print("mia");
  }

  if(string filename; arguments.take("--identify", filename)) {
    return print(identify(filename), "\n");
  }

  if(string system; arguments.take("--system", system)) {
    for(auto& medium : media) {
      if(medium->name() != system) continue;

      if(string manifest; arguments.take("--manifest", manifest)) {
        if(auto result = medium->manifest(manifest)) return print(result);
        if(medium->extensions().find("cue")) {
          //audio CD-ROM fallback
          string result;
          result.append("game\n");
          result.append("  name:  ", medium->name(manifest), "\n");
          result.append("  label: ", medium->name(manifest), "\n");
          return print(result);
        }
        return;
      }

      if(string import; arguments.take("--import", import)) {
        return (void)medium->import(import);
      }

      if(arguments.take("--import")) {
        if(auto import = BrowserDialog()
        .setTitle({"Import ", system, " Game"})
        .setPath(settings.recent)
        .setAlignment(Alignment::Center)
        .openFile()
        ) {
          if(auto error = medium->import(import)) {
            MessageDialog()
            .setTitle("Error")
            .setAlignment(Alignment::Center)
            .setText({"Failed to import: ", Location::file(import), "\n\nError: ", error, "."})
            .error();
          }
        }
        return;
      }
    }
  }

  Instances::programWindow.construct();

  #if defined(PLATFORM_MACOS)
  Application::Cocoa::onAbout([&] { programWindow.aboutAction.doActivate(); });
  Application::Cocoa::onPreferences([&] {});
  Application::Cocoa::onQuit([&] { Application::quit(); });
  #endif

  programWindow.setVisible();
  Application::run();

  directory::create({Path::userSettings(), "mia/"});
  file::write({Path::userSettings(), "mia/settings.bml"}, settings.serialize());

  Instances::programWindow.destruct();
}

}

#if !defined(MIA_LIBRARY)

#include <ares/resource/resource.cpp>

#include <nall/main.hpp>
auto nall::main(Arguments arguments) -> void {
  mia::main(arguments);
}

#endif
