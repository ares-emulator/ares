#pragma once
#include "machine-service.hpp"
// Bounded prologue unwinding adapted from ares-64 cpu/debug-control.{hpp,cpp}
// at 1c9b9637476a83f7b80d3db1115a0f71529820d9. Native observation only.
struct StackService {
  using u64 = uint64_t; using u32 = uint32_t; using u8 = uint8_t;
  using s32 = int32_t; using s16 = int16_t; using string = nall::string;
  using Snapshot = ares::Nintendo64::CPU::DebugControl::Snapshot;
  struct StackFrame {
    u64 pc = 0;          //PC within this frame
    u64 sp = 0;          //stack pointer in this frame
    u64 ra = 0;          //return address used to reach the caller
    u64 callsite = 0;    //ra - 8 (the JAL), 0 when not justified
    u64 functionStart = 0;  //lowest scanned address / detected prologue
    s32 frameSize = 0;   //bytes reserved by ADDIU/DADDIU sp,sp,-N (0 = leaf / unknown)
    s32 raOffset = -1;   //offset of the saved RA slot, -1 = RA not saved
    u64 scanLow = 0, scanHigh = 0;
    u32 raSaveWidth = 0;
    bool epilogueSeen = false, hintUsed = false;
    string raSource = "unresolved";
    u32 method = 0;      //0=live registers, 1=prologue analysis
    u32 confidence = 0;  //0=low 1=medium 2=high
  };
  struct StackResult {
    std::vector<StackFrame> frames;
    string stopReason;
    u64 rawSp = 0;
    std::vector<u8> rawBytes;  //raw stack window at SP (RDRAM view), for manual inspection
  };

  static bool readData(u64 address, u32 size, u64& value) {
    using namespace ares::Nintendo64;
    u32 physical; nall::string error;
    if(address < 0x80000000ull || (size != 1 && size != 4 && size != 8) || (address & (size - 1)) || !cpu.debugControl.normalizeAddress(address, size, physical, error)) return false;
    auto canonical = MachineService::canonical(address);
    bool cached = canonical < 0xffffffffa0000000ull;
    value = 0;
    for(u32 i = 0; i < size; ++i) value = value << 8 | (cached ? cpu.dcache.readDebug<Byte>(0xffffffff80000000ull | (physical + i), physical + i) : rdram.ram.read<Byte>(physical + i, RBusDevice::ARES_DEBUGGER));
    return true;
  }
  static bool readCode(u64 address, u32& word) { u64 value; if(!readData(address, 4, value)) return false; word = value; return true; }
static auto unwind(const Snapshot& snap, u32 maxFrames, u64 functionStartHint) -> StackResult {
  StackResult result;
  static constexpr u32 ScanWindow = 256;  //instructions scanned backwards for a prologue
  u64 pc = snap.pc;
  u64 sp = snap.gpr[29];
  u64 ra = snap.gpr[31];
  result.rawSp = sp;
  for(u32 n = 0; n < 256; ++n) {
    u64 b;
    if(!readData(sp + n, 1, b)) break;
    result.rawBytes.push_back((u8)b);
  }

  auto isCall = [&](u64 address) -> bool {
    u32 w;
    if(!readCode(address, w)) return false;
    if((w >> 26) == 0x03) return true;                                   //JAL
    if((w >> 26) == 0x00 && (w & 0x3f) == 0x09 && (w >> 11 & 31)) return true;  //JALR rd!=0
    if((w >> 26) == 0x01 && (w >> 16 & 31) >= 0x10 && (w >> 16 & 31) <= 0x13) return true;  //BxxAL
    return false;
  };
  auto plausibleCode = [&](u64 address) -> bool {
    u32 w;
    return !(address & 3) && address >= 0x8000'0000ull && readCode(address, w);
  };

  std::vector<std::pair<u64, u64>> seen;
  for(u32 depth = 0; depth < maxFrames; depth++) {
    if(!plausibleCode(pc)) { result.stopReason = {"pc 0x", nall::hex(pc), " is not readable RDRAM code"}; break; }
    for(auto& s : seen) if(s.first == pc && s.second == sp) { result.stopReason = "cycle detected"; return result; }
    seen.push_back({pc, sp});

    StackFrame frame;
    frame.pc = pc;
    frame.sp = sp;

    //scan backwards for `addiu/daddiu sp,sp,-N`, stopping at the previous function's `jr ra`
    u64 allocAt = ~0ull;
    s32 frameSize = 0;
    u64 saveAt = ~0ull;
    s32 raOffset = -1;
    bool raSavedIsDual = false;
    bool deallocated = false;
    u64 functionStart = pc;
    for(u32 k = 0; k < ScanWindow; k++) {
      u64 address = pc - (u64)k * 4;
      if(depth == 0 && functionStartHint && address < functionStartHint) break;
      u32 w;
      if(!readCode(address, w)) break;
      u32 op = w >> 26, rs = w >> 21 & 31, rt = w >> 16 & 31;
      s16 imm = (s16)w;
      if(k > 0 && w == 0x03e0'0008) {  //jr ra of the previous function (+ its delay slot)
        functionStart = address + 8;
        break;
      }
      functionStart = address;
      if((op == 0x09 || op == 0x19) && rs == 29 && rt == 29) {
        if(imm < 0) {
          if(address < pc) {  //already executed: the frame exists
            allocAt = address;
            frameSize = -imm;
          }
          break;
        } else if(address < pc) {
          deallocated = true;  //epilogue already executed between prologue and pc
        }
      }
      if((op == 0x2b || op == 0x3f) && rt == 31 && rs == 29 && address < pc && saveAt == ~0ull) {
        saveAt = address;
        raOffset = imm;
        raSavedIsDual = op == 0x3f;
      }
    }
    frame.functionStart = functionStart;
    frame.scanLow = functionStart; frame.scanHigh = pc + 4;
    frame.raSaveWidth = raOffset >= 0 ? (raSavedIsDual ? 8 : 4) : 0;
    frame.epilogueSeen = deallocated;
    frame.hintUsed = depth == 0 && functionStartHint != 0;

    u64 callerSp = sp;
    u64 returnAddress = 0;
    bool haveReturn = false;
    if(allocAt != ~0ull && !deallocated) {
      frame.frameSize = frameSize;
      frame.method = 1;
      callerSp = sp + frameSize;
      if(saveAt > allocAt && raOffset >= 0) {
        u64 value;
        u64 slot = sp + raOffset;
        if(readData(slot, raSavedIsDual ? 8 : 4, value)) {
          returnAddress = raSavedIsDual ? value : (u64)(s32)(u32)value;
          frame.raOffset = raOffset;
          frame.raSource = "saved_stack_slot";
          haveReturn = true;
          frame.confidence = 2;
        }
      } else if(depth == 0) {
        frame.raSource = "live_ra_before_save";
        returnAddress = ra;  //frame allocated but RA not saved (yet): RA still live
        haveReturn = true;
        frame.confidence = 1;
      }
    } else if(depth == 0) {
      //leaf, pre-prologue or post-epilogue: the return address is still in $ra
      frame.method = 0;
      frame.raSource = deallocated ? "live_ra_after_epilogue" : "live_ra_leaf_or_unknown_prologue";
      returnAddress = ra;
      haveReturn = true;
      frame.confidence = deallocated ? 1 : 0;
    }

    if(!haveReturn) {
      frame.ra = 0;
      result.frames.push_back(frame);
      result.stopReason = "no saved return address found for a non-leaf frame";
      break;
    }
    frame.ra = returnAddress;
    if(returnAddress >= 8 && isCall(returnAddress - 8)) frame.callsite = returnAddress - 8;
    result.frames.push_back(frame);

    if(!plausibleCode(returnAddress)) { result.stopReason = {"return address 0x", nall::hex(returnAddress), " is not readable RDRAM code"}; break; }
    if(!frame.callsite) { result.stopReason = "return address is not preceded by a call instruction"; break; }
    if(callerSp < sp) { result.stopReason = "caller stack pointer would decrease"; break; }
    u64 dummy;
    if(!readData(callerSp & ~3ull, 4, dummy)) { result.stopReason = "caller stack pointer outside RDRAM"; break; }
    pc = returnAddress;
    sp = callerSp;
    if(depth + 1 == maxFrames) result.stopReason = "frame limit reached";
  }
  return result;
}
  static ControlResult execute(const std::vector<std::string>& args, bool available, uint64_t epoch, uint64_t frame) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    u64 hint = 0; u32 word;
    auto snapshot = cpu.debugControl.snapshot();
    if(args.size() > 1 || (!args.empty() && !N64Service::address(args[0], hint))) return {"invalid_request", {}};
    if(!args.empty()) {
      hint = MachineService::canonical(hint);
      if((hint & 3) || hint > snapshot.pc || snapshot.pc - hint >= 1024 || !readCode(hint, word)) return {"invalid_function_start", {}};
    }
    ares::Nintendo64::system.prepareVideo();
#if defined(VULKAN)
    if(vulkan.enable) vulkan.flush();
#endif
    auto result = unwind(snapshot, 8, hint);
    std::string frames = "[";
    for(auto& f : result.frames) {
      if(frames.size() > 1) frames += ',';
      MachineService::Object row, bounds;
      row.hex("pc", f.pc); row.hex("sp", f.sp);
      row.add("return_address", f.ra ? controlQuote(N64Service::hex(f.ra)) : "null");
      row.add("callsite", f.callsite ? controlQuote(N64Service::hex(f.callsite)) : "null");
      row.hex("function_start_guess", f.functionStart); row.number("frame_size", f.frameSize);
      bounds.hex("start", f.scanLow); bounds.hex("end", f.scanHigh); row.add("scan_bounds", bounds.end());
      row.add("sp_adjustment", std::to_string(-f.frameSize)); row.number("ra_save_width", f.raSaveWidth);
      row.add("ra_source", controlQuote(f.raSource.data())); row.add("epilogue_seen", f.epilogueSeen ? "true" : "false");
      row.add("function_hint_used", f.hintUsed ? "true" : "false");
      row.add("ra_slot_offset", f.raOffset >= 0 ? std::to_string(f.raOffset) : "null");
      row.add("method", controlQuote(f.method ? "prologue_analysis" : "live_registers"));
      row.add("confidence", controlQuote(f.confidence == 2 ? "high" : f.confidence == 1 ? "medium" : "low"));
      frames += row.end();
    }
    std::string raw; static const char h[] = "0123456789abcdef";
    for(auto b : result.rawBytes) { raw += h[b >> 4]; raw += h[b & 15]; }
    MachineService::Object out; out.add("frames", frames + ']'); out.add("stop_reason", controlQuote(result.stopReason.data()));
    out.add("heuristic", "true"); out.hex("sp", result.rawSp); out.add("raw_sp_window", controlQuote(raw)); out.number("epoch", epoch); out.number("frame", frame);
    out.add("assumptions", "[\"bounded backward prologue scan; no symbol/unwind metadata\",\"positive SP adjustment indicates an executed epilogue\",\"caller RA must be preceded by a linked call\"]");
    return {{}, out.end()};
  }
};
