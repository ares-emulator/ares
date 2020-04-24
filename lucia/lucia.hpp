#include <ruby/ruby.hpp>
//using namespace ruby;

#include <hiro/hiro.hpp>
using namespace hiro;

#include <ares/ares.hpp>
#include <mia/mia.hpp>

#include <nall/instance.hpp>
#include <nall/beat/single/apply.hpp>
#include <nall/encode/png.hpp>
#include <nall/hash/crc16.hpp>

namespace ruby {
  extern Video video;
  extern Audio audio;
  extern Input input;
}

#include "resource/resource.hpp"
#include "emulator/emulator.hpp"
#include "program/program.hpp"
#include "input/input.hpp"
#include "presentation/presentation.hpp"
#include "settings/settings.hpp"
#include "tools/tools.hpp"

auto locate(const string& name) -> string;
