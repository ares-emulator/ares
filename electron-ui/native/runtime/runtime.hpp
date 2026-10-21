#pragma once

#include <ruby/ruby.hpp>
#include <ares/ares.hpp>
#include <ares/resource/resource.hpp>
#include <mia/mia.hpp>
#include <nall/hash/crc16.hpp>
#include <mutex>

namespace ruby { extern Input input; }

// Only the native host's emulation thread loads/unloads or changes core state.
// Input device updates also arrive on the stdin reader, so mappings share a mutex.
struct FrontendRuntime {
  std::recursive_mutex inputMutex;
  bool keyboardCaptured = true;
  bool externalInput = true;
  std::vector<string> mediaQueue;
};
extern FrontendRuntime runtime;

#include "input/input.hpp"
#include "emulator/emulator.hpp"
#include "settings/settings.hpp"

auto updatePalette() -> void;
