#include "mia.hpp"

#include <TZXFile.h>

namespace mia {

function<string ()> homeLocation = [] { return string{Path::user(), "Emulation/Systems/"}; };
function<string ()> saveLocation = [] { return string{}; };
std::vector<string> media;

auto locate(const string &name) -> string {
  // First check each path for the presence of the file we are looking for in the following order
  // allowing users to override the default resources if they wish to do so.

  // 1. The application directory
  string location = {Path::program(), name};
  if (inode::exists(location)) return location;

  // 2. The user data directory
  location = {Path::userData(), "ares/", name};
  if (inode::exists(location)) return location;

  // 3. The shared data directory
#if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)
  /// Unix-like systems have multiple notions of a 'shared data' directory. First, check for
  /// an install prefix, as would be used by package managers that do not use `/usr/share`.
  /// Secondly, look in `/usr/local/share` to cover software compiled by the user.
  /// Lastly, look in the 'global' shared data directory, `/usr/share`.
  location = {Path::prefixSharedData(), "ares/", name};
  if (inode::exists(location)) return location;
  
  location = {Path::localSharedData(), "ares/", name};
  if (inode::exists(location)) return location;
#endif
  
  location = {Path::sharedData(), "ares/", name};
  if (inode::exists(location)) return location;

  // 4. The application bundle resource directory (macOS only)
#if defined(PLATFORM_MACOS)
  location = {Path::resources(), name};
  if(inode::exists(location)) return location;
#endif

  // If the file was not found in any of the above locations, we may be intending to create it
#if defined(PLATFORM_WINDOWS)
  // We must return a path to a user writable directory; on Windows, this is the executable directory
  return {Path::program(), name};
#else
  // On other platforms, this is the "user data" directory
  directory::create({Path::userData(), "ares/"});
  return {Path::userData(), "ares/", name};
#endif

}

auto operator+=(string& lhs, const string& rhs) -> string& {
  lhs.append(rhs);
  return lhs;
}

auto hexString(array_view<u8> view) -> string {
  string s;
  for(u8 n : view) s.append(hex(n, 2L), " ");
  return s.stripRight();
}

#include "settings/settings.cpp"
#include "system/system.cpp"
#include "medium/medium.cpp"
#include "pak/pak.cpp"
#if !defined(MIA_LIBRARY)
#include "program/program.cpp"
#endif

auto setHomeLocation(function<string ()> callback) -> void {
  homeLocation = callback;
}

auto setSaveLocation(function<string ()> callback) -> void {
  saveLocation = callback;
}

auto construct() -> void {
  static bool initialized = false;
  if(initialized) return;
  initialized = true;

  media.push_back("Atari 2600");
  media.push_back("BS Memory");
  media.push_back("ColecoVision");
  media.push_back("MyVision");
  media.push_back("Famicom");
  media.push_back("Famicom Disk System");
  media.push_back("Game Boy");
  media.push_back("Game Boy Color");
  media.push_back("Game Boy Advance");
  media.push_back("Game Gear");
  media.push_back("Master System");
  media.push_back("Mega Drive");
  media.push_back("Mega 32X");
  media.push_back("Mega CD");
  media.push_back("Mega LD");  
  media.push_back("MSX");
  media.push_back("MSX2");
  media.push_back("Neo Geo");
  media.push_back("Neo Geo Pocket");
  media.push_back("Neo Geo Pocket Color");
  media.push_back("Nintendo 64");
  media.push_back("Nintendo 64DD");
  media.push_back("PC Engine");
  media.push_back("PC Engine CD");
  media.push_back("PlayStation");
  media.push_back("Pocket Challenge V2");
  media.push_back("Saturn");
  media.push_back("SC-3000");
  media.push_back("SG-1000");
  media.push_back("Sufami Turbo");
  media.push_back("Super Famicom");
  media.push_back("SuperGrafx");
  media.push_back("WonderSwan");
  media.push_back("WonderSwan Color");
  media.push_back("ZX Spectrum");
}

auto identify(const string& filename) -> string {
  construct();
  auto extension = Location::suffix(filename).trimLeft(".", 1L).downcase();

  if(extension == "zip") {
    Decode::ZIP archive;
    if(archive.open(filename)) {
      for(auto& file : archive.file) {
        auto match = Location::suffix(file.name).trimLeft(".", 1L).downcase();
        for(auto& medium : media) {
          auto pak = mia::Medium::create(medium);
          auto exts = pak->extensions();
          if(std::ranges::find(exts, match) != exts.end()) {
            extension = match;
          }
        }
      }
    }
  }

  for(auto& medium : media) {
    auto pak = mia::Medium::create(medium);
    auto exts = pak->extensions();
    if(std::ranges::find(exts, extension) != exts.end()) {
      if(pak->load(filename) != successful) continue; // Skip medium that the system cannot load
      if(pak->pak->attribute("audio").boolean()) continue; // Skip audio-only media to give the next system a chance to match
      return pak->name();
    }
  }

  return {};  //unable to identify
}

auto import(shared_pointer<Pak> pak, const string& filename) -> bool {
  if(pak->load(filename) == successful) {
    string pathname = {Path::user(), "Emulation/", pak->name(), "/", Location::prefix(filename), ".", pak->extensions().front(), "/"};
    if(!directory::create(pathname)) return false;
    for(auto& node : *pak->pak) {
      if(auto input = node.cast<vfs::file>()) {
        if(input->name() == "manifest.bml" && !settings.createManifests) continue;
        if(auto output = file::open({pathname, input->name()}, file::mode::write)) {
          while(!input->end()) output.write(input->read());
        }
      }
    }
    return true;
  }
  return false;
}

auto main(Arguments arguments) -> void {
  #if !defined(MIA_LIBRARY)
  Application::setName("mia");
  #endif

  construct();

  auto document = file::read(locate("settings.bml"));
  if(!document.empty()) {
    settings.unserialize(document);
  }

  if(arguments.take("--name")) {
    return print("mia");
  }

  if(string filename; arguments.take("--identify", filename)) {
    return print(identify(filename), "\n");
  }

  if(string system; arguments.take("--system", system)) {
    auto pak = mia::Medium::create(system);
    if(!pak) return;

    if(string manifest; arguments.take("--manifest", manifest)) {
      if(pak->load(manifest) == successful) {
        if(auto fp = pak->pak->read("manifest.bml")) return print(fp->reads());
      }
      return;
    }

    if(string import; arguments.take("--import", import)) {
      return (void)mia::import(pak, import);
    }

    #if !defined(MIA_LIBRARY)
    if(arguments.take("--import")) {
      if(auto import = BrowserDialog()
      .setTitle({"Import ", system, " Game"})
      .setPath(settings.recent)
      .setAlignment(Alignment::Center)
      .openFile()
      ) {
        if(!mia::import(pak, import)) {
          MessageDialog()
          .setTitle("Error")
          .setAlignment(Alignment::Center)
          .setText({"Failed to import: ", Location::file(import)})
          .error();
        }
      }
      return;
    }
    #endif
  }

  #if !defined(MIA_LIBRARY)
  Instances::programWindow.construct();

  #if defined(PLATFORM_MACOS)
  Application::Cocoa::onAbout([&] { programWindow.aboutAction.doActivate(); });
  Application::Cocoa::onPreferences([&] {});
  Application::Cocoa::onQuit([&] { Application::quit(); });
  #endif

  programWindow.setVisible();
  Application::run();

  Instances::programWindow.destruct();
  #endif

  file::write(locate("settings.bml"), settings.serialize());
}

}

#if !defined(MIA_LIBRARY)
#include <nall/main.hpp>

auto nall::main(Arguments arguments) -> void {
  mia::main(arguments);
}
#endif
