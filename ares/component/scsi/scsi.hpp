#pragma once

#include <functional>
#include <span>
#include <vector>

namespace ares {

struct SCSIDevice {
  virtual ~SCSIDevice() = default;

  virtual auto power() -> void { status = 0; }
  virtual auto execute(std::span<const u8> cdb) -> void = 0;
  virtual auto dataInAvailable() -> bool { return false; }
  virtual auto dataIn() -> n8 { return 0; }
  virtual auto dataOutExpected() -> bool { return false; }
  virtual auto dataOut(n8 data) -> void {}
  virtual auto serialize(serializer&) -> void {}

  static auto commandLength(n8 opcode) -> u32;

  n8 status = 0;
};

}
