#pragma once

#include <nall/file.hpp>
#include <nall/decode/zip.hpp>

namespace nall::vfs {

struct memory : file {
  ~memory() { delete[] _data; }

  static auto open(const void* data, u64 size) -> shared_pointer<memory> {
    auto instance = shared_pointer<memory>{new memory};
    instance->_open((const u8*)data, size);
    return instance;
  }

  static auto open(string location, bool decompress = false) -> shared_pointer<memory> {
    auto instance = shared_pointer<memory>{new memory};
    if(decompress && location.iendsWith(".zip")) {
      Decode::ZIP archive;
      if(archive.open(location) && archive.file.size() == 1) {
        auto memory = archive.extract(archive.file.first());
        instance->_open(memory.data(), memory.size());
        return instance;
      }
    }
    auto memory = nall::file::read(location);
    instance->_open(memory.data(), memory.size());
    return instance;
  }

  auto data() const -> const u8* { return _data; }
  auto size() const -> u64 override { return _size; }
  auto offset() const -> u64 override { return _offset; }

  auto seek(s64 offset, index mode) -> void override {
    if(mode == index::absolute) _offset  = (u64)offset;
    if(mode == index::relative) _offset += (s64)offset;
  }

  auto read() -> u8 override {
    if(_offset >= _size) return 0x00;
    return _data[_offset++];
  }

  auto write(u8 data) -> void override {
    if(_offset >= _size) return;
    _data[_offset++] = data;
  }

private:
  memory() = default;
  memory(const file&) = delete;
  auto operator=(const memory&) -> memory& = delete;

  auto _open(const u8* data, u64 size) -> void {
    _size = size;
    _data = new u8[size];
    nall::memory::copy(_data, data, size);
  }

  u8* _data = nullptr;
  u64 _size = 0;
  u64 _offset = 0;
};

}
