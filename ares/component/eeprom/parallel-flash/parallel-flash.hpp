#pragma once

namespace ares {

struct ParallelFlash {
  enum class Model : u32 { SST29EE010, SST29EE020, W29C040, AT29C040A };
  std::vector<u8> memory;
  u32 capacity;
  u32 pageSize;
  u8 deviceID;
  u8 manufacturerID;
  u8 page[256] = {};
  enum class Phase : u32 { Read, Load, Program, Erase } phase = Phase::Read;
  u32 command = 0;
  u32 pageAddress = 0;
  u32 remaining = 0;
  u8 lastData = 0xff;
  bool id = false;
  bool protection = false;
  bool toggle = false;
  ParallelFlash(Model model);
  auto power() -> void;
  auto read(u32 address) -> u8;
  auto write(u32 address, u8 data) -> void;
  auto step(u32 microseconds) -> void;
  auto serialize(serializer&) -> void;
};

}
