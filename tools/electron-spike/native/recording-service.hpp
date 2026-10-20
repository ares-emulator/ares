#pragma once
// Adapted from ares-64 desktop-ui/program/video-recording.hpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. Native stores bounded bottom-up BGR;
// Electron main streams AVI publication. Render callbacks never read core state.
#include "execution-service.hpp"
#include <mutex>
struct RecordingService {
  using Clock = std::chrono::steady_clock;
  std::mutex mutex;
  bool active = false, available = false;
  std::string owner, reason, frameKind = "unavailable";
  uint64_t epoch = 0, startFrame = 0, endFrame = 0, nextId = 1, pin = 0;
  uint64_t maxFrames = 600, maxBytes = 128ull << 20, fps = 60, width = 0, height = 0;
  Clock::time_point deadline, pinDeadline;
  std::vector<std::vector<uint8_t>> frames;
  void finish(const std::string& why) { if(active) { active = false; reason = why; } }
  void expire() { if(active && Clock::now() >= deadline) finish("seconds_limit"); if(pin && Clock::now() >= pinDeadline) pin = 0; }
  void clear() { active = available = false; owner.clear(); reason.clear(); frames.clear(); pin = 0; width = height = epoch = startFrame = endFrame = 0; }
  void reset() { std::lock_guard lock(mutex); clear(); }
  void releaseOwner(const std::string& who) { std::lock_guard lock(mutex); if(owner == who) clear(); }
  void capture(const uint8_t* rgba, uint32_t w, uint32_t h, uint64_t e, uint64_t f, const std::string& kind = "unavailable") {
    std::lock_guard lock(mutex); expire(); if(!active) return;
    if(e != epoch) { finish("epoch_changed"); return; }
    if(f < startFrame || w <= 1 || h <= 1) return;
    if(width && (w != width || h != height)) { finish("dimensions_changed"); return; }
    uint64_t stride = (uint64_t(w) * 3 + 3) & ~3ull, size = stride * h;
    if(w > 4096 || h > 4096 || size > maxBytes || frames.size() >= maxBytes / size) { finish("bytes_limit"); return; }
    width = w; height = h; std::vector<uint8_t> bytes(size);
    for(uint32_t y = 0; y < h; ++y) for(uint32_t x = 0; x < w; ++x) {
      auto* in = rgba + (uint64_t(y) * w + x) * 4; auto* out = bytes.data() + (h - 1 - y) * stride + x * 3;
      out[0] = in[2]; out[1] = in[1]; out[2] = in[0];
    }
    if(frames.empty()) startFrame = f; endFrame = f; frames.push_back(std::move(bytes));
    if(frames.size() >= maxFrames) finish("frames_limit");
  }
  std::string status() {
    return "{\"active\":" + std::string(active ? "true" : "false") + ",\"available\":" + (available ? "true" : "false")
      + ",\"frames\":" + std::to_string(frames.size()) + ",\"fps\":" + std::to_string(fps) + ",\"width\":" + std::to_string(width) + ",\"height\":" + std::to_string(height)
      + ",\"epoch\":" + std::to_string(epoch) + ",\"start_frame\":" + std::to_string(startFrame) + ",\"end_frame\":" + std::to_string(endFrame)
      + ",\"frame_kind\":" + controlQuote(frameKind) + ",\"stop_reason\":" + controlQuote(reason) + ",\"format\":\"avi_raw_bgr24\",\"audio\":false,\"source\":\"presented\",\"max_frames\":" + std::to_string(maxFrames)
      + ",\"max_bytes\":" + std::to_string(maxBytes) + ",\"captured_bytes\":" + std::to_string(frames.empty() ? 0 : frames.size() * frames[0].size()) + "}";
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& who, uint64_t e, uint64_t f, const std::string& kind = "unavailable") {
    std::lock_guard lock(mutex); expire();
    if(method == "video.record.status") { if(!args.empty()) return {"invalid_request", {}}; return {{}, status()}; }
    if(available && owner != who) return {"busy", {}};
    if(method == "video.record.start") {
      uint64_t count, bytes, rate, ms;
      if(args.size() != 5 || !ExecutionService::number(args[0], count) || !count || count > 36000 || !ExecutionService::number(args[1], bytes) || bytes < (1 << 20) || bytes > (256ull << 20) ||
        !ExecutionService::number(args[2], rate) || !rate || rate > 120 || !ExecutionService::number(args[3], ms) || !ms || ms > 3600000 || (args[4] != "0" && args[4] != "1")) return {"invalid_request", {}};
      if(pin) return {"busy", {}};
      if(active || (available && args[4] != "1")) return {"recording_exists", {}};
      clear(); owner = who; active = available = true; maxFrames = count; maxBytes = bytes; fps = rate;
      epoch = e; frameKind = kind; startFrame = endFrame = f; deadline = Clock::now() + std::chrono::milliseconds(ms);
      return {{}, status()};
    }
    if(!available) return {"no_recording", {}};
    if(method == "video.record.begin") {
      if(!args.empty()) return {"invalid_request", {}};
      if(pin) return {"busy", {}};
      finish("requested"); if(frames.empty()) return {"no_frames", {}};
      pin = nextId++; pinDeadline = Clock::now() + std::chrono::seconds(30);
      auto json = status(); json.pop_back(); return {{}, json + ",\"snapshot_id\":" + std::to_string(pin) + "}"};
    }
    uint64_t id, offset = 0, length = 0;
    if(method != "video.record.read" && method != "video.record.release") return {"unavailable", {}};
    if(args.size() != (method == "video.record.read" ? 3 : 2) || !ExecutionService::number(args[0], id) ||
      (method == "video.record.read" && (!ExecutionService::number(args[1], offset) || !ExecutionService::number(args[2], length) || !length || length > 16384)) ||
      (method == "video.record.release" && args[1] != "0" && args[1] != "1")) return {"invalid_request", {}};
    if(!pin || id != pin) return {"stale_snapshot", {}};
    if(method == "video.record.release") { pin = 0; if(args[1] == "1") { available = false; frames.clear(); } return {{}, "{\"released\":true}"}; }
    uint64_t frameBytes = frames[0].size(), total = frames.size() * frameBytes;
    if(offset >= total || length > total - offset) return {"out_of_bounds", {}};
    std::string hex; hex.reserve(length * 2); static const char digits[] = "0123456789abcdef";
    for(uint64_t n = offset; n < offset + length; ++n) { auto byte = frames[n / frameBytes][n % frameBytes]; hex += digits[byte >> 4]; hex += digits[byte & 15]; }
    return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"offset\":" + std::to_string(offset) + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
  }
};
