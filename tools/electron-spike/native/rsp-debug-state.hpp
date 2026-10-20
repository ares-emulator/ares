#pragma once
#include "n64-service.hpp"
inline std::string rspDebugState() {
  auto& d = ares::Nintendo64::rsp.debugControl;
  std::string bps = "[";
  for(auto pc : d.breakpoints) { if(bps.size() > 1) bps += ','; bps += controlQuote(N64Service::hex(pc)); }
  return std::string("{\"available\":true,\"active\":") + (d.active ? "true" : "false")
    + ",\"stopped\":" + (d.stopped ? "true" : "false") + ",\"curPc\":" + controlQuote(N64Service::hex(ares::Nintendo64::rsp.ipu.pc & 0xfff))
    + ",\"instrCount\":" + std::to_string(d.instructions) + ",\"stepsRemaining\":" + std::to_string(d.remaining)
    + ",\"runToBp\":" + (d.runToBreakpoint ? "true" : "false") + ",\"breakpoints\":" + bps + ']'
    + ",\"stop_reason\":" + controlQuote(d.reason) + ",\"executor\":" + (d.active ? "\"interpreter\"" : (ares::Nintendo64::Accuracy::RSP::Recompiler && ares::Nintendo64::rsp.recompiler.enabled ? "\"recompiler\"" : "\"interpreter\"")) + "}";
}
