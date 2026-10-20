#pragma once
#include "rsp-debug-state.hpp"
#include "execution-service.hpp"
struct RspDebugService {
  using Clock = ExecutionService::Clock;
  std::string owner;
  Clock::time_point expires;
  bool active() const { return ares::Nintendo64::rsp.debugControl.active; }
  void reset() { ares::Nintendo64::rsp.debugControl = {}; owner.clear(); }
  void releaseOwner(const std::string& actor) { if(owner == actor) reset(); }
  void observe(const std::string& actor) { if(active() && owner == actor) expires = Clock::now() + std::chrono::seconds(30); }
  void finish(const std::string& reason) { if(active()) ares::Nintendo64::rsp.debugControl.pause(ares::Nintendo64::rsp.ipu.pc, reason); }
  void service(ExecutionService& execution, bool& paused) {
    if(active() && (Clock::now() >= expires || nall::GDB::server.hasClient())) {
      execution.complete(nall::GDB::server.hasClient() ? "gdb_attached" : "rsp_lease_expired", paused); reset(); paused = true;
    }
  }
  bool permitted(const std::string& method, const std::string& actor, bool running) const {
    if(!active()) return true;
    if(method == "n64.rsp.registers" || method.rfind("n64.rsp.", 0) == 0) return true;
    if((actor == owner || actor == "ui") && (method == "session.pause" || method == "session.resume" || method == "session.reset" || method == "execution.cancel")) return true;
    if(actor == owner && !running && (method == "memory.write" || method == "n64.memory.write")) return true;
    static const std::vector<std::string> reads = {"session.status", "memory.regions", "memory.read", "execution.status", "execution.release", "n64.memory.read", "n64.cpu.disassemble", "n64.debug.registers", "n64.debug.lastStop", "n64.debug.breakList", "n64.debug.watchList", "n64.rdp.state", "n64.peripheral.registers", "n64.input.state", "n64.profiler.top", "n64.trace.page", "n64.dma.log", "n64.state.format", "n64.state.layout", "video.begin", "video.read", "video.release", "audio.status", "audio.read", "audio.release"};
    for(auto& read : reads) if(method == read) return true;
    return false;
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& actor, uint64_t epoch, bool available, ExecutionService& execution, bool& paused) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    auto& d = rsp.debugControl;
    if(method == "n64.rsp.debug") {
      if(args.size() != 1 || (args[0] != "0" && args[0] != "1")) return {"invalid_request", {}};
      bool enable = args[0] == "1";
      if(active() && owner != actor && actor != "ui") return {"busy", {}};
      if(enable && !active()) {
        if(execution.active || execution.reservation || nall::GDB::server.hasClient() || !cpu.debugControl.breakpoints.empty() || !cpu.debugControl.watchpoints.empty() || (cpu.debugControl.trace.pcs || cpu.debugControl.trace.calls)) return {"busy", {}};
        d = {}; d.active = true; d.pause(rsp.ipu.pc, "rsp_enabled"); owner = actor; observe(actor);
        cpu.debugControl.resume(); paused = true;
      } else if(!enable && active()) {
        if(execution.active) execution.complete("rsp_disabled", paused);
        d.active = d.stopped = d.runToBreakpoint = d.skipBreakpoint = false; d.remaining = 0; d.reason = "disabled"; owner.clear();
      }
      observe(actor); execution.executor = N64Service::executor(); return {{}, rspDebugState()};
    }
    if(method == "n64.rsp.break" && args == std::vector<std::string>{"list"}) {
      observe(actor); return {{}, "{\"breakpoints\":" + breakpointList() + "}"};
    }
    if(!active()) return {"not_active", {}};
    if(owner != actor && actor != "ui") return {"busy", {}};
    observe(actor);
    if(method == "n64.rsp.break") {
      if(args.empty()) return {"invalid_request", {}};
      if(args == std::vector<std::string>{"clear"}) d.breakpoints.clear();
      else {
        uint64_t pc;
        if(args.size() != 2 || (args[0] != "add" && args[0] != "remove") || !N64Service::address(args[1], pc) || pc > 0xfff || (pc & 3)) return {"invalid_request", {}};
        auto at = std::find(d.breakpoints.begin(), d.breakpoints.end(), pc);
        if(args[0] == "remove") { if(at != d.breakpoints.end()) d.breakpoints.erase(at); }
        else if(at == d.breakpoints.end()) {
          if(d.breakpoints.size() == RSP::DebugControl::MaxBreakpoints) return {"table_full", {}};
          d.breakpoints.push_back(pc);
        }
      }
      return {{}, "{\"breakpoints\":" + breakpointList() + "}"};
    }
    if(method == "n64.rsp.stop") {
      if(!args.empty()) return {"invalid_request", {}};
      if(execution.active) execution.complete("rsp_stop", paused);
      d.pause(rsp.ipu.pc, "rsp_stop"); paused = true;
      return {{}, "{\"stopRequested\":true,\"debug\":" + rspDebugState() + "}"};
    }
    bool step = method == "n64.rsp.step"; uint64_t count = 0;
    if(!step && method != "n64.rsp.run") return {"unavailable", {}};
    if((step && (args.size() != 1 || !ExecutionService::number(args[0], count) || !count || count > 1000000)) || (!step && !args.empty())) return {"invalid_request", {}};
    if(execution.active || execution.reservation || nall::GDB::server.hasClient()) return {"busy", {}};
    if(execution.frame == ExecutionService::maxInteger) return {"invalid_target", {}};
    auto start = d.instructions; auto pc = rsp.ipu.pc & 0xfff;
    auto result = execution.execute("execution.runUntilFrame", {std::to_string(ExecutionService::maxInteger), "5000"}, owner, epoch, paused);
    if(!result.error.empty()) return result;
    execution.active->kind = step ? "rsp_step" : "rsp_run";
    execution.active->rspProgress = [start, pc, count](const std::string& reason) {
      auto& rsp = ares::Nintendo64::rsp;
      return "{\"start_pc\":" + controlQuote(N64Service::hex(pc)) + ",\"pc\":" + controlQuote(N64Service::hex(rsp.ipu.pc & 0xfff)) + ",\"executed\":" + std::to_string(rsp.debugControl.instructions - start)
        + ",\"requested\":" + std::to_string(count) + ",\"hardware_halted\":" + (rsp.status.halted ? "true" : "false") + ",\"stop_reason\":" + controlQuote(reason.empty() ? rsp.debugControl.reason : reason) + "}";
    };
    d.grant(count, !step); return {{}, execution.snapshot(*execution.active)};
  }
  std::string breakpointList() const {
    std::string list = "[";
    for(auto pc : ares::Nintendo64::rsp.debugControl.breakpoints) { if(list.size() > 1) list += ','; list += controlQuote(N64Service::hex(pc)); }
    return list + ']';
  }
};
