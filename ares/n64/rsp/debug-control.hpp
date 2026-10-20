// Cooperative instruction gate adapted from ares-64 rsp-commands.cpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. Host-only, never serialized.
// No transport pump or spin inside the core: a stop unwinds RSP/CPU::main.
struct DebugControl {
  bool active = false, stopped = false, runToBreakpoint = false, skipBreakpoint = false;
  u32 pc = 0, remaining = 0;
  u64 instructions = 0;
  std::vector<u32> breakpoints;
  std::string reason = "disabled";
  static constexpr u32 MaxBreakpoints = 16;
  auto pause(u32 address, const std::string& why) -> void {
    stopped = true; runToBreakpoint = false; remaining = 0; pc = address & 0xfff; reason = why;
  }
  auto grant(u32 count, bool run) -> void {
    stopped = false; runToBreakpoint = run; remaining = count; skipBreakpoint = true; reason = "running";
  }
  auto before(u32 address) -> bool {
    if(!active) return true;
    pc = address & 0xfff;
    if(stopped) return false;
    if(!remaining && !runToBreakpoint) { pause(pc, "rsp_stop"); return false; }
    if(!skipBreakpoint && std::find(breakpoints.begin(), breakpoints.end(), pc) != breakpoints.end()) {
      pause(pc, "rsp_breakpoint"); return false;
    }
    skipBreakpoint = false;
    return true;
  }
  auto after(u32 address, bool halted) -> void {
    if(!active) return;
    ++instructions; pc = address & 0xfff;
    if(remaining) --remaining;
    if(halted) pause(pc, "rsp_halted");
    else if(!runToBreakpoint && !remaining) pause(pc, "rsp_instructions_done");
  }
} debugControl;
