#pragma once
#include <cstddef>
#include <cstdint>

namespace nall::TCP {
// Run on the socket's send worker, never on the emulator/frontend thread.
// Write returns a positive byte count, -1 for retryable interruption/would-block,
// and zero for a terminal failure. No byte is resent after partial progress.
template<typename Write, typename Valid, typename Pause>
auto sendAll(const uint8_t* data, size_t size, Write write, Valid valid, Pause pause) -> bool {
  size_t offset = 0;
  while(offset < size) {
    if(!valid()) return false;
    auto count = write(data + offset, size - offset);
    if(count == -1) { pause(); continue; }
    if(count <= 0 || uint64_t(count) > size - offset) return false;
    offset += size_t(count);
  }
  return true;
}
}
