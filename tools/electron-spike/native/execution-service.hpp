#pragma once
#include <chrono>
#include <deque>
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
  };
  std::optional<Operation> active;
  uint64_t nextId = 1;
  std::deque<std::pair<uint64_t, std::string>> finished;
  static constexpr uint64_t maxInteger = 9007199254740991ull;

  std::string coordinates() const {
    return "\"frame\":" + std::to_string(frame) + ",\"frame_kind\":\"" + frameKind
      + "\",\"at_frame_boundary\":" + (atBoundary ? "true" : "false")
      + ",\"executor\":\"" + executor + "\",\"operation_id\":" + (active ? std::to_string(active->id) : "null");
  }
  std::string snapshot(const Operation& op, const std::string& reason = "") const {
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
      + ",\"elapsed_seconds\":" + std::to_string(std::chrono::duration<double>(Clock::now() - op.started).count()) + "}";
  }
  std::string complete(const std::string& reason, bool& paused) {
    if(!active) return "null";
    auto result = snapshot(*active, reason);
    finished.emplace_back(active->id, result);
    if(finished.size() > 16) finished.pop_front();
    active.reset(); paused = true;
    return result;
  }
  void service(bool& paused) {
    if(!active) return;
    if(frame >= active->target) complete("frames_done", paused);
    else if(Clock::now() >= active->deadline) complete("timeout", paused);
  }
  static bool number(const std::string& value, uint64_t& result) {
    if(value.empty() || value.size() > 16 || value.find_first_not_of("0123456789") != std::string::npos) return false;
    try { result = std::stoull(value); return result <= maxInteger; } catch(...) { return false; }
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args,
                        const std::string& owner, uint64_t epoch, bool& paused) {
    if(method == "execution.release") {
      if(!args.empty()) return {"invalid_request", ""};
      if(active && active->owner == owner) complete("disconnected", paused);
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
    if(method != "execution.runFrames" && method != "execution.runUntilFrame") return {"unavailable", ""};
    uint64_t amount, milliseconds;
    if(args.size() != 2 || !number(args[0], amount) || !number(args[1], milliseconds) || milliseconds < 1 || milliseconds > 600000) return {"invalid_request", ""};
    if(!supported) return {"unavailable", ""};
    if(active) return {"busy", ""};
    bool relative = method == "execution.runFrames";
    if((relative && (amount > 1000000 || amount > maxInteger - frame)) || (!relative && amount < frame)) return {"invalid_target", ""};
    auto now = Clock::now();
    active = Operation{nextId++, epoch, frame, relative ? frame + amount : amount, 0,
      owner, relative ? "run_frames" : "run_until_frame", atBoundary, now, now + std::chrono::milliseconds(milliseconds)};
    paused = true; // ordinary play stays stopped; the operation alone drives slices
    if(active->target == frame) return {"", complete("frames_done", paused)};
    return {"", snapshot(*active)};
  }
};
