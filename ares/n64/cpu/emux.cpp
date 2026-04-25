// emux - emulator extensions for homebrew developers

auto CPU::ProfileSlot::global() -> ProfileSlot
{
  auto p = ProfileSlot{};
  p.cpu = ::ares::Nintendo64::cpu.profile;
  p.rdram = ::ares::Nintendo64::rdram.profile;
  return p;
}

auto CPU::XDETECT(r64& rd, u64 code) -> void {
  if(!system.homebrewMode) return;
  n64 detect = 0;
  n64 ioctl = 0;
  detect.bit(0x20) = 1;  // XDETECT
  detect.bit(0x25) = 1;  // XLOG
  detect.bit(0x27) = 1;  // XHEXDUMP
  detect.bit(0x28) = 1;  // XPROF
  detect.bit(0x29) = 1;  // XPROFREAD
  detect.bit(0x2a) = 1;  // XEXCEPTION
  detect.bit(0x2c) = 1;  // XIOCTL
  ioctl.bit(0x01) = 1;   // XIOCTL exit
  ioctl.bit(0x02) = 1;   // XIOCTL fast
  ioctl.bit(0x03) = 1;   // XIOCTL slow
  switch(code) {
  case 0x00: rd.s64 = (s32)detect.bit(0x00, 0x1F); break;
  case 0x01: rd.s64 = (s32)detect.bit(0x20, 0x3F); break;
  case 0x02: rd.s64 = (s32)ioctl.bit(0x00, 0x1F); break;
  default:   rd.s64 = 0; break;
  }
}

auto CPU::XLOG(cr64& rd, cr64& rt, u64 code) -> void {
    if(!system.homebrewMode) return;

    auto& emux = debugger.tracer.emux;
    u64 vaddr = rd.u64;
    switch (code) {
    case 0x00:
        while (1) {
            char ch = readDebug<Byte>(vaddr++);
            if(!ch) break;
            emux->notify(ch);
        }
        break;
    case 0x01:
        for(u64 n = 0; n < rt.u64; n++) {
            char ch = readDebug<Byte>(vaddr++);
            emux->notify(ch);
        }
        break;
    }
}

auto CPU::XHEXDUMP(cr64& rd, cr64& rt) -> void {
    if(!system.homebrewMode) return;

    auto& emux = debugger.tracer.emux;
    u64 vaddr = rd.u64;
    u64 length = rt.u64 ? rt.u64 : 256;
    string dump;

    for(u64 n = 0; n < length; n += 16) {
        dump.append(string{hex(vaddr + n, 16L), " ", hex(vaddr + n - rd.u64, 4L), ": "});
        u8 mem[16]; u64 l = length - n < 16 ? length - n : 16;
        for(u64 m = 0; m < l; m++)  mem[m] = readDebug<Byte>(vaddr + n + m);
        for(u64 m = 0; m < 16; m++) {
            if(m < l) dump.append(string{hex(mem[m], 2L), " "});
            else       dump.append("   ");
            if(m == 7) dump.append(" ");
        } 
        dump.append(" |");
        for(u64 m = 0; m < l; ++m) {
            if(mem[m] >= 32 && mem[m] <= 126) dump.append(string{(char)mem[m]});
            else                              dump.append(".");
        }
        for(u64 m = l; m < 16; m++) dump.append(" ");
        dump.append("|\n");
    }

    emux->notify(dump);
}

auto CPU::XPROF(cr64& rd, u64 code) -> void {
    if(!system.homebrewMode) return;

    if(code == 4) {  //reset profiling data
        profileSlots.clear();
        return;
    }

    u64 slot = rd.u64;
    if (slot >= 1024) return;
    if(profileSlots.size() <= slot) {
        profileSlots.resize(slot + 1);
    }

    auto& prof = profileSlots[slot];
    auto global = ProfileSlot::global();

    if(code == 1) { //start profiling
        for (int i=0; i<sizeof(prof.cpu.data)/sizeof(prof.cpu.data[0]); i++) {
            prof.cpu.data[i] -= global.cpu.data[i];
        }
        for (int i=0; i<sizeof(prof.rdram.metrics)/sizeof(prof.rdram.metrics[0]); i++) {
            prof.rdram.metrics[i].reads  -= global.rdram.metrics[i].reads;
            prof.rdram.metrics[i].writes -= global.rdram.metrics[i].writes;
        }
        prof.started = 1;
    }
    if(code == 2) { //stop profiling
        for (int i=0; i<sizeof(prof.cpu.data)/sizeof(prof.cpu.data[0]); i++) {
            prof.cpu.data[i] += global.cpu.data[i];
        }
        for (int i=0; i<sizeof(prof.rdram.metrics)/sizeof(prof.rdram.metrics[0]); i++) {
            prof.rdram.metrics[i].reads  += global.rdram.metrics[i].reads;
            prof.rdram.metrics[i].writes += global.rdram.metrics[i].writes;
        }
        prof.started = 0;
    }
    if(code == 3) { //clear profiling data
        prof = {};
    }
}

auto CPU::XPROFREAD(cr64& rd, r64& rt) -> void {
  if(!system.homebrewMode) return;

  i64 slot = (i64)rd.u64;
  if (slot >= profileSlots.size()) {
    rt.u64 = 0;
    return;
  }

  auto prof = (slot < 0 ? ProfileSlot::global() : profileSlots[slot]);

  u64 code = rt.u64;
  switch(code) {
    case 0x0000: rt.u64 = prof.cpu.cpuCycles; break;
    case 0x0001: rt.u64 = prof.cpu.cpuCyclesExc; break;
    case 0x0010: rt.u64 = prof.cpu.icacheHits; break;
    case 0x0011: rt.u64 = prof.cpu.icacheMisses; break;
    case 0x0012: rt.u64 = prof.cpu.icacheWritebacks; break;
    case 0x0020: rt.u64 = prof.cpu.dcacheHits; break;
    case 0x0021: rt.u64 = prof.cpu.dcacheMisses; break;
    case 0x0022: rt.u64 = prof.cpu.dcacheWritebacks; break;
    case 0x0300: rt.u64 = prof.rdram.total().total(); break;
    case 0x0301: rt.u64 = prof.rdram.total().reads; break;
    case 0x0302: rt.u64 = prof.rdram.total().writes; break;
    case 0x0310: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_ICACHE].total(); break;
    case 0x0311: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_ICACHE].reads; break;
    case 0x0312: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_ICACHE].writes; break;
    case 0x0320: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_DCACHE].total(); break;
    case 0x0321: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_DCACHE].reads; break;
    case 0x0322: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_DCACHE].writes; break;
    case 0x0330: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_UNCACHED].total(); break;
    case 0x0331: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_UNCACHED].reads; break;
    case 0x0332: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VR4300_UNCACHED].writes; break;
    case 0x0340: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SP_DMA].total(); break;
    case 0x0341: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SP_DMA].reads; break;
    case 0x0342: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SP_DMA].writes; break;
    case 0x0350: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::PI_DMA].total(); break;
    case 0x0351: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::PI_DMA].reads; break;
    case 0x0352: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::PI_DMA].writes; break;
    case 0x0360: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SI_DMA].total(); break;
    case 0x0361: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SI_DMA].reads; break;
    case 0x0362: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::SI_DMA].writes; break;
    case 0x0370: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DRAW].total(); break;
    case 0x0371: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DRAW].reads; break;
    case 0x0372: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DRAW].writes; break;
    case 0x0380: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::AI_DMA].total(); break;
    case 0x0381: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::AI_DMA].reads; break;
    case 0x0382: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::AI_DMA].writes; break;
    case 0x0390: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VI_DMA].total(); break;
    case 0x0391: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VI_DMA].reads; break;
    case 0x0392: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::VI_DMA].writes; break;
    case 0x03A0: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DMA].total(); break;
    case 0x03A1: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DMA].reads; break;
    case 0x03A2: rt.u64 = prof.rdram.metrics[(u32)RBusDevice::DP_DMA].writes; break;
    default:     rt.u64 = 0; break;
  }
}

auto CPU::XIOCTL(u64 code) -> void {
  if(!system.homebrewMode) return;

  switch(code) {
    case 0x1: //exit
      printf("[emux] Ares exit requested by application\n");
      platform->event(ares::Event::Shutdown);
      break;
    case 0x2: //fast
      platform->event(ares::Event::FastForwardOn);
      break;
    case 0x3: //slow
      platform->event(ares::Event::FastForwardOff);
      break;
  }
}

auto CPU::XEXCEPTION(r64& rt) -> void {
  if(!system.homebrewMode) return;
  emuxState.excMask = rt.u64;
}
