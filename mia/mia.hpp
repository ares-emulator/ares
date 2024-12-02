#include <nall/nall.hpp>
#include <nall/cd.hpp>
#include <nall/ips.hpp>
#include <nall/vfs.hpp>
#include <nall/beat/single/apply.hpp>
#include <nall/decode/cue.hpp>
#include <nall/decode/chd.hpp>
#include <nall/decode/wav.hpp>
using namespace nall;

#if !defined(MIA_LIBRARY)
#include <hiro/hiro.hpp>
using namespace hiro;
#endif

#include <ares/ares.hpp>
#include <ares/resource/resource.hpp>
#include <mia/resource/resource.hpp>

namespace mia {
  #include "settings/settings.hpp"
  #include "pak/pak.hpp"
  #include "system/system.hpp"
  #include "medium/medium.hpp"
  #if !defined(MIA_LIBRARY)
  #include "program/program.hpp"
  #endif

  extern function<string ()> homeLocation;
  extern function<string ()> saveLocation;
  auto setHomeLocation(function<string ()>) -> void;
  auto setSaveLocation(function<string ()>) -> void;
  auto construct() -> void;
  auto identify(const string& filename) -> string;
  auto import(shared_pointer<Pak>, const string& filename) -> bool;
}
