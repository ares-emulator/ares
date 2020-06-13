#pragma once

namespace nall {

struct bump_allocator {
  static constexpr uint32_t executable = 1 << 0;
  static constexpr uint32_t zero_fill  = 1 << 1;

  ~bump_allocator() {
    reset();
  }

  explicit operator bool() const {
    return _memory;
  }

  auto reset() -> void {
    free(_memory);
    _memory = nullptr;
  }

  auto resize(uint32_t capacity, uint32_t flags = 0) -> bool {
    reset();
    _offset = 0;
    _capacity = capacity + 4095 & ~4095;  //alignment
    _memory = (uint8_t*)malloc(_capacity);
    if(!_memory) return false;

    if(flags & executable) {
      #if defined(PLATFORM_WINDOWS)
      DWORD privileges;
      VirtualProtect((void*)_memory, _capacity, PAGE_EXECUTE_READWRITE, &privileges);
      #else
      mprotect(_memory, _capacity, PROT_READ | PROT_WRITE | PROT_EXEC);
      #endif
    }

    if(flags & zero_fill) {
      memset(_memory, 0x00, _capacity);
    }

    return true;
  }

  //release all acquired memory
  auto release(uint32_t flags = 0) -> void {
    _offset = 0;
    if(flags & zero_fill) memset(_memory, 0x00, _capacity);
  }

  auto capacity() const -> uint32_t {
    return _capacity;
  }

  auto available() const -> uint32_t {
    return _capacity - _offset;
  }

  //for allocating blocks of known size
  auto acquire(uint32_t size) -> uint8_t* {
    #ifdef DEBUG
    struct out_of_memory {};
    if((_offset + size + 15 & ~15) > _capacity) throw out_of_memory{};
    #endif
    auto memory = _memory + _offset;
    _offset = _offset + size + 15 & ~15;  //alignment
    return memory;
  }

  //for allocating blocks of unknown size (eg for a dynamic recompiler code block)
  auto acquire() -> uint8_t* {
    #ifdef DEBUG
    struct out_of_memory {};
    if(_offset > _capacity) throw out_of_memory{};
    #endif
    return _memory + _offset;
  }

  //size can be reserved once the block size is known
  auto reserve(uint32_t size) -> void {
    #ifdef DEBUG
    struct out_of_memory {};
    if((_offset + size + 15 & ~15) > _capacity) throw out_of_memory{};
    #endif
    _offset = _offset + size + 15 & ~15;  //alignment
  }

private:
  uint8_t* _memory = nullptr;
  uint32_t _capacity = 0;
  uint32_t _offset = 0;
};

}
