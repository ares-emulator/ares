#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
#include <utility>
#include "execution.hpp"

// PI provenance adapted from ares-64 system/debug-session.hpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. Diagnostic-only, never serialized.
// Recording is opt-in. Nonidentity chip mappings retain attempted bus ranges,
// explicitly unconfirmed; initialized linear RDRAM ranges are clipped to storage.
namespace ares::Nintendo64 {
struct DmaProvenance {
  struct Range { uint32_t address, length; };
  struct Record {
    uint64_t id = 0, epoch = 0, frame = 0, pc = 0;
    uint32_t cartAddress = 0, dramAddress = 0, requestedLength = 0, romOffset = 0;
    uint32_t bytesAttempted = 0, bytesWritten = 0, ramSize = 0;
    bool fromRom = false, writesConfirmed = false, rangesTruncated = false;
    std::vector<Range> ranges;
  };
  static constexpr uint32_t capacity = 4096, maxRanges = 64;
  bool enabled = false, historyComplete = false;
  uint64_t epoch = 1, enabledAtFrame = 0, nextId = 1, dropped = 0;
  uint32_t writeIndex = 0;
  std::vector<Record> records;
  Record* current = nullptr;
  void reset(uint64_t e) {
    records.clear(); current = nullptr; dropped = 0; writeIndex = 0;
    epoch = e; enabledAtFrame = 0; historyComplete = enabled;
  }
  void enable() {
    if(enabled) return;
    enabled = true; enabledAtFrame = executionCoordinate.frame; historyComplete = false;
  }
  void begin(uint32_t cart, uint32_t dram, uint32_t length, uint64_t pc, uint32_t romSize, uint32_t ramSize, bool linear) {
    if(!enabled) { current = nullptr; return; }
    Record r;
    r.id = nextId++; r.epoch = epoch; r.frame = executionCoordinate.frame; r.pc = pc;
    r.cartAddress = cart; r.dramAddress = dram; r.requestedLength = length; r.ramSize = ramSize; r.writesConfirmed = linear;
    r.fromRom = cart >= 0x10000000u && uint64_t(cart) < 0x10000000ull + romSize;
    r.romOffset = r.fromRom ? cart - 0x10000000u : 0;
    if(records.size() < capacity) { records.push_back(std::move(r)); current = &records.back(); }
    else { records[writeIndex] = std::move(r); current = &records[writeIndex]; writeIndex = (writeIndex + 1) % capacity; ++dropped; }
  }
  void wrote(uint32_t address, uint32_t length) {
    if(!current || !length) return;
    auto& r = *current;
    r.bytesAttempted += length;
    if(r.writesConfirmed) {
      if(address >= r.ramSize) return;
      length = std::min(length, r.ramSize - address); r.bytesWritten += length;
    }
    if(!r.ranges.empty() && uint64_t(r.ranges.back().address) + r.ranges.back().length == address) r.ranges.back().length += length;
    else if(r.ranges.size() < maxRanges) r.ranges.push_back({address, length});
    else r.rangesTruncated = true;
  }
  void end() { current = nullptr; }
};
inline DmaProvenance dmaProvenance;
}
