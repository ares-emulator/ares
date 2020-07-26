#pragma once

namespace nall::recompiler {
  using  u8 =  uint8_t;
  using u16 = uint16_t;
  using u32 = uint32_t;
  using u64 = uint64_t;

  struct amd64 {
    #include "emitter.hpp"
    #include "constants.hpp"
    #include "encoder-instructions.hpp"
    #if defined(PLATFORM_WINDOWS)
      #include "encoder-calls-windows.hpp"
    #else
      #include "encoder-calls-systemv.hpp"
    #endif
  };
}
