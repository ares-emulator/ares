#pragma once
#include <charconv>
#include <iomanip>
#include <sstream>
#include <n64/n64.hpp>
#include <nall/gdb/server.hpp>
#include "memory-service.hpp"

// N64 semantics adapted from ares-64 debug-control and telemetry memory helpers
// at 1c9b9637476a83f7b80d3db1115a0f71529820d9. Only called at native safe points.
struct N64Service {
  using Debug = ares::Nintendo64::CPU::DebugControl;
  bool available = false;
  static std::string hex(uint64_t value) {
    std::ostringstream s; s << "0x" << std::hex << value; return s.str();
  }
  static bool address(const std::string& text, uint64_t& value) {
    if(text.size() < 3 || text.size() > 18 || !(text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))) return false;
    auto [end, error] = std::from_chars(text.data() + 2, text.data() + text.size(), value, 16);
    return error == std::errc{} && end == text.data() + text.size();
  }
  static std::string executor() {
    using namespace ares::Nintendo64;
    return !cpu.debugControl.active && Accuracy::CPU::Recompiler && cpu.recompiler.enabled ? "recompiler" : "interpreter";
  }
  static std::string registers(const Debug::Snapshot& s) {
    std::string out = "{\"pc\":" + controlQuote(hex(s.pc)) + ",\"next_pc\":" + controlQuote(hex(s.nextPc))
      + ",\"delay_slot\":" + (s.delaySlot ? "true" : "false") + ",\"lo\":" + controlQuote(hex(s.lo)) + ",\"hi\":" + controlQuote(hex(s.hi));
    auto array = [&](const char* name, const auto& values, uint32_t valid) {
      out += std::string(",\"") + name + "\":[";
      for(unsigned i = 0; i < 32; ++i) { if(i) out += ','; out += (valid & (1u << i)) ? controlQuote(hex(values[i])) : "null"; }
      out += ']';
    };
    array("gpr", s.gpr, ~0u); array("cop0", s.cop0, s.cop0Valid); array("fpr", s.fpr, ~0u);
    return out + ",\"fcr0\":" + controlQuote(hex(s.fcr0)) + ",\"fcr31\":" + controlQuote(hex(s.fcr31)) + "}";
  }
  static std::string reason(Debug::StopReason reason) {
    switch(reason) {
    case Debug::StopReason::Breakpoint: return "breakpoint";
    case Debug::StopReason::Watchpoint: return "watchpoint";
    case Debug::StopReason::InstructionsDone: return "instructions_done";
    case Debug::StopReason::Requested: return "requested";
    default: return "none";
    }
  }
  static std::string stop(uint64_t epoch) {
    auto& d = ares::Nintendo64::cpu.debugControl;
    if(!d.stopValid) return "{\"valid\":false}";
    auto& s = d.stop;
    auto ids = [](const auto& values) { std::string out = "["; for(auto id : values) { if(out.size() > 1) out += ','; out += std::to_string(id); } return out + ']'; };
    std::string out = "{\"valid\":true,\"stop_reason\":" + controlQuote(reason(s.reason)) + ",\"epoch\":" + std::to_string(epoch)
      + ",\"frame\":" + std::to_string(s.frame) + ",\"instructions\":" + std::to_string(s.instructions)
      + ",\"registers\":" + registers(s.snapshot);
    if(s.reason == Debug::StopReason::Breakpoint) out += ",\"hit\":{\"id\":" + std::to_string(s.breakpointId)
      + ",\"pc\":" + controlQuote(hex(s.breakpointPc)) + ",\"other_ids\":" + ids(s.otherBreakpointIds) + "}";
    if(s.access.valid) {
      auto& a = s.access;
      out += ",\"hit\":{\"id\":" + std::to_string(a.watchId) + ",\"other_ids\":" + ids(a.otherIds)
        + ",\"kind\":" + controlQuote(a.kind == Debug::AccessRead ? "read" : "write") + ",\"access_pc\":" + controlQuote(hex(a.pc))
        + ",\"instruction\":" + controlQuote(hex(a.instruction)) + ",\"addr\":" + controlQuote(hex(a.vaddr))
        + ",\"paddr\":" + controlQuote(hex(a.paddr)) + ",\"size\":" + std::to_string(a.size)
        + ",\"value\":" + controlQuote(hex(a.value)) + ",\"old_value\":" + (a.hasOld ? controlQuote(hex(a.oldValue)) : "null") + "}";
    }
    return out + "}";
  }
  static bool condition(const std::vector<std::string>& args, size_t start, Debug::Condition& result) {
    if(args.size() - start > Debug::MaxPredicates) return false;
    for(size_t i = start; i < args.size(); ++i) {
      std::istringstream in(args[i]); std::string parts[5];
      for(unsigned j = 0; j < 5; ++j) if(!std::getline(in, parts[j], ':') || parts[j].empty()) return false;
      if(in.peek() != EOF || std::count(args[i].begin(), args[i].end(), ':') != 4) return false;
      uint32_t field, index, op; uint64_t value, mask;
      if(!MemoryService::number(parts[0], field) || field > 4 || !MemoryService::number(parts[1], index) || index > 31 ||
         !MemoryService::number(parts[2], op) || op > 5 || !address(parts[3], value) || !address(parts[4], mask)) return false;
      result.all.push_back({static_cast<Debug::Field>(field), index, static_cast<Debug::Op>(op), value, mask});
    }
    return true;
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, uint64_t epoch, bool busy) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    auto& d = cpu.debugControl;
    if(method == "n64.debug.registers") {
      if(!args.empty()) return {"invalid_request", {}};
      return {{}, registers(d.snapshot())};
    }
    if(method == "n64.debug.lastStop") {
      if(!args.empty()) return {"invalid_request", {}};
      return {{}, stop(epoch)};
    }
    if(method == "n64.debug.breakList" || method == "n64.debug.watchList") {
      uint32_t offset;
      if(args.size() != 1 || !MemoryService::number(args[0], offset)) return {"invalid_request", {}};
      std::string out = "{\"definitions\":["; unsigned count = 0, total = 0;
      auto list = [&](const auto& definitions) {
        total = definitions.size();
        for(size_t i = offset; i < definitions.size() && count < 32; ++i, ++count) {
          auto& b = definitions[i]; if(count) out += ',';
          out += "{\"id\":" + std::to_string(b.id) + ",\"addr\":" + controlQuote(hex(b.address)) + ",\"paddr\":" + controlQuote(hex(b.paddr)) + ",\"hits\":" + std::to_string(b.hits);
          if constexpr(requires { b.length; }) out += ",\"length\":" + std::to_string(b.length) + ",\"kind\":" + std::to_string(b.kind);
          out += ",\"condition\":[";
          for(size_t j = 0; j < b.condition.all.size(); ++j) {
            auto& p = b.condition.all[j]; if(j) out += ',';
            out += "{\"field\":" + std::to_string(static_cast<uint32_t>(p.field)) + ",\"index\":" + std::to_string(p.index) + ",\"op\":" + std::to_string(static_cast<uint32_t>(p.op)) + ",\"value\":" + controlQuote(hex(p.value)) + ",\"mask\":" + controlQuote(hex(p.mask)) + "}";
          }
          out += "]}";
        }
      };
      if(method == "n64.debug.breakList") list(d.breakpoints); else list(d.watchpoints);
      return {{}, out + "],\"total\":" + std::to_string(total) + "}"};
    }
    if(method == "n64.memory.read" || method == "n64.memory.write") return memory(method, args, busy);
    if(busy) return {"busy", {}};
    if(nall::GDB::server.hasClient()) return {"busy", {}};
    if(method == "n64.debug.clear") {
      if(!args.empty()) return {"invalid_request", {}};
      d.breakpoints.clear(); d.watchpoints.clear(); d.resume();
      return {{}, "{\"cleared\":true}"};
    }
    if(method == "n64.debug.breakRemove" || method == "n64.debug.watchRemove") {
      uint32_t id;
      if(args.size() != 1 || !MemoryService::number(args[0], id) || !id) return {"invalid_request", {}};
      bool removed = method == "n64.debug.breakRemove" ? d.removeBreakpoint(id) : d.removeWatchpoint(id);
      return removed ? ControlResult{{}, "{\"removed\":true}"} : ControlResult{"unknown_definition", {}};
    }
    if(method == "n64.debug.breakAdd" || method == "n64.debug.watchAdd") {
      bool watch = method == "n64.debug.watchAdd";
      size_t start = watch ? 3 : 1; uint64_t addr; uint32_t length = 0, kind = 0;
      if(args.size() < start || !address(args[0], addr) || (watch && (!MemoryService::number(args[1], length) || !MemoryService::number(args[2], kind)))) return {"invalid_request", {}};
      Debug::Condition c;
      if(!condition(args, start, c)) return {"invalid_request", {}};
      nall::string error;
      uint32_t id = watch ? d.addWatchpoint(addr, length, kind, std::move(c), error) : d.addBreakpoint(addr, std::move(c), error);
      if(!id) return {"invalid_definition", {}};
      return {{}, "{\"id\":" + std::to_string(id) + "}"};
    }
    return {"unavailable", {}};
  }
  static ControlResult memory(const std::string& method, const std::vector<std::string>& args, bool busy) {
    using namespace ares::Nintendo64;
    bool write = method == "n64.memory.write";
    if(write && (busy || nall::GDB::server.hasClient())) return {"busy", {}};
    uint64_t addr; uint32_t length, paddr; nall::string error;
    if(args.size() != 3 || !address(args[0], addr) || (args[1] != "cpu" && args[1] != "rdram")) return {"invalid_request", {}};
    std::vector<uint8_t> bytes;
    if(write) {
      if(args[2].empty() || args[2].size() % 2 || args[2].size() > MemoryService::maxTransfer * 2) return {"invalid_request", {}};
      for(size_t i = 0; i < args[2].size(); i += 2) {
        int hi = MemoryService::nibble(args[2][i]), lo = MemoryService::nibble(args[2][i + 1]);
        if(hi < 0 || lo < 0) return {"invalid_request", {}};
        bytes.push_back(hi << 4 | lo);
      }
      length = bytes.size();
    } else if(!MemoryService::number(args[2], length) || !length || length > MemoryService::maxTransfer) return {"invalid_request", {}};
    if(!cpu.debugControl.normalizeAddress(addr, length, paddr, error)) return {"out_of_bounds", {}};
    // Until RDRAM initialization finishes, chip mapping can silently drop a write.
    if(write && !rdram.mapIdentity) return {"rdram_not_initialized", {}};
#if defined(VULKAN)
    if(vulkan.enable) vulkan.flush();
#endif
    bool kseg1 = (addr >= 0xa0000000ull && addr < 0xc0000000ull) || (addr >= 0xffffffffa0000000ull && addr < 0xffffffffc0000000ull);
    bool cached = args[1] == "cpu" && !kseg1;
    unsigned dirty = 0, resident = 0;
    for(auto& line : cpu.dcache.lines) {
      uint32_t pa = (line.tagKey & ~0xfffu) | line.index;
      if(line.valid() && line.dirty && pa < paddr + length && pa + 16 > paddr) ++dirty;
    }
    for(auto& line : cpu.icache.lines) {
      uint32_t pa = (line.tagKey & ~0xfffu) | line.index;
      if(line.valid() && pa < paddr + length && pa + 32 > paddr) ++resident;
    }
    const char* digits = "0123456789abcdef"; std::string encoded;
    for(uint32_t i = 0; i < length; ++i) {
      uint32_t p = paddr + i; uint64_t va = 0xffffffff80000000ull | p;
      if(write) {
        if(cached) cpu.dcache.writeDebug<Byte>(va, p, bytes[i]);
        else rdram.ram.write<Byte>(p, bytes[i], RBusDevice::ARES_DEBUGGER);
      } else {
        uint8_t byte = cached ? cpu.dcache.readDebug<Byte>(va, p) : rdram.ram.read<Byte>(p, RBusDevice::ARES_DEBUGGER);
        encoded += digits[byte >> 4]; encoded += digits[byte & 15];
      }
    }
    if(write && Accuracy::CPU::Recompiler) cpu.recompiler.invalidateRange(paddr, length);
    std::string out = "{\"addr\":" + controlQuote(hex(addr)) + ",\"paddr\":" + controlQuote(hex(paddr)) + ",\"view\":" + controlQuote(args[1]) + ",\"cache_aware\":" + (cached ? "true" : "false");
    if(!write) return {{}, out + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(encoded) + "}"};
    std::string hazards = "[";
    if(!cached && dirty) hazards += "\"dirty_dcache_may_overwrite\"";
    if(resident) { if(hazards.size() > 1) hazards += ','; hazards += "\"resident_icache_not_updated\""; }
    hazards += ']';
    return {{}, out + ",\"hazards\":" + hazards + ",\"written\":" + std::to_string(length) + ",\"jit_invalidated\":" + (Accuracy::CPU::Recompiler ? "true" : "false")
      + ",\"icache_invalidated\":false,\"dcache_dirty_lines_overlapping\":" + std::to_string(dirty) + ",\"icache_lines_overlapping\":" + std::to_string(resident) + "}"};
  }
};
