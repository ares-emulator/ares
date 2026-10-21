#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include <functional>
#include <string>
namespace ares::Nintendo64 {
// Host-only command viewer state. Adapted from ares-64 RSPCapture/RDPCapture and
// Program::n64AfterRun at 1c9b9637476a83f7b80d3db1115a0f71529820d9.
// Live records are copied at real dispatch/decoder hooks, committed on nonzero
// VI-origin changes, and double-buffered so the committed frame cannot be reused.
struct CommandCapture {
  static constexpr uint32_t Capacity = 4096;
  struct RspRecord {
    uint64_t seq = 0, frame = 0, cycle = 0, bytesIn = 0, bytesOut = 0;
    uint32_t word0 = 0, word1 = 0;
    uint8_t opcode = 0;
    bool overhead = false;
    uint8_t overlayId = 0, wordCount = 0, segmentType = 0;
    std::array<uint32_t, 16> words{};
    std::string overlay, command, args;
    bool wordsTruncated = false;
  };
  struct RdpRecord { uint64_t seq, frame, word0, word1; uint8_t opcode, wordCount; };
  static constexpr uint8_t RdpWords[64] = {
    1,1,1,1,1,1,1,1,4,6,12,14,12,14,20,22,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,2,2,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  };
  bool rspEnabled = false, rdpEnabled = false, segmentOpen = false;
  int32_t sigPc = -1, loopPc = -1, dispatchPc = -1;
  struct QueueConfig {
    bool loaded = false;
    std::array<uint32_t, 5> hooks{}; // command, loop, overlay-load, fetch, fetch-with-ptr
    std::array<std::array<uint32_t, 2>, 5> signature{};
    uint32_t cmdsOffset = 0xa0, cmdsSize = 0x100;
  } queue;
  struct DisplayConfig {
    std::array<uint32_t, 4> signature{0x00196603, 0x000c5840};
    uint8_t count = 2, word0Reg = 25, word1Reg = 24, opcodeShift = 24;
    int32_t loopOffset = -12, dispatchOffset = 20;
  } display;
  bool queueMode = false;
  std::function<void(RspRecord&)> annotate;
  uint32_t lastOrigin = 0;
  uint64_t sequence = 0, presentedFrame = 0, generation = 1, segmentStart = 0;
  uint64_t rspObserved = 0, rdpObserved = 0, rspCommitted = 0, rdpCommitted = 0;
  RspRecord segment;
  std::vector<RspRecord> rspLive, rspLatest;
  std::vector<RdpRecord> rdpLive, rdpLatest;
  auto reset() -> void { *this = {}; }
  auto detectRsp() -> void;
  auto rspInstruction(uint32_t pc) -> void;
  auto rspFlush() -> void;
  auto rspDma(uint32_t bytes, bool toRdram) -> void;
  auto rdpCommand(uint8_t opcode, uint64_t word0, uint64_t word1, uint8_t words) -> void;
  auto afterRun() -> void;
  static auto rspName(uint8_t opcode) -> const char*;
  static auto rdpName(uint8_t opcode) -> const char*;
};
inline CommandCapture commandCapture;
}
