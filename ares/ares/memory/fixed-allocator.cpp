#include <ares/ares.hpp>

namespace ares::Memory {

alignas(4096) u8 fixedBuffer[1_GiB];

FixedAllocator::FixedAllocator() {
  _allocator.resize(sizeof(fixedBuffer), 0, fixedBuffer);
}

auto FixedAllocator::get() -> bump_allocator& {
  static FixedAllocator allocator;
  return allocator._allocator;
}

}
