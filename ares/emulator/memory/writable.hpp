#pragma once

#include <emulator/memory/memory.hpp>

namespace ares::Memory {

template<typename T>
struct Writable {
  ~Writable() { reset(); }

  inline auto reset() -> void {
    delete[] self.data;
    self.data = nullptr;
    self.size = 0;
    self.mask = 0;
  }

  inline auto allocate(uint size, T fill = ~0ull) -> void {
    if(!size) return reset();
    delete[] self.data;
    self.size = size;
    self.mask = bit::round(self.size) - 1;
    self.data = new T[self.mask + 1];
    memory::fill<T>(self.data, self.mask + 1, fill);
  }

  inline auto fill(T fill = ~0ull) -> void {
    for(uint address : range(self.size)) {
      self.data[address] = fill;
    }
  }

  inline auto load(shared_pointer<vfs::file> fp) -> void {
    if(!self.size) return;
    fp->read(self.data, min(fp->size(), self.size * sizeof(T)));
    for(uint address = self.size; address <= self.mask; address++) {
      self.data[address] = self.data[mirror(address, self.size)];
    }
  }

  inline auto save(shared_pointer<vfs::file> fp) -> void {
    if(!self.size) return;
    fp->write(self.data, self.size * sizeof(T));
  }

  explicit operator bool() const { return (bool)self.data; }
  inline auto data() -> T* { return self.data; }
  inline auto data() const -> const T* { return self.data; }
  inline auto size() const -> uint { return self.size; }
  inline auto mask() const -> uint { return self.mask; }

  inline auto operator[](uint address) -> T& { return self.data[address & self.mask]; }
  inline auto operator[](uint address) const -> T { return self.data[address & self.mask]; }
  inline auto read(uint address) const -> T { return self.data[address & self.mask]; }
  inline auto write(uint address, T data) -> void { self.data[address & self.mask] = data; }
  inline auto program(uint address, T data) -> void { self.data[address & self.mask] = data; }

  auto begin() -> T* { return &self.data[0]; }
  auto end() -> T* { return &self.data[self.size]; }

  auto begin() const -> const T* { return &self.data[0]; }
  auto end() const -> const T* { return &self.data[self.size]; }

  auto serialize(serializer& s) -> void {
    const uint size = self.size;
    s.integer(self.size);
    s.integer(self.mask);
    if(self.size != size) allocate(self.size);
    s.array(self.data, self.size);
  }

private:
  struct {
    T* data = nullptr;
    uint size = 0;
    uint mask = 0;
  } self;
};

}
