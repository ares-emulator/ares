#pragma once
#include "execution-service.hpp"
#include "n64-service.hpp"

// One most-recent owned trace, paged independently of the operation result.
// Core counters/call decoding derive from ares-64 1c9b9637476a83f7b80d3db1115a0f71529820d9.
struct TraceService {
  bool available = false, completed = false, calls = false;
  uint64_t operationId = 0, epoch = 0, startFrame = 0, endFrame = 0, maxRecords = 0;
  uint64_t uniquePcs = 0, countedInstructions = 0, framesTraced = 0;
  uint32_t registerMask = 0;
  bool f12 = false;
  std::string stopReason;
  void reset() { operationId = 0; completed = false; }
  void finish(const std::string& reason) {
    if(!operationId || completed) return;
    auto& d = ares::Nintendo64::cpu.debugControl;
    d.endTrace();
    std::sort(d.trace.pcRecords.begin(), d.trace.pcRecords.end(), [](const auto& a, const auto& b) { return a.frame != b.frame ? a.frame < b.frame : a.pc < b.pc; });
    std::unordered_map<uint64_t, bool> unique;
    countedInstructions = 0;
    for(auto& r : d.trace.pcRecords) { unique.emplace(r.pc, true); countedInstructions += r.count; }
    uniquePcs = unique.size();
    uint64_t last = std::min<uint64_t>(ares::Nintendo64::executionCoordinate.frame, endFrame);
    framesTraced = last > startFrame ? last - startFrame : 0;
    stopReason = reason; completed = true;
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner, uint64_t e, bool& paused, ExecutionService& execution) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    if(method == "n64.trace.page") return page(args, e);
    if(method != "n64.trace.start") return {"unavailable", {}};
    if(!execution.supported || !execution.cpuSupported) return {"unavailable", {}};
    if(execution.active || nall::GDB::server.hasClient()) return {"busy", {}};
    uint64_t start, frames, requested, milliseconds, low, high, targetLow, targetHigh;
    uint32_t mask;
    if(args.size() != 11 || (args[0] != "pc" && args[0] != "calls") || !ExecutionService::number(args[1], start) || start < execution.frame ||
       !ExecutionService::number(args[2], frames) || !frames || frames > 100000 || start > ExecutionService::maxInteger - frames ||
       !N64Service::address(args[3], low) || !N64Service::address(args[4], high) || low >= high ||
       !N64Service::address(args[5], targetLow) || !N64Service::address(args[6], targetHigh) || targetLow >= targetHigh ||
       !ExecutionService::number(args[7], requested) || !requested || requested > 2000000 || !MemoryService::number(args[8], mask) ||
       (args[9] != "0" && args[9] != "1") || !ExecutionService::number(args[10], milliseconds) || !milliseconds || milliseconds > 600000) return {"invalid_request", {}};
    calls = args[0] == "calls"; maxRecords = std::min<uint64_t>(requested, calls ? 65536 : 262144);
    epoch = e; startFrame = start; endFrame = start + frames; registerMask = mask; f12 = args[9] == "1";
    cpu.debugControl.beginTrace(!calls, calls, startFrame, endFrame, low, high, maxRecords);
    auto& trace = cpu.debugControl.trace;
    trace.targetLow = targetLow; trace.targetHigh = targetHigh; trace.registerMask = mask; trace.captureF12 = f12;
    operationId = execution.nextId; completed = false; stopReason.clear();
    auto result = execution.execute("execution.runUntilFrame", {std::to_string(endFrame), std::to_string(milliseconds)}, owner, e, paused);
    if(!result.error.empty()) { cpu.debugControl.endTrace(); reset(); return result; }
    execution.active->kind = calls ? "trace_calls" : "trace_pc";
    result.json = execution.snapshot(*execution.active);
    return {{}, "{\"operation\":" + result.json + ",\"storage_record_limit\":" + std::to_string(maxRecords) + "}"};
  }
  ControlResult page(const std::vector<std::string>& args, uint64_t e) const {
    using namespace ares::Nintendo64;
    uint64_t id, offset, limit;
    if(args.size() != 3 || !ExecutionService::number(args[0], id) || !id || !ExecutionService::number(args[1], offset) ||
       !ExecutionService::number(args[2], limit) || !limit || limit > 64) return {"invalid_request", {}};
    if(id != operationId || e != epoch) return {"stale_trace", {}};
    if(!completed) return {"busy", {}};
    auto& t = cpu.debugControl.trace;
    uint64_t total = calls ? t.callRecords.size() : t.pcRecords.size();
    if(offset > total) return {"out_of_bounds", {}};
    limit = std::min<uint64_t>(limit, calls ? 32 : 64);
    uint64_t end = std::min(total, offset + limit);
    auto quote = [](uint64_t n) { return controlQuote(N64Service::hex(n)); };
    std::string out = "{\"operation_id\":" + std::to_string(id) + ",\"kind\":\"" + (calls ? "calls" : "pc") + "\",\"epoch\":" + std::to_string(epoch)
      + ",\"start_frame\":" + std::to_string(startFrame) + ",\"end_frame\":" + std::to_string(endFrame) + ",\"stop_reason\":" + controlQuote(stopReason)
      + ",\"record_count\":" + std::to_string(total) + ",\"storage_record_limit\":" + std::to_string(maxRecords) + ",\"offset\":" + std::to_string(offset)
      + ",\"complete\":" + (stopReason == "frames_done" ? "true" : "false") + ",\"truncated\":" + (t.truncated ? "true" : "false")
      + ",\"unique_pcs\":" + std::to_string(uniquePcs) + ",\"counted_instructions\":" + std::to_string(countedInstructions) + ",\"frames_traced\":" + std::to_string(framesTraced)
      + ",\"dropped_records\":" + std::to_string(t.droppedRecords) + ",\"overflow_observations\":" + std::to_string(t.overflowObservations)
      + ",\"traced_instructions\":" + std::to_string(t.totalInstructions) + ",\"instructions_observed\":" + std::to_string(t.instructionsObserved)
      + ",\"calls_observed\":" + std::to_string(t.callsObserved) + ",\"excluded_callsite\":" + std::to_string(t.excludedCallsite)
      + ",\"excluded_target\":" + std::to_string(t.excludedTarget) + ",\"records\":[";
    for(uint64_t i = offset; i < end; ++i) {
      if(i != offset) out += ',';
      if(!calls) {
        auto& r = t.pcRecords[i];
        out += "{\"pc\":" + quote(r.pc) + ",\"frame\":" + std::to_string(r.frame) + ",\"count\":" + std::to_string(r.count) + "}";
      } else {
        auto& r = t.callRecords[i];
        out += "{\"pc\":" + quote(r.pc) + ",\"frame\":" + std::to_string(r.frame) + ",\"sequence\":" + std::to_string(r.sequence) + ",\"target\":" + quote(r.target)
          + ",\"kind\":" + std::to_string(r.kind) + ",\"link_register\":" + std::to_string(r.linkRegister) + ",\"link_value\":" + quote(r.linkValue) + ",\"sp\":" + quote(r.sp) + ",\"registers\":{";
        bool first = true;
        for(unsigned n = 0; n < 32; ++n) if(registerMask & (1u << n)) { if(!first) out += ','; first = false; out += controlQuote(std::to_string(n)) + ':' + quote(r.registers[n]); }
        out += '}';
        if(f12) out += ",\"f12\":" + quote(r.f12) + ",\"fr\":" + (r.fr ? "true" : "false");
        out += '}';
      }
    }
    return {{}, out + "],\"next_offset\":" + (end < total ? std::to_string(end) : "null") + "}"};
  }
};
