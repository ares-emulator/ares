#pragma once
#include "execution-service.hpp"
#include <algorithm>
#include <cmath>
#include <optional>

// Selective port of the reference post-mix capture/WAV semantics. All access is
// on the emulation thread. Freeze at the requested cap instead of wrapping.
struct AudioService {
  static constexpr uint32_t rate = 48000, maxFrames = 192000;
  using Clock = std::chrono::steady_clock;
  struct Pin { uint64_t id; std::string owner; std::vector<uint8_t> bytes; Clock::time_point expires; };
  std::string owner;
  bool active = false;
  uint32_t cap = 0;
  uint64_t nextId = 1;
  std::vector<uint8_t> samples;
  std::optional<Pin> pin;
  void reset() { active = false; cap = 0; owner.clear(); samples.clear(); pin.reset(); }
  void releaseOwner(const std::string& who) { if(owner == who) reset(); }
  void push(double left, double right) {
    if(!active || samples.size() / 4 >= cap) return;
    for(double value : {left, right}) {
      int16_t pcm = int16_t((std::isfinite(value) ? std::clamp(value, -1.0, 1.0) : 0) * 32767.0);
      samples.push_back(uint8_t(pcm)); samples.push_back(uint8_t(uint16_t(pcm) >> 8));
    }
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& who, bool available) {
    if(pin && Clock::now() >= pin->expires) pin.reset();
    if(method == "audio.status") {
      if(!args.empty()) return {"invalid_request", {}};
      auto count = active ? samples.size() / 4 : 0;
      return {{}, "{\"available\":" + std::string(available ? "true" : "false") + ",\"active\":" + (active ? "true" : "false")
        + ",\"count\":" + std::to_string(count) + ",\"cap\":" + std::to_string(cap) + ",\"rate\":48000,\"done\":" + (!active || count >= cap ? "true" : "false") + "}"};
    }
    if(!available) return {"unavailable", {}};
    if(active && owner != who) return {"busy", {}};
    if(method == "audio.capture") {
      uint64_t frames;
      if(args.size() != 1 || !ExecutionService::number(args[0], frames) || frames > maxFrames) return {"invalid_request", {}};
      if(pin) return {"busy", {}};
      owner = who; active = true; cap = frames; samples.clear(); samples.reserve(cap * 4);
      return {{}, "{\"capturing\":true,\"cap\":" + std::to_string(cap) + ",\"rate\":48000,\"seconds\":" + std::to_string(double(cap) / rate) + "}"};
    }
    if(method == "audio.begin") {
      if(!args.empty()) return {"invalid_request", {}};
      if(!active) return {"no_capture", {}};
      if(pin) return {"busy", {}};
      pin = Pin{nextId++, who, samples, Clock::now() + std::chrono::seconds(30)};
      return {{}, "{\"snapshot_id\":" + std::to_string(pin->id) + ",\"frames\":" + std::to_string(pin->bytes.size() / 4) + ",\"bytes\":" + std::to_string(pin->bytes.size()) + ",\"rate\":48000}"};
    }
    uint64_t id, offset = 0, length = 0;
    if(method != "audio.read" && method != "audio.release") return {"unavailable", {}};
    if(args.size() != (method == "audio.read" ? 3 : 2) || !ExecutionService::number(args[0], id) ||
      (method == "audio.read" && (!ExecutionService::number(args[1], offset) || !ExecutionService::number(args[2], length) || !length || length > 16384)) ||
      (method == "audio.release" && args[1] != "0" && args[1] != "1")) return {"invalid_request", {}};
    if(!pin || pin->id != id) return {"stale_snapshot", {}};
    if(pin->owner != who) return {"busy", {}};
    if(method == "audio.release") {
      if(args[1] == "1") { active = false; samples.clear(); owner.clear(); }
      pin.reset(); return {{}, "{\"released\":true}"};
    }
    if(offset >= pin->bytes.size() || length > pin->bytes.size() - offset) return {"out_of_bounds", {}};
    std::string hex; hex.reserve(length * 2); static const char digits[] = "0123456789abcdef";
    for(uint64_t n = offset; n < offset + length; ++n) { hex += digits[pin->bytes[n] >> 4]; hex += digits[pin->bytes[n] & 15]; }
    return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"offset\":" + std::to_string(offset) + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
  }
};
