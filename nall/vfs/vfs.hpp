#pragma once

#include <nall/range.hpp>
#include <nall/shared-pointer.hpp>

namespace nall::vfs {

struct file {
  enum class mode  : u32 { read, write, modify, create };
  enum class index : u32 { absolute, relative };

  virtual ~file() = default;

  virtual auto size() const -> u64 = 0;
  virtual auto offset() const -> u64 = 0;

  virtual auto seek(s64 offset, index = index::absolute) -> void = 0;
  virtual auto read() -> u8 = 0;
  virtual auto write(u8 data) -> void = 0;
  virtual auto flush() -> void {}

  auto end() const -> bool {
    return offset() >= size();
  }

  auto read(void* vdata, u64 bytes) -> void {
    auto data = (u8*)vdata;
    while(bytes--) *data++ = read();
  }

  auto readl(u32 bytes) -> u64 {
    u64 data = 0;
    for(auto n : range(bytes)) data |= (u64)read() << n * 8;
    return data;
  }

  auto readm(u32 bytes) -> u64 {
    u64 data = 0;
    for(auto n : range(bytes)) data = data << 8 | read();
    return data;
  }

  auto reads() -> string {
    string s;
    s.resize(size());
    read(s.get<u8>(), s.size());
    return s;
  }

  auto write(const void* vdata, u64 bytes) -> void {
    auto data = (const u8*)vdata;
    while(bytes--) write(*data++);
  }

  auto writel(u64 data, u32 bytes) -> void {
    for(auto n : range(bytes)) write(data), data >>= 8;
  }

  auto writem(u64 data, u32 bytes) -> void {
    for(auto n : reverse(range(bytes))) write(data >> n * 8);
  }

  auto writes(const string& s) -> void {
    write(s.data<u8>(), s.size());
  }
};

}

#include <nall/vfs/cdrom.hpp>
#include <nall/vfs/disk.hpp>
#include <nall/vfs/memory.hpp>
