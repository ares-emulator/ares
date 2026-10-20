#pragma once
#include "execution-service.hpp"
#include "n64-service.hpp"

// All profiler access occurs at the native emulation safe point.
struct ProfilerService {
  bool available = false;
  std::string owner;
  uint32_t lastOrigin = 0;
  void releaseOwner(const std::string& actor) {
    if(!available || owner != actor) return;
    ares::Nintendo64::cpu.profiler.setEnabled(false); owner.clear();
  }
  void reset() {
    if(available) ares::Nintendo64::cpu.profiler.setEnabled(false);
    owner.clear(); lastOrigin = 0;
  }
  void afterRun() {
    if(!available) return;
    auto origin = uint32_t(ares::Nintendo64::vi.io.dramAddress);
    if(origin && origin != lastOrigin) {
      ares::Nintendo64::cpu.profiler.onFrame(); lastOrigin = origin;
    }
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& actor, bool running) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    auto& p = cpu.profiler;
    if(method != "n64.profiler.top") {
      if(running || nall::GDB::server.hasClient() || (!owner.empty() && owner != actor && actor != "ui")) return {"busy", {}};
      if(method == "n64.profiler.control") {
        if(args.size() != 1 || (args[0] != "0" && args[0] != "1")) return {"invalid_request", {}};
        bool value = args[0] == "1";
        p.setEnabled(value); owner = value ? actor : "";
      } else if(method == "n64.profiler.clear") {
        if(!args.empty()) return {"invalid_request", {}};
        p.clearStats();
      } else return {"unavailable", {}};
      return {{}, "{\"enabled\":" + std::string(p.enabled ? "true" : "false") + ",\"owner\":" + controlQuote(owner) + "}"};
    }
    uint64_t limit;
    if(args.size() != 2 || (args[0] != "continuous" && args[0] != "frame") || !ExecutionService::number(args[1], limit) || !limit || limit > 64) return {"invalid_request", {}};
    auto& source = args[0] == "frame" ? p.frameStats : p.stats;
    std::vector<const CPU::Profiler::FuncStat*> sorted;
    for(auto& entry : source) sorted.push_back(&entry.second);
    std::sort(sorted.begin(), sorted.end(), [](auto* a, auto* b) { return a->exclCycles != b->exclCycles ? a->exclCycles > b->exclCycles : a->addr < b->addr; });
    std::string out = "{\"window\":" + controlQuote(args[0]) + ",\"enabled\":" + (p.enabled ? "true" : "false") + ",\"owner\":" + controlQuote(owner)
      + ",\"timebase_hz\":187500000,\"cycle_unit\":\"master_ticks\",\"frame_window\":\"vi_origin_swaps\",\"symbols_loaded\":false,\"wait_classification\":\"unavailable\""
      + ",\"function_capacity\":65536,\"frame_count\":" + std::to_string(p.frameCount) + ",\"accumulation_frozen\":" + (p.frameCount >= p.maxFrames ? "true" : "false")
      + ",\"dropped_function_updates\":" + std::to_string(p.droppedFunctionUpdates) + ",\"dropped_stack_frames\":" + std::to_string(p.droppedStackFrames)
      + ",\"truncated\":" + ((p.droppedFunctionUpdates || p.droppedStackFrames) ? "true" : "false") + ",\"total_functions\":" + std::to_string(source.size()) + ",\"entries\":[";
    uint64_t count = 0;
    for(auto* s : sorted) {
      if(count >= limit) break;
      for(auto n : {s->callCount, s->inclCycles, s->exclCycles, s->waitCycles, s->exclBytesIn, s->exclBytesOut}) if(n > ExecutionService::maxInteger) return {"counter_overflow", {}};
      if(count++) out += ',';
      out += "{\"addr\":" + controlQuote(N64Service::hex(s->addr)) + ",\"name\":" + controlQuote(std::string(p.labelFor(s->addr).data()))
        + ",\"callCount\":" + std::to_string(s->callCount) + ",\"inclCycles\":" + std::to_string(s->inclCycles) + ",\"exclCycles\":" + std::to_string(s->exclCycles)
        + ",\"waitCycles\":" + std::to_string(s->waitCycles) + ",\"exclBytesIn\":" + std::to_string(s->exclBytesIn) + ",\"exclBytesOut\":" + std::to_string(s->exclBytesOut) + "}";
    }
    return {{}, out + "],\"more\":" + (source.size() > count ? "true" : "false") + "}"};
  }
};
