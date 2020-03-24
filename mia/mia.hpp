#include <nall/nall.hpp>
#include <nall/cd.hpp>
#include <hiro/hiro.hpp>
using namespace nall;
using namespace hiro;

#include "resource/resource.hpp"

namespace mia {
  static const string Name      = "mia";
  static const string Version   = "111";
  static const string License   = "GPLv3+";
  static const string Website   = "https://ares.dev";

  #include "settings/settings.hpp"
  #include "media/media.hpp"
  #include "cartridge/cartridge.hpp"
  #include "compact-disc/compact-disc.hpp"
  #include "floppy-disk/floppy-disk.hpp"
  #include "program/program.hpp"

  extern vector<shared_pointer<Media>> media;
  auto construct() -> void;
}
