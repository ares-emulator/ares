#pragma once

//Generic flash

namespace ares {

struct SST39SF0x0 {
  auto reset() -> void;
  auto read(n32 address) -> n8;
  auto write(n32 address, n8 data) -> void;

  Memory::Writable<n8> flash;

  enum class Mode : u32 {
    Data,
    ID, /* 0x90 */
    Program, /* 0xA0 */
    Erase, /* 0x80 */
    EraseBlock /* 0x80 0x30 */
  } mode;

  enum class UnlockStep : u32 {
    Unlock0, /* 0xF0 */
    Unlock1,
    Unlock2
  } unlockStep;
};

}
