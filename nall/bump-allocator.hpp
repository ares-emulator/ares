#pragma once

#include <nall/memory.hpp>

namespace nall {

struct bump_allocator {
  static constexpr u32 executable = 1 << 0;
  static constexpr u32 zero_fill  = 1 << 1;

  ~bump_allocator() {
    reset();
  }

  explicit operator bool() const {
    return _memory;
  }

  auto reset() -> void {
    if(_owner) memory::free<u8, 4096>(_memory);
    _memory = nullptr;
  }

  auto resize(u32 capacity, u32 flags = 0, u8* buffer = nullptr) -> bool {
    reset();
    _offset = 0;
    if(buffer) {
      _owner = false;
      _capacity = capacity;
      _memory = buffer;
    } else {
      _owner = true;
      _capacity = capacity + 4095 & ~4095;              //capacity alignment
      _memory = memory::allocate<u8, 4096>(_capacity);  //_SC_PAGESIZE alignment
      if(!_memory) return false;
    }

    if(flags & executable) {
      #if defined(PLATFORM_WINDOWS)
      DWORD privileges;
      VirtualProtect((void*)_memory, _capacity, PAGE_EXECUTE_READWRITE, &privileges);
      #else
      int ret = mprotect(_memory, _capacity, PROT_READ | PROT_WRITE | PROT_EXEC);
      assert(ret == 0);
      #endif
    }

    if(flags & zero_fill) {
      memset(_memory, 0x00, _capacity);
    }

    return true;
  }

  //release all acquired memory
  auto release(u32 flags = 0) -> void {
    _offset = 0;
    if(flags & zero_fill) memset(_memory, 0x00, _capacity);
  }

  auto capacity() const -> u32 {
    return _capacity;
  }

  auto available() const -> u32 {
    return _capacity - _offset;
  }

  //for allocating blocks of known size
  auto acquire(u32 size) -> u8* {
    #ifdef DEBUG
    struct out_of_memory {};
    if((_offset + size + 15 & ~15) > _capacity) throw out_of_memory{};
    #endif
    auto memory = _memory + _offset;
    _offset = _offset + size + 15 & ~15;  //alignment
    return memory;
  }

  //for allocating blocks of unknown size (eg for a dynamic recompiler code block)
  auto acquire() -> u8* {
    #ifdef DEBUG
    struct out_of_memory {};
    if(_offset > _capacity) throw out_of_memory{};
    #endif
    return _memory + _offset;
  }

  //size can be reserved once the block size is known
  auto reserve(u32 size) -> void {
    #ifdef DEBUG
    struct out_of_memory {};
    if((_offset + size + 15 & ~15) > _capacity) throw out_of_memory{};
    #endif
    _offset = _offset + size + 15 & ~15;  //alignment
  }

private:
  u8* _memory = nullptr;
  u32 _capacity = 0;
  u32 _offset = 0;
  bool _owner = false;
};

}
