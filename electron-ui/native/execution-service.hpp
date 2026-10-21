#pragma once
#include <chrono>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include "control-result.hpp"

// State-machine semantics adapted from ares-64's desktop-ui/program/debug-session
// at 1c9b9637476a83f7b80d3db1115a0f71529820d9. No legacy transport or UI coupling.
struct ExecutionService {
  using Clock = std::chrono::steady_clock;
  uint64_t frame = 0;
  bool atBoundary = true, supported = false;
  std::string frameKind = "unavailable", executor = "unavailable";
  struct Operation {
    uint64_t id, epoch, start, target, slices = 0;
    std::string owner, kind;
    bool startBoundary;
    Clock::time_point started, deadline;
    uint32_t tapPort = 0, tapButton = 0;
    bool tapKeep = false;
    std::function<std::string()> memoryVerification;
    std::function<std::string(const std::string&)> rspProgress;
    std::function<void(const std::string&)> beforeComplete;
    std::function<std::string()> callProgress;
  };
  std::optional<Operation> active;
  struct Reservation { uint64_t id; std::string owner; Clock::time_point deadline; };
  std::optional<Reservation> reservation;
  // The batch driver lives in main, but exclusivity/deadline live here.
  bool permitted(const std::string& method, const std::string& owner) const {
    if(reservation && owner == reservation->owner) return true;
    if(!reservation && (owner.rfind("batch-", 0) != 0 || method == "execution.batchBegin" || method == "execution.batchCheck" || method == "n64.state.loadAbort")) return true;
    if(owner == "ui" && (method == "session.pause" || method == "session.resume" || method == "session.reset" || method == "execution.cancel")) return true;
    static const std::vector<std::string> reads = {
      "session.status", "memory.regions", "memory.read", "execution.status", "execution.release",
      "n64.capture.control", "n64.capture.rsp", "n64.capture.rdp", "n64.cpu.stack", "n64.render.probe", "n64.dump.read", "n64.dump.release", "n64.cpu.disassemble", "n64.rsp.registers", "n64.rdp.state", "n64.peripheral.registers",
      "n64.memory.read", "n64.debug.registers", "n64.debug.lastStop", "n64.debug.breakList", "n64.debug.watchList",
      "n64.input.state", "n64.profiler.top", "n64.dma.log", "n64.trace.page", "n64.state.format", "n64.state.layout",
      "n64.state.read", "n64.state.release", "video.begin", "video.read", "video.release", "video.record.status",
      "video.record.read", "video.record.release", "audio.status", "audio.read", "audio.release"
    };
    for(auto& read : reads) if(method == read) return true;
    return false;
  }
  uint64_t instructions = 0;
  uint64_t instructionStart = 0, instructionCount = 0;
  bool cpuSupported = false;
  std::function<void()> onStart;
  std::function<void(const std::string&)> onFinish;
  std::function<std::string()> debugSnapshot;
  std::function<std::string()> registerSnapshot;
  uint64_t nextId = 1;
  std::deque<std::pair<uint64_t, std::string>> finished;
  static constexpr uint64_t maxInteger = 9007199254740991ull;

  std::string coordinates() const {
    return "\"frame\":" + std::to_string(frame) + ",\"frame_kind\":\"" + frameKind
      + "\",\"at_frame_boundary\":" + (atBoundary ? "true" : "false")
      + ",\"executor\":\"" + executor + "\",\"operation_id\":" + (active ? std::to_string(active->id) : "null")
      + ",\"batch_id\":" + (reservation ? std::to_string(reservation->id) : "null");
  }
  std::string snapshot(const Operation& op, const std::string& reason = "") const {
    std::string tap;
    if(op.tapPort) {
      static constexpr const char* names[] = {"a", "b", "z", "start", "up", "down", "left", "right", "c_up", "c_down", "c_left", "c_right", "l", "r"};
      bool retained = op.tapKeep && reason != "disconnected" && reason != "host_input" && reason != "shutdown" && reason != "reset";
      tap = ",\"port\":" + std::to_string(op.tapPort) + ",\"button\":" + (std::string("\"") + names[op.tapButton] + "\"")
        + ",\"held_frames\":" + std::to_string(frame - op.start) + ",\"released\":" + (reason.empty() ? "false" : "true")
        + ",\"ownership\":" + std::string(reason.empty() || retained ? "\"injected\"" : "\"host\"");
    }
    return "{\"operation_id\":" + std::to_string(op.id) + ",\"kind\":\"" + op.kind
      + "\",\"owner\":\"" + op.owner + "\",\"state\":\"" + (reason.empty() ? "running" : "done")
      + "\",\"stopped\":" + (reason.empty() ? "false" : "true") + ",\"stop_reason\":\"" + reason
      + "\",\"epoch\":" + std::to_string(op.epoch) + ",\"start_epoch\":" + std::to_string(op.epoch)
      + ",\"start_frame\":" + std::to_string(op.start) + ",\"target_frame\":" + std::to_string(op.target)
      + ",\"frame\":" + std::to_string(frame) + ",\"frame_kind\":\"" + frameKind
      + "\",\"elapsed_frames\":" + std::to_string(frame - op.start)
      + ",\"start_at_frame_boundary\":" + (op.startBoundary ? "true" : "false")
      + ",\"at_frame_boundary\":" + (atBoundary ? "true" : "false")
      + ",\"executor\":\"" + executor + "\",\"slices\":" + std::to_string(op.slices)
      + ",\"elapsed_seconds\":" + std::to_string(std::chrono::duration<double>(Clock::now() - op.started).count()) + ",\"executed\":" + std::to_string(instructions - instructionStart)
      + (op.callProgress ? ",\"call\":" + op.callProgress() : "")
      + (op.rspProgress ? ",\"rsp\":" + op.rspProgress(reason) : "")
      + (!reason.empty() && op.memoryVerification ? ",\"memory_verification\":" + op.memoryVerification() : "")
      + (!reason.empty() && registerSnapshot ? ",\"completion_registers\":" + registerSnapshot() : "")
      + (!reason.empty() && debugSnapshot ? ",\"debug_stop\":" + debugSnapshot() : "") + tap + "}";
  }
  std::string complete(const std::string& reason, bool& paused) {
    if(!active) return "null";
    if(active->beforeComplete) active->beforeComplete(reason);
    auto result = snapshot(*active, reason);
    finished.emplace_back(active->id, result);
    if(finished.size() > 16) finished.pop_front();
    active.reset(); paused = true;
    if(onFinish) onFinish(reason);
    return result;
  }
  void service(bool& paused) {
    if(reservation && Clock::now() >= reservation->deadline) {
      if(active && active->owner == reservation->owner) complete("timeout", paused);
      reservation.reset(); paused = true;
    }
    if(!active) return;
    if(active->kind != "step_instructions" && frame >= active->target) complete(active->kind == "call_function" ? "frame_limit" : "frames_done", paused);
    else if(Clock::now() >= active->deadline) complete("timeout", paused);
  }
  static bool number(const std::string& value, uint64_t& result) {
    if(value.empty() || value.size() > 16 || value.find_first_not_of("0123456789") != std::string::npos) return false;
    try { result = std::stoull(value); return result <= maxInteger; } catch(...) { return false; }
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args,
                        const std::string& owner, uint64_t epoch, bool& paused) {
    if(method == "execution.batchBegin") {
      uint64_t ms;
      if(args.size() != 1 || !number(args[0], ms) || ms < 1 || ms > 600000) return {"invalid_request", {}};
      if(!supported) return {"unavailable", {}};
      if(active || reservation) return {"busy", {}};
      reservation = Reservation{nextId++, owner, Clock::now() + std::chrono::milliseconds(ms)};
      paused = true;
      return {{}, "{\"batch_id\":" + std::to_string(reservation->id) + "}"};
    }
    if(method == "execution.batchCheck") {
      uint64_t id;
      if(args.size() != 1 || !number(args[0], id)) return {"invalid_request", {}};
      if(!reservation || reservation->id != id) return {"batch_interrupted", {}};
      if(reservation->owner != owner) return {"busy", {}};
      return {{}, "{\"active\":true}"};
    }
    if(!permitted(method, owner)) return {"busy", {}};
    if(method == "execution.release") {
      if(!args.empty()) return {"invalid_request", ""};
      if(active && active->owner == owner) complete("disconnected", paused);
      if(reservation && reservation->owner == owner) reservation.reset();
      return {"", "{\"released\":true}"};
    }
    if(method == "execution.status" || method == "execution.cancel") {
      uint64_t id;
      if(args.size() != 1 || !number(args[0], id)) return {"invalid_request", ""};
      if(active && (id == active->id || (id == 0 && method == "execution.cancel"))) {
        if(method == "execution.status") return {"", snapshot(*active)};
        if(owner != "ui" && owner != active->owner) return {"busy", ""};
        return {"", complete("cancelled", paused)};
      }
      for(auto& [fid, result] : finished) if(fid == id) return {"", result};
      return {"unknown_operation", ""};
    }
    bool step = method == "execution.stepInstructions";
    if(!step && method != "execution.runFrames" && method != "execution.runUntilFrame") return {"unavailable", ""};
    uint64_t amount, milliseconds;
    if(args.size() != 2 || !number(args[0], amount) || !number(args[1], milliseconds) || milliseconds < 1 || milliseconds > 600000) return {"invalid_request", ""};
    if(!supported || (step && !cpuSupported)) return {"unavailable", ""};
    if(active) return {"busy", ""};
    bool relative = method == "execution.runFrames";
    if(step && (amount < 1 || amount > 1000000)) return {"invalid_target", ""};
    if((relative && (amount > 1000000 || amount > maxInteger - frame)) || (!relative && !step && amount < frame)) return {"invalid_target", ""};
    auto now = Clock::now();
    active = Operation{nextId++, epoch, frame, step ? maxInteger : relative ? frame + amount : amount, 0,
      owner, step ? "step_instructions" : relative ? "run_frames" : "run_until_frame", atBoundary, now, now + std::chrono::milliseconds(milliseconds)};
    instructionStart = instructions; instructionCount = step ? amount : 0;
    paused = true; // ordinary play stays stopped; the operation alone drives slices
    if(active->target == frame) return {"", complete("frames_done", paused)};
    if(onStart) onStart();
    return {"", snapshot(*active)};
  }
};
