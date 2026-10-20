#pragma once
#include "n64-service.hpp"
#include "execution-service.hpp"

// A single native safe point validates the entire set before starting or writing.
// This is prevalidation, not rollback: overlapping writes retain their order.
struct MemoryExperiment {
  using Writes = std::vector<std::vector<std::string>>;
  static std::string readbacks(const Writes& writes) {
    std::string out = "{\"readbacks\":[", matches = ",\"matches\":[";
    for(auto& w : writes) {
      auto read = N64Service::memory("n64.memory.read", {w[0], w[1], std::to_string(w[2].size() / 2)}, false);
      std::string expected = w[2];
      for(auto& c : expected) if(c >= 'A' && c <= 'F') c += 'a' - 'A';
      if(out.back() != '[') { out += ','; matches += ','; }
      out += read.json;
      matches += read.error.empty() && read.json.find("\"hex\":\"" + expected + "\"") != std::string::npos ? "true" : "false";
    }
    return out + ']' + matches + "]}";
  }
  static ControlResult execute(const std::vector<std::string>& args, const std::string& owner,
      uint64_t epoch, bool available, ExecutionService& execution, bool& paused) {
    if(!available) return {"unavailable", {}};
    if(execution.active || nall::GDB::server.hasClient()) return {"busy", {}};
    uint64_t frames, ms;
    if(args.size() < 5 || (args.size() - 2) % 3 || args.size() > 98
      || !ExecutionService::number(args[0], frames) || frames > 100000
      || !ExecutionService::number(args[1], ms) || ms < 1 || ms > 600000) return {"invalid_request", {}};
    if(frames && (!execution.supported || frames > ExecutionService::maxInteger - execution.frame)) return {"invalid_target", {}};
    Writes writes; size_t bytes = 0;
    for(size_t i = 2; i < args.size(); i += 3) {
      std::vector<std::string> w{args[i], args[i + 1], args[i + 2]};
      auto checked = N64Service::memory("n64.memory.write", w, false, true);
      if(!checked.error.empty()) return checked;
      bytes += w[2].size() / 2;
      if(bytes > 1024) return {"invalid_request", {}};
      writes.push_back(std::move(w));
    }
    ares::Nintendo64::system.prepareVideo();
    if(!ares::Nintendo64::rdram.hidden.data) return {"renderer_unavailable", {}};
    if(frames) {
      auto started = execution.execute("execution.runFrames", {args[0], args[1]}, owner, epoch, paused);
      if(!started.error.empty()) return started;
      execution.active->kind = "verify_writes";
      execution.active->memoryVerification = [writes] { return readbacks(writes); };
    }
    std::string out = "{\"writes\":[";
    for(auto& w : writes) {
      if(out.back() != '[') out += ',';
      out += N64Service::memory("n64.memory.write", w, false).json;
    }
    return {{}, out + "],\"immediate_verification\":" + readbacks(writes)
      + ",\"side_effects_rolled_back\":false,\"operation\":" + (frames ? execution.snapshot(*execution.active) : "null") + "}"};
  }
};
