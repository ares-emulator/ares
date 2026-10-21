#pragma once
#include "execution-service.hpp"
#include "stack-service.hpp"
// Adapted from ares-64 debug-session.cpp/telemetry-server.cpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. No instruction patches or inner pump.
struct CallService {
  using Debug = ares::Nintendo64::CPU::DebugControl;
  Debug::Snapshot saved;
  bool savedValid = false;
  uint64_t epoch = 0, activeId = 0, entry = 0, returnGate = 0;
  std::string owner, finalData;
  void reset() { savedValid = false; activeId = 0; owner.clear(); finalData.clear(); ares::Nintendo64::cpu.debugControl.callGate = {}; ares::Nintendo64::cpu.debugControl.updateActive(); }
  // A disconnected owner's retained context can be recovered by a new authenticated
  // connection. While connected, only that owner or UI can restore/discard it.
  void releaseOwner(const std::string& actor) { if(owner == actor) owner.clear(); }
  void restore() {
    using namespace ares::Nintendo64;
    for(unsigned n = 1; n < 32; ++n) cpu.ipu.r[n].u64 = saved.gpr[n];
    cpu.ipu.hi.u64 = saved.hi; cpu.ipu.lo.u64 = saved.lo;
    cpu.setControlRegister(12, saved.cop0[12]); // restore FR before raw FPR bank
    for(unsigned n = 0; n < 32; ++n) cpu.fpu.r[n].u64 = saved.fpr[n];
    cpu.setControlRegisterFPU(31, saved.fcr31);
    cpu.scc.llbit = 0;
    cpu.pipeline.setPc(saved.pc);
    cpu.debugControl.callGate = {};
    cpu.debugControl.resume(); cpu.debugControl.stopValid = false; cpu.debugControl.suppressBreakOnce = false;
    cpu.debugControl.updateActive();
    if constexpr(Accuracy::CPU::Recompiler) cpu.recompiler.reset();
    savedValid = false; owner.clear();
  }
  std::string progress() const {
    if(!finalData.empty()) return finalData;
    MachineService::Object out;
    out.hex("entry", entry); out.hex("return_gate", returnGate); out.hex("pc", ares::Nintendo64::cpu.ipu.pc);
    out.add("completion_pc", "null"); out.add("v0", "null"); out.add("v1", "null"); out.add("exception_code", "null");
    out.add("caller_context_restored", "false"); out.add("recovery_available", "false"); out.add("memory_device_effects_retained", "true");
    out.add("caller_snapshot", "null"); out.add("completion_stop", "{\"valid\":false}"); return out.end();
  }
  void finish(const std::string& reason) {
    using namespace ares::Nintendo64;
    if(!activeId) return;
    auto& d = cpu.debugControl;
    MachineService::Object out;
    out.hex("entry", entry); out.hex("return_gate", returnGate); out.hex("completion_pc", cpu.ipu.pc);
    out.add("completion_stop", N64Service::stop(epoch));
    out.add("exception_code", d.callGate.exception ? std::to_string(d.callGate.exceptionCode) : "null");
    bool success = reason == "function_return";
    out.add("v0", success ? controlQuote(N64Service::hex(cpu.ipu.r[2].u64)) : "null");
    out.add("v1", success ? controlQuote(N64Service::hex(cpu.ipu.r[3].u64)) : "null");
    out.add("caller_snapshot", success ? "null" : N64Service::registers(saved));
    d.callGate.active = false; d.updateActive();
    if(success) restore();
    out.hex("pc", cpu.ipu.pc); out.add("caller_context_restored", success ? "true" : "false");
    out.add("recovery_available", savedValid ? "true" : "false"); out.add("memory_device_effects_retained", "true");
    finalData = out.end(); activeId = 0;
  }
  // Called after each ordinary host slice, before generic debug-stop handling.
  void service(ExecutionService& execution, bool& paused) {
    using namespace ares::Nintendo64;
    if(!activeId || !execution.active || execution.active->id != activeId) return;
    auto& d = cpu.debugControl; auto& g = d.callGate;
    std::string reason;
    if(nall::GDB::server.hasClient()) reason = "gdb_attached";
    else if(g.exception && d.halted) reason = "exception";
    else if(g.returned || ((!d.halted || d.stop.reason == Debug::StopReason::InstructionsDone) && g.armed && !cpu.pipeline.inDelaySlot() && cpu.ipu.pc == g.pc && cpu.ipu.r[29].u64 == g.sp)) reason = "function_return";
    else if(g.invalidReturn) reason = "invalid_return";
    else if(d.halted && d.stop.reason == Debug::StopReason::InstructionsDone) reason = "instruction_limit";
    if(!reason.empty()) execution.complete(reason, paused);
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& actor, uint64_t e, bool available, ExecutionService& execution, bool& paused) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    if(execution.active || execution.reservation || nall::GDB::server.hasClient()) return {"busy", {}};
    if(!paused) return {"not_stopped", {}};
    if(method == "n64.cpu.callRecover") {
      if(args.size() != 1 || (args[0] != "0" && args[0] != "1")) return {"invalid_request", {}};
      if(!savedValid || epoch != e) return {"no_recovery", {}};
      if(!owner.empty() && owner != actor && actor != "ui") return {"busy", {}};
      bool restoring = args[0] == "1";
      if(restoring) restore(); else { savedValid = false; owner.clear(); }
      MachineService::Object out; out.add("restored", restoring ? "true" : "false"); out.add("memory_device_effects_retained", "true");
      out.hex("pc", cpu.ipu.pc); out.number("epoch", e); out.number("frame", execution.frame); return {{}, out.end()};
    }
    if(method != "n64.cpu.call") return {"unavailable", {}};
    if(savedValid) return {"recovery_pending", {}};
    uint64_t address, count, frames, ms, value, dummy; uint32_t physical; nall::string error;
    if(args.size() < 4 || args.size() > 8 || !N64Service::address(args[0], address) || !ExecutionService::number(args[1], count) || !count || count > 1000000 ||
       !ExecutionService::number(args[2], frames) || !frames || frames > 10000 || !ExecutionService::number(args[3], ms) || !ms || ms > 600000) return {"invalid_request", {}};
    if(frames > ExecutionService::maxInteger - execution.frame) return {"invalid_target", {}};
    if(cpu.pipeline.inDelaySlot() || cpu.scc.status.exceptionLevel || cpu.scc.status.errorLevel) return {"unsafe_context", {}};
    if(address & 3) return {"unaligned_address", {}};
    if(!cpu.debugControl.normalizeAddress(address, 4, physical, error)) return {"out_of_bounds", {}};
    if((cpu.ipu.r[29].u64 & 7) || !StackService::readData(cpu.ipu.r[29].u64, 8, dummy)) return {"invalid_stack", {}};
    std::vector<uint64_t> arguments;
    for(size_t i = 4; i < args.size(); ++i) { if(!N64Service::address(args[i], value)) return {"invalid_request", {}}; arguments.push_back(value); }
    saved = cpu.debugControl.snapshot(); savedValid = true; epoch = e; owner = actor; finalData.clear();
    auto result = execution.execute("execution.runFrames", {std::to_string(frames), std::to_string(ms)}, actor, e, paused);
    if(!result.error.empty()) { savedValid = false; owner.clear(); return result; }
    entry = address < 0x80000000ull ? 0xffffffff80000000ull | physical : MachineService::canonical(address);
    auto& d = cpu.debugControl;
    returnGate = 0xffffffff80000000ull | (rdram.ram.size - 4);
    d.callGate = {}; d.callGate.active = true; d.callGate.pc = returnGate; d.callGate.sp = cpu.ipu.r[29].u64;
    for(unsigned i = 0; i < arguments.size(); ++i) cpu.ipu.r[4 + i].u64 = arguments[i];
    cpu.ipu.r[25].u64 = entry; cpu.ipu.r[31].u64 = returnGate; cpu.pipeline.setPc(entry);
    d.stopValid = false; d.suppressBreakOnce = false; d.requestStepInstructions(count);
    activeId = execution.active->id; execution.active->kind = "call_function";
    execution.active->beforeComplete = [this](const std::string& reason) { finish(reason); };
    execution.active->callProgress = [this] { return progress(); };
    execution.executor = N64Service::executor();
    return {{}, execution.snapshot(*execution.active)};
  }
};
