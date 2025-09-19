#include <span>

#include <nall/nall.hpp>
#include <nall/cd.hpp>
#include <nall/ips.hpp>
#include <nall/span-helpers.hpp>
#include <nall/vfs.hpp>
#include <nall/beat/single/apply.hpp>
#include <nall/decode/cue.hpp>
#include <nall/string/markup/json.hpp>
#if defined(ARES_ENABLE_CHD)
#include <nall/decode/chd.hpp>
#endif
#include <nall/decode/wav.hpp>
#include <nall/decode/mmi.hpp>
using namespace nall;

#if !defined(MIA_LIBRARY)
#include <hiro/hiro.hpp>
using namespace hiro;
#endif

#include <ares/ares.hpp>
#include <ares/resource/resource.hpp>
#include <mia/resource/resource.hpp>

enum ResultEnum {
  successful,
  noFileSelected,
  databaseNotFound,
  romNotFoundInDatabase,
  romNotFound,
  invalidROM,
  couldNotParseManifest,
  noFirmware,
  otherError
};

struct LoadResult {
  ResultEnum result;

  string info;
  string firmwareType;
  string firmwareSystemName;
  string firmwareRegion;

  LoadResult(ResultEnum r) : result(r) {}

  LoadResult(ResultEnum r, string i) : result(r), info(i) {}

  bool operator==(const LoadResult& other) {
    return result == other.result;
  }
  bool operator!=(const LoadResult& other) {
    return result != other.result;
  }
};

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
