#pragma once
#include "n64-service.hpp"

// Safe-point observations/mutation adapted from ares-64 telemetry-server.cpp at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. No MMIO reads or legacy transport.
struct MachineService {
  struct Object {
    std::string text = "{";
    void add(const std::string& key, const std::string& value) { if(text.size() > 1) text += ','; text += controlQuote(key) + ':' + value; }
    void number(const std::string& key, uint64_t value) { add(key, std::to_string(value)); }
    void hex(const std::string& key, uint64_t value) { add(key, controlQuote(N64Service::hex(value))); }
    std::string end() const { return text + '}'; }
  };
  static constexpr const char* names[] = {"zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"};
  static uint64_t canonical(uint64_t address) { return address >= 0x80000000ull && address <= 0xffffffffull ? address | 0xffffffff00000000ull : address; }
  static ControlResult mutate(const std::vector<std::string>& args, uint64_t epoch, uint64_t frame, bool busy, bool paused) {
    using namespace ares::Nintendo64;
    if(busy || nall::GDB::server.hasClient()) return {"busy", {}};
    if(!paused) return {"not_stopped", {}};
    if(args.size() > 36) return {"invalid_request", {}};
    struct Write { std::string field; unsigned index; uint64_t value; };
    std::vector<Write> writes; bool seen[36] = {};
    for(auto& arg : args) {
      auto colon = arg.find(':'); if(colon == std::string::npos) return {"invalid_request", {}};
      auto field = arg.substr(0, colon), value = arg.substr(colon + 1); unsigned index = 0, slot = 0;
      if(field == "gpr") {
        auto next = value.find(':'); uint32_t reg;
        if(next == std::string::npos || !MemoryService::number(value.substr(0, next), reg) || !reg || reg > 31) return {"invalid_register", {}};
        index = reg; slot = reg - 1; value = value.substr(next + 1);
      } else if(field == "pc") slot = 31;
      else if(field == "hi") slot = 32;
      else if(field == "lo") slot = 33;
      else if(field == "llbit") slot = 34;
      else if(field == "status") slot = 35;
      else return {"invalid_register", {}};
      uint64_t number;
      if(seen[slot] || !N64Service::address(value, number)) return {"invalid_request", {}};
      if((field == "llbit" && number > 1) || (field == "status" && number > 0xffffffffull)) return {"invalid_request", {}};
      if(field == "pc") { number = canonical(number); if(number & 3) return {"unaligned_pc", {}}; }
      seen[slot] = true; writes.push_back({field, index, number});
    }
    Object changed, gpr;
    // Validation of the entire set precedes all architectural changes.
    for(auto& w : writes) {
      if(w.field == "gpr") { cpu.ipu.r[w.index].u64 = w.value; gpr.hex(names[w.index], w.value); }
      else if(w.field == "hi") cpu.ipu.hi.u64 = w.value;
      else if(w.field == "lo") cpu.ipu.lo.u64 = w.value;
      else if(w.field == "llbit") cpu.scc.llbit = w.value;
      else if(w.field == "status") cpu.setControlRegister(12, w.value);
      else {
        cpu.pipeline.setPc(w.value);
        if(cpu.debugControl.halted) cpu.debugControl.resume();
        cpu.debugControl.suppressBreakOnce = false;
        cpu.debugControl.stopValid = false;
      }
      if(w.field != "gpr") changed.add(w.field, w.field == "llbit" ? std::to_string(w.value) : controlQuote(N64Service::hex(w.value)));
    }
    if(gpr.text.size() > 1) changed.add("gpr", gpr.end());
    if constexpr(Accuracy::CPU::Recompiler) cpu.recompiler.reset();
    Object out; out.add("changed", changed.end()); out.hex("pc", cpu.ipu.pc); out.number("epoch", epoch); out.number("frame", frame);
    out.add("jit_invalidated", Accuracy::CPU::Recompiler ? "true" : "false");
    return {{}, out.end()};
  }
  static std::string rspRegisters() {
    using namespace ares::Nintendo64;
    Object out, status, dma;
    std::string gpr = "[", vu = "[", signals = "[";
    for(unsigned i = 0; i < 32; ++i) {
      if(i) { gpr += ','; vu += ','; }
      gpr += controlQuote(N64Service::hex(rsp.ipu.r[i].u32));
      vu += vector(rsp.vpu.r[i]);
    }
    for(unsigned i = 0; i < 8; ++i) { if(i) signals += ','; signals += std::to_string((uint32_t)rsp.status.signal[i]); }
    auto& s = rsp.status;
    status.number("halted", s.halted); status.number("broken", s.broken); status.number("full", s.full);
    status.number("singleStep", s.singleStep); status.number("interruptOnBreak", s.interruptOnBreak); status.number("semaphore", s.semaphore); status.add("signal", signals + ']');
    out.add("gpr", gpr + ']'); out.hex("pc", rsp.ipu.pc); out.add("status", status.end()); out.add("vu", vu + ']');
    out.add("acch", vector(rsp.vpu.acch)); out.add("accm", vector(rsp.vpu.accm)); out.add("accl", vector(rsp.vpu.accl));
    auto& d = rsp.dma;
    dma.add("busy", d.busy.any() ? "true" : "false"); dma.add("full", d.full.any() ? "true" : "false");
    dma.number("busyR", d.busy.read); dma.number("busyW", d.busy.write); dma.number("fullR", d.full.read); dma.number("fullW", d.full.write);
    dma.add("clock", controlQuote(std::to_string(d.clock)));
    dma.number("curLen", d.current.length); dma.number("curCount", d.current.count); dma.hex("curDram", d.current.dramAddress);
    dma.hex("curPbus", d.current.pbusAddress); dma.number("curRegion", d.current.pbusRegion);
    dma.hex("pendDram", d.pending.dramAddress); dma.hex("pendPbus", d.pending.pbusAddress); dma.number("pendRegion", d.pending.pbusRegion); dma.number("pendLen", d.pending.length);
    dma.add("curOriginCpu", d.current.originCpu ? "true" : "false"); dma.hex("curOriginPc", d.current.originPc);
    out.add("dma", dma.end()); out.add("debug", "{\"available\":false}");
    uint32_t instruction = rsp.imem.size >= 4096 ? rsp.imem.read<Word>(rsp.ipu.pc & 0xfff) : 0;
    out.hex("curInstrHex", instruction);
    RSP::Disassembler disassembler(rsp); disassembler.showColors = false;
    out.add("curInstr", controlQuote(disassembler.disassemble(rsp.ipu.pc & 0xfff, instruction).data()));
    return out.end();
  }
  template<typename V> static std::string vector(V& value) {
    std::string out = "["; for(unsigned i = 0; i < 8; ++i) { if(i) out += ','; out += std::to_string((uint16_t)value.element(i)); } return out + ']';
  }
  static std::string rdpState() {
    auto& c = ares::Nintendo64::rdp.command; Object out;
    out.hex("DPC_START", c.start); out.hex("DPC_END", c.end); out.hex("DPC_CURRENT", c.current); out.hex("DPC_CLOCK", c.clock);
    out.hex("PIPE_BUSY", c.pipeBusy); out.hex("TMEM_BUSY", c.tmemBusy); out.hex("BUFFER_BUSY", c.bufferBusy);
    out.number("ready", c.ready); out.number("freeze", c.freeze); out.number("crashed", c.crashed); out.number("flush", c.flush); out.add("source", c.source ? "\"DMEM\"" : "\"RDRAM\"");
    return out.end();
  }
  static std::string peripherals() {
    using namespace ares::Nintendo64; Object out, v, a, p, s, r;
    auto& vi = ares::Nintendo64::vi.io;
    v.number("VI_CTRL.colorDepth", vi.colorDepth); v.number("VI_CTRL.serrate", vi.serrate); v.number("VI_CTRL.antialias", vi.antialias);
    v.hex("VI_ORIGIN", vi.dramAddress); v.number("VI_WIDTH", vi.width); v.number("VI_V_CURRENT", vi.vcounter);
    v.number("VI_X_SCALE", vi.xscale); v.number("VI_Y_SCALE", vi.yscale); v.number("VI_H_START", vi.hstart); v.number("VI_H_END", vi.hend);
    v.number("VI_V_START", vi.vstart); v.number("VI_V_END", vi.vend); v.number("field", vi.field);
    auto& ai = ares::Nintendo64::ai.io;
    a.hex("AI_DRAM_ADDR", ai.dmaAddress[0]); a.number("AI_LENGTH", ai.dmaLength[0]); a.number("AI_DMA_COUNT", ai.dmaCount);
    a.number("AI_DACRATE", ai.dacRate); a.number("AI_BITRATE", ai.bitRate); a.number("AI_ENABLE", ai.dmaEnable);
    auto& pi = ares::Nintendo64::pi.io;
    p.hex("PI_DRAM_ADDR", pi.dramAddress); p.hex("PI_CART_ADDR", pi.pbusAddress); p.number("PI_RD_LEN", pi.readLength); p.number("PI_WR_LEN", pi.writeLength);
    p.number("PI_STATUS.dmaBusy", pi.dmaBusy); p.number("PI_STATUS.ioBusy", pi.ioBusy); p.number("PI_STATUS.error", pi.error); p.number("PI_STATUS.interrupt", pi.interrupt);
    auto& si = ares::Nintendo64::si.io;
    s.hex("SI_DRAM_ADDR", si.dramAddress); s.hex("SI_PIF_RD_ADDR", si.readAddress); s.hex("SI_PIF_WR_ADDR", si.writeAddress);
    s.number("SI_STATUS.dmaBusy", si.dmaBusy); s.number("SI_STATUS.ioBusy", si.ioBusy); s.number("SI_STATUS.interrupt", si.interrupt);
    auto& ri = ares::Nintendo64::ri.io;
    r.hex("RI_MODE", ri.mode); r.hex("RI_CONFIG", ri.config); r.hex("RI_SELECT", ri.select); r.hex("RI_REFRESH", ri.refresh); r.hex("RI_LATENCY", ri.latency);
    out.add("vi", v.end()); out.add("ai", a.end()); out.add("pi", p.end()); out.add("si", s.end()); out.add("ri", r.end()); return out.end();
  }
  static ControlResult disassemble(const std::vector<std::string>& args, uint64_t epoch, uint64_t frame) {
    using namespace ares::Nintendo64; uint64_t address; uint32_t count, physical; nall::string error;
    if(args.size() != 3 || !N64Service::address(args[0], address) || !MemoryService::number(args[1], count) || !count || count > 128 || (args[2] != "cpu" && args[2] != "rdram")) return {"invalid_request", {}};
    address = canonical(address);
    if(address & 3) return {"unaligned_address", {}};
    if(!cpu.debugControl.normalizeAddress(address, count * 4, physical, error)) return {"out_of_bounds", {}};
    if(address < 0x80000000ull) address = 0xffffffff80000000ull | physical;
#if defined(VULKAN)
    if(vulkan.enable) vulkan.flush();
#endif
    CPU::Disassembler disassembler(cpu); disassembler.showColors = disassembler.showValues = false;
    bool cached = args[2] == "cpu" && !((address >= 0xffffffffa0000000ull && address < 0xffffffffc0000000ull));
    std::string rows = "[";
    for(unsigned i = 0; i < count; ++i) {
      uint64_t pc = address + i * 4;
      uint32_t word = cached ? cpu.dcache.readDebug<Word>(pc, physical + i * 4) : rdram.ram.read<Word>(physical + i * 4, RBusDevice::ARES_DEBUGGER);
      std::string text = disassembler.disassemble((uint32_t)pc, word).data(); auto split = text.find(' '), operands = text.find_first_not_of(' ', split);
      Object row; row.hex("addr", pc); row.hex("word", word); row.add("mnemonic", controlQuote(text.substr(0, split))); row.add("operands", controlQuote(operands == std::string::npos ? "" : text.substr(operands)));
      unsigned op = word >> 26;
      if(op == 2 || op == 3) row.hex("target", ((pc + 4) & 0xfffffffff0000000ull) | (uint64_t(word & 0x3ffffff) << 2));
      else if(op == 1 || (op >= 4 && op <= 7) || (op >= 0x14 && op <= 0x17) || (op == 0x11 && (word >> 21 & 31) == 8)) row.hex("target", pc + 4 + int64_t(int16_t(word)) * 4);
      else if(op == 0 && ((word & 63) == 8 || (word & 63) == 9)) { unsigned reg = word >> 21 & 31; row.hex("target", reg ? cpu.ipu.r[reg].u64 : 0); row.add("target_source", "\"current_register_context\""); row.add("target_register", controlQuote(names[reg])); }
      if(i) rows += ','; rows += row.end();
    }
    Object out; out.add("instructions", rows + ']'); out.add("view", controlQuote(args[2])); out.number("epoch", epoch); out.number("frame", frame); return {{}, out.end()};
  }
  static ControlResult execute(const std::string& method, const std::vector<std::string>& args, uint64_t epoch, uint64_t frame, bool available, bool busy, bool paused) {
    if(!available) return {"unavailable", {}};
    if(method == "n64.cpu.setRegisters") return mutate(args, epoch, frame, busy, paused);
    if(method == "n64.cpu.disassemble") return disassemble(args, epoch, frame);
    if(!args.empty()) return {"invalid_request", {}};
    if(method == "n64.rsp.registers") return {{}, rspRegisters()};
    if(method == "n64.rdp.state") return {{}, rdpState()};
    if(method == "n64.peripheral.registers") return {{}, peripherals()};
    return {"unavailable", {}};
  }
};
