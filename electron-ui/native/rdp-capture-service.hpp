#pragma once
#include "execution-service.hpp"
#include "n64-service.hpp"
#include "machine-service.hpp"
struct RdpCaptureService {
  uint64_t operationId = 0, epoch = 0;
  std::string owner, stopReason, startPoint = "null", endPoint = "null", rom = "null", renderer = "none";
  std::function<std::string()> romIdentity;
  // Reference machine-point fields, frozen at arm/completion rather than export.
  static std::string point(uint64_t epoch, uint64_t frame) {
    using namespace ares::Nintendo64;
    MachineService::Object out, v;
    auto& io = vi.io;
    v.number("VI_CTRL.colorDepth", io.colorDepth); v.number("VI_CTRL.serrate", io.serrate); v.number("VI_CTRL.antialias", io.antialias);
    v.hex("VI_ORIGIN", io.dramAddress); v.number("VI_WIDTH", io.width); v.number("VI_V_CURRENT", io.vcounter);
    v.number("VI_X_SCALE", io.xscale); v.number("VI_Y_SCALE", io.yscale); v.number("VI_H_START", io.hstart); v.number("VI_H_END", io.hend);
    v.number("VI_V_START", io.vstart); v.number("VI_V_END", io.vend); v.number("field", io.field);
    out.hex("pc", cpu.ipu.pc); out.number("frame", frame); out.number("epoch", epoch); out.add("vi", v.end()); out.add("dpc", MachineService::rdpState()); return out.end();
  }
  bool completed = false, intervalComplete = false;
  void reset() { operationId = 0; owner.clear(); completed = false; ares::Nintendo64::rdp.debugCapture = {}; }
  void releaseOwner(const std::string& actor) { if(owner == actor) reset(); }
  void finish(const std::string& reason) {
    if(!operationId || completed) return;
    auto& c = ares::Nintendo64::rdp.debugCapture;
    intervalComplete = reason == "frames_done" && ares::Nintendo64::executionCoordinate.frame == c.frame + 1;
    endPoint = point(epoch, ares::Nintendo64::executionCoordinate.frame);
    c.disarm(); stopReason = reason; completed = true;
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& actor, uint64_t e, bool available, bool& paused, ExecutionService& execution) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    if(method == "n64.rdp.capturePage") return page(args, e);
    if(method != "n64.rdp.captureStart") return {"unavailable", {}};
    uint64_t limit, ms;
    if(args.size() != 3 || !ExecutionService::number(args[0], limit) || !limit || limit > RDPDebugCapture::MaxWords ||
       (args[1] != "0" && args[1] != "1") || !ExecutionService::number(args[2], ms) || !ms || ms > 600000) return {"invalid_request", {}};
    if(execution.active || execution.reservation || nall::GDB::server.hasClient()) return {"busy", {}};
    if(!execution.atBoundary) return {"not_at_frame_boundary", {}};
    auto result = execution.execute("execution.runFrames", {"1", std::to_string(ms)}, actor, e, paused);
    if(!result.error.empty()) return result;
    operationId = execution.active->id; epoch = e; owner = actor; completed = false; stopReason.clear();
    startPoint = point(e, execution.frame); endPoint = "null"; rom = romIdentity ? romIdentity() : "null";
    renderer = vulkan.enable ? "paraLLEl-RDP" : "software";
    rdp.debugCapture.arm(e, execution.frame, limit, args[1] == "1");
    execution.active->kind = "rdp_capture";
    return {{}, execution.snapshot(*execution.active)};
  }
  ControlResult page(const std::vector<std::string>& args, uint64_t e) const {
    using namespace ares::Nintendo64;
    uint64_t id, offset, limit;
    if(args.size() != 4 || !ExecutionService::number(args[0], id) || !id ||
       (args[1] != "words" && args[1] != "batches" && args[1] != "tasks") || !ExecutionService::number(args[2], offset) ||
       !ExecutionService::number(args[3], limit) || !limit || limit > 64) return {"invalid_request", {}};
    if(id != operationId || e != epoch) return {"stale_capture", {}};
    if(!completed) return {"busy", {}};
    auto& c = rdp.debugCapture;
    bool words = args[1] == "words", batches = args[1] == "batches";
    uint64_t total = words ? c.words.size() : batches ? c.batches.size() : c.tasks.size();
    if(offset > total) return {"out_of_bounds", {}};
    limit = std::min<uint64_t>(limit, words ? 64 : batches ? 16 : 1);
    auto end = std::min(total, offset + limit);
    auto quote = [](uint64_t value) { return controlQuote(N64Service::hex(value)); };
    auto bytes = [](const auto& data) { static const char* h = "0123456789abcdef"; std::string s; for(auto b : data) { s += h[(uint8_t)b >> 4]; s += h[(uint8_t)b & 15]; } return controlQuote(s); };
    std::string out = "{\"operation_id\":" + std::to_string(id) + ",\"epoch\":" + std::to_string(epoch) + ",\"frame\":" + std::to_string(c.frame)
      + ",\"rom\":" + rom + ",\"renderer\":" + controlQuote(renderer) + ",\"start\":" + startPoint + ",\"end\":" + endPoint + ",\"rdram_size\":" + std::to_string(rdram.ram.size)
      + ",\"kind\":" + controlQuote(args[1]) + ",\"stop_reason\":" + controlQuote(stopReason)
      + ",\"complete\":" + (intervalComplete ? "true" : "false")
      + ",\"truncated\":" + (c.truncated ? "true" : "false") + ",\"dropped_words\":" + std::to_string(c.droppedWords)
      + ",\"dropped_batches\":" + std::to_string(c.droppedBatches) + ",\"dropped_tasks\":" + std::to_string(c.droppedTasks)
      + ",\"batches_outside_interval\":" + std::to_string(c.batchesOutsideInterval)
      + ",\"word_count\":" + std::to_string(c.words.size()) + ",\"batch_count\":" + std::to_string(c.batches.size()) + ",\"task_count\":" + std::to_string(c.tasks.size())
      + ",\"storage_word_limit\":" + std::to_string(c.maxWords) + ",\"storage_batch_limit\":512,\"storage_task_limit\":64,\"dmem_snapshots\":" + (c.keepDmem ? "true" : "false")
      + ",\"offset\":" + std::to_string(offset) + ",\"total\":" + std::to_string(total) + ",\"records\":[";
    for(uint64_t i = offset; i < end; ++i) {
      if(i != offset) out += ',';
      if(words) {
        auto& w = c.words[i]; out += "{\"value\":" + quote(w.value) + ",\"address\":" + quote(w.address) + ",\"batch\":" + std::to_string(w.batch) + "}";
      } else if(batches) {
        auto& b = c.batches[i]; out += "{\"id\":" + std::to_string(b.id) + ",\"source\":" + controlQuote(b.source ? "DMEM" : "RDRAM")
          + ",\"dpc_start\":" + quote(b.dpcStart) + ",\"current\":" + quote(b.current) + ",\"end\":" + quote(b.end)
          + ",\"words\":" + std::to_string(b.words) + ",\"first_word\":" + std::to_string(b.firstWord) + ",\"captured_words\":" + std::to_string(b.capturedWords)
          + ",\"cpu_pc\":" + quote(b.cpuPc) + ",\"rsp_pc\":" + quote(b.rspPc) + ",\"rsp_running\":" + (b.rspRunning ? "true" : "false") + ",\"task\":" + std::to_string(b.task) + "}";
      } else {
        auto& t = c.tasks[i]; out += "{\"id\":" + std::to_string(t.id) + ",\"cpu_pc\":" + quote(t.cpuPc) + ",\"header\":" + bytes(t.header)
          + ",\"dmem_hash_xxh64\":" + quote(t.dmemHash) + ",\"imem_hash_xxh64\":" + quote(t.imemHash) + ",\"dmem\":" + (t.dmem.empty() ? "null" : bytes(t.dmem)) + "}";
      }
    }
    return {{}, out + "],\"next_offset\":" + (end < total ? std::to_string(end) : "null") + "}"};
  }
};
