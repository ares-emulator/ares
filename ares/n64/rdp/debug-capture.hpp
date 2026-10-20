// Submission-time capture adapted from ares-64 rdp.hpp/debug-capture.cpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. Host-only; never serialized.
struct RDPDebugCapture {
  struct Word { u64 value; u32 address, batch; };
  struct Batch {
    u32 id = 0, dpcStart = 0, current = 0, end = 0, words = 0, firstWord = 0, capturedWords = 0;
    u64 cpuPc = 0;
    u32 rspPc = 0;
    s32 task = -1;
    bool source = false, rspRunning = false;
  };
  struct Task {
    u32 id = 0;
    u64 cpuPc = 0, dmemHash = 0, imemHash = 0;
    u8 header[64] = {};
    std::vector<u8> dmem;
  };
  static constexpr u32 MaxWords = 65536, MaxBatches = 512, MaxTasks = 64;
  bool armed = false, keepDmem = false, truncated = false;
  u64 epoch = 0, frame = 0, droppedWords = 0, droppedBatches = 0, droppedTasks = 0, batchesOutsideInterval = 0;
  u32 maxWords = MaxWords;
  s32 lastTask = -1;
  std::vector<Word> words;
  std::vector<Batch> batches;
  std::vector<Task> tasks;
  auto arm(u64 epoch_, u64 frame_, u32 limit, bool dmem) -> void {
    *this = {}; armed = true; epoch = epoch_; frame = frame_;
    maxWords = std::min(limit, MaxWords); keepDmem = dmem;
  }
  auto disarm() -> void { armed = false; }
  auto inInterval() const -> bool { return armed && executionCoordinate.frame == frame; }
  auto onRspStart(u64 pc) -> void;
  auto onBatch(u32 start, u32 current, u32 end, bool source, u64 pc) -> void;
};
