#pragma once
#include <cstdint>

// Minimal VI coordinate/slice hooks adapted from ares-64 bdcadf33d and
// 1c9b9637476a83f7b80d3db1115a0f71529820d9 (debug-session.hpp / CPU::main).
// Host-only state: never serialized. Active scanout boundaries include fields;
// inactive VI liveness yields do not advance this coordinate.
namespace ares::Nintendo64 {
struct ExecutionCoordinate {
  uint64_t frame = 0;
  bool atBoundary = true, boundaryLatched = false, livenessYield = false;
  uint32_t sliceBudget = 0;
  auto boundary() -> void { ++frame; boundaryLatched = true; }
  auto reset() -> void { frame = 0; atBoundary = true; boundaryLatched = false; livenessYield = false; }
};
inline ExecutionCoordinate executionCoordinate;
}
