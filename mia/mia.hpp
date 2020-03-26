#include <nall/nall.hpp>
#include <nall/cd.hpp>
using namespace nall;

#include <hiro/hiro.hpp>
using namespace hiro;

#include <ares/information.hpp>
#include <ares/resource/resource.hpp>

#include "resource/resource.hpp"

namespace mia {
  #include "settings/settings.hpp"
  #include "media/media.hpp"
  #include "cartridge/cartridge.hpp"
  #include "compact-disc/compact-disc.hpp"
  #include "floppy-disk/floppy-disk.hpp"
  #include "program/program.hpp"

  extern vector<shared_pointer<Media>> media;
  auto construct() -> void;
}
