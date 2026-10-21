#pragma once
#include "execution-service.hpp"
#include "input-service.hpp"
#include <nall/gdb/server.hpp>

// A tap is one owned native run, never a host-side sleep or a pair of input RPCs.
struct TapService {
  uint32_t port = 0;
  std::string owner;
  bool keep = false;
  bool active() const { return port != 0; }
  static bool retains(const std::string& reason) {
    return reason != "disconnected" && reason != "host_input" && reason != "shutdown" && reason != "reset";
  }
  void finish(const std::string& reason, InputService& input, uint64_t epoch, uint64_t frame) {
    if(!active()) return;
    auto& pending = input.pending[port - 1]; auto& latched = input.latched[port - 1];
    // Human takeover and generation/epoch revocation must never reacquire a hold.
    if(keep && retains(reason) && pending.owner == owner) {
      pending = {owner, 0, 0, 0, epoch}; latched = pending;
    } else {
      if(pending.owner == owner) pending = {};
      if(latched.owner == owner) latched = {};
    }
    port = 0; owner.clear(); keep = false;
  }
  ControlResult execute(const std::vector<std::string>& args, const std::string& actor, uint64_t epoch, bool& paused, InputService& input, ExecutionService& execution) {
    if(!input.available || !execution.supported) return {"unavailable", {}};
    if(execution.active || nall::GDB::server.hasClient()) return {"busy", {}};
    uint64_t p, button, frames, milliseconds;
    if(args.size() != 5 || !ExecutionService::number(args[0], p) || p < 1 || p > 4 || !ExecutionService::number(args[1], button) || button > 13 ||
       !ExecutionService::number(args[2], frames) || !frames || frames > 100000 || frames > ExecutionService::maxInteger - execution.frame ||
       !ExecutionService::number(args[3], milliseconds) || !milliseconds || milliseconds > 600000 || (args[4] != "0" && args[4] != "1")) return {"invalid_request", {}};
    auto result = input.execute("n64.input.set", {args[0], std::to_string(1u << button), "0", "0", "0", "0"}, actor, epoch, execution.frame, "");
    if(!result.error.empty()) return result;
    port = p; owner = actor; keep = args[4] == "1";
    result = execution.execute("execution.runFrames", {args[2], args[3]}, actor, epoch, paused);
    if(!result.error.empty()) { finish("disconnected", input, epoch, execution.frame); return result; }
    execution.active->kind = "controller_tap";
    execution.active->tapPort = port; execution.active->tapButton = button; execution.active->tapKeep = keep;
    return {{}, execution.snapshot(*execution.active)};
  }
};
