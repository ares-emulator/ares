#include <nall/nall.hpp>
#include <nall/cd.hpp>
#include <nall/vfs.hpp>
#include <nall/beat/single/apply.hpp>
#include <nall/decode/cue.hpp>
#include <nall/decode/wav.hpp>
using namespace nall;

#if !defined(MIA_LIBRARY)
#include <hiro/hiro.hpp>
using namespace hiro;
#endif

#include <ares/information.hpp>
#include <ares/resource/resource.hpp>
#include <mia/resource/resource.hpp>

namespace mia {
  #include "settings/settings.hpp"
  #include "media/media.hpp"
  #include "cartridge/cartridge.hpp"
  #include "compact-disc/compact-disc.hpp"
  #include "compact-disc/mega-cd.hpp"
  #include "compact-disc/pc-engine-cd.hpp"
  #include "compact-disc/playstation.hpp"
  #include "floppy-disk/floppy-disk.hpp"
  #if !defined(MIA_LIBRARY)
  #include "program/program.hpp"
  #endif

  extern vector<shared_pointer<Media>> media;
  extern function<string ()> saveLocation;
  auto setSaveLocation(function<string ()>) -> void;
  auto construct() -> void;
  auto medium(const string& name) -> shared_pointer<Media>;
  auto identify(const string& filename) -> string;
  auto import(shared_pointer<Media>, const string& filename) -> bool;
}
