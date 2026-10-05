#pragma once

#include <cassert>
#include <ctime>
#include <span>
#include <vector>
#include <nall/maybe.hpp>
#include <nall/string.hpp>

namespace nall::Decode {

// Minimal read-only interface shared by archive container decoders.
//
// Archive entries are value objects so callers can retain an entry while the
// archive remains open. Backends may use the format-specific fields internally;
// callers should prefer the Archive operations rather than interpreting them.
struct Archive {
  struct File {
    string name;
    const u8* data = nullptr;
    u64 size = 0;
    u64 csize = 0;
    u32 cmode = 0;
    u32 crc32 = 0;
    time_t timestamp = 0;
  };

  virtual ~Archive() = default;

  virtual auto findFile(const string& filename) const -> const maybe<File> = 0;
  virtual auto files() const -> std::vector<File> = 0;
  virtual auto open(const string& filename) -> bool = 0;
  virtual auto extract(const File& file) const -> std::vector<u8> = 0;
  virtual auto isDataUncompressed(const File& file) const -> bool = 0;
  virtual auto dataViewIfUncompressed(const File& file) const -> std::span<const u8> = 0;
  virtual auto close() -> void = 0;
};

}
