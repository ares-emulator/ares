#include <n64/n64.hpp>
#include <nall/gdb/server.hpp>

namespace ares::Nintendo64 {

CPU cpu;
#include "context.cpp"
#include "dcache.cpp"
#include "tlb.cpp"
#include "memory.cpp"
#include "exceptions.cpp"
#include "algorithms.cpp"
#include "interpreter.cpp"
#include "interpreter-ipu.cpp"
#include "interpreter-scc.cpp"
#include "interpreter-fpu.cpp"
#include "interpreter-cop2.cpp"
#include "recompiler.cpp"
#include "debugger.cpp"
#include "serialization.cpp"
#include "disassembler.cpp"

auto CPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("CPU");
  debugger.load(node);
}

auto CPU::unload() -> void {
  debugger.unload();
  node.reset();
}

auto CPU::main() -> void {
  while(!vi.refreshed && GDB::server.reportPC(ipu.pc & 0xFFFFFFFF)) {
    instruction();
    synchronize();
  }

  vi.refreshed = false;
  queue.remove(Queue::GDB_Poll);
  if(GDB::server.hasClient()) {
    queue.insert(Queue::GDB_Poll, system.frequency()/60/240);
  }
}

auto CPU::gdbPoll() -> void {
  if(GDB::server.hasClient()) {
    GDB::server.updateLoop();
    queue.insert(Queue::GDB_Poll, system.frequency()/60/240);
  }
}

auto CPU::synchronize() -> void {
  auto clocks = Thread::clock;
  Thread::clock = 0;

   mi.clock -= clocks;
   vi.clock -= clocks;
   ai.clock -= clocks;
  rsp.clock -= clocks;
  rdp.clock -= clocks;
  if(!system._BB()) pif.clock -= clocks;
  mi.main();
  vi.main();
  ai.main();
  rsp.main();
  rdp.main();
  if(!system._BB()) pif.main();

  queue.step(clocks, [](u32 event) {
    switch(event) {
    case Queue::PI_DMA_Read:   return pi.dmaFinished();
    case Queue::PI_DMA_Write:  return pi.dmaFinished();
    case Queue::PI_BUS_Write:  return pi.writeFinished();
    case Queue::SI_DMA_Read:   return si.dmaRead();
    case Queue::SI_DMA_Write:  return si.dmaWrite();
    case Queue::SI_BUS_Write:  return si.writeFinished();
    case Queue::VIRAGE0_Command: return virage0.commandFinished();
    case Queue::VIRAGE1_Command: return virage1.commandFinished();
    case Queue::VIRAGE2_Command: return virage2.commandFinished();
    case Queue::NAND_Command: return pi.nandCommandFinished();
    case Queue::AES_Command: return pi.aesCommandFinished();
    case Queue::BB_RTC_Tick: return pi.bb_rtc.tickClock();
    case Queue::RTC_Tick:      return cartridge.rtc.tick();
    case Queue::DD_Clock_Tick:  return dd.rtc.tickClock();
    case Queue::DD_MECHA_Response:  return dd.mechaResponse();
    case Queue::DD_BM_Request:  return dd.bmRequest();
    case Queue::DD_Motor_Mode:  return dd.motorChange();
    case Queue::GDB_Poll:      return cpu.gdbPoll();
    }
  });

  clocks >>= 1;
  if(scc.count < scc.compare && scc.count + clocks >= scc.compare) {
    scc.cause.interruptPending.bit(Interrupt::Timer) = 1;
  }
  scc.count += clocks;
}

auto CPU::instruction() -> void {
  if(auto interrupts = scc.cause.interruptPending & scc.status.interruptMask) {
    if(scc.status.interruptEnable && !scc.status.exceptionLevel && !scc.status.errorLevel) {
      debugger.interrupt(scc.cause.interruptPending);
      step(1 * 2);
      return exception.interrupt();
    }
  }
  if (scc.nmiPending || scc.nmiStrobe) {
    scc.nmiStrobe = 0;
    debugger.nmi();
    step(1 * 2);
    return exception.nmi();
  }
  if (scc.sysadFrozen) {
    step(1 * 2);
    return;
  }

  auto access = devirtualize<Read, Word>(ipu.pc);
  if(!access) return;

  if(Accuracy::CPU::Recompiler && recompiler.enabled && access.cache) {
    if(vaddrAlignedError<Word>(access.vaddr, false)) return;
    auto block = recompiler.block(ipu.pc, access.paddr, GDB::server.hasBreakpoints());
    if(block) {
      block->execute(*this);
      return;
    } 
  }

  auto data = fetch(access);
  if (!data) return;
  pipeline.begin();
  instructionPrologue(ipu.pc, *data);
  decoderEXECUTE(*data);
  instructionEpilogue<0>();
  pipeline.end();
}

auto CPU::instructionPrologue(u64 address, u32 instruction) -> void {
  debugger.instruction(address, instruction);
}

template<bool Recompiled>
auto CPU::instructionEpilogue() -> void {
  if constexpr(!Recompiled) {
    ipu.r[0].u64 = 0;
  }
}

auto CPU::power(bool reset) -> void {
  Thread::reset();

  context.endian = Context::Endian::Big;
  context.mode = Context::Mode::Kernel;
  context.bits = 64;
  for(auto& segment : context.segment) segment = Context::Segment::Unused;
  icache.power(reset);
  dcache.power(reset);
  for(auto& entry : tlb.entry) entry = {}, entry.synchronize();
  tlb.physicalAddress = 0;
  for(auto& r : ipu.r) r.u64 = 0;
  ipu.lo.u64 = 0;
  ipu.hi.u64 = 0;
  ipu.r[29].u64 = 0xffff'ffff'a400'1ff0ull;  //stack pointer
  pipeline.setPc(0xffff'ffff'bfc0'0000ull);
  scc = {};
  for(auto& r : fpu.r) r.u64 = 0;
  fpu.csr = {};
  cop2 = {};
  fenv.setRound(float_env::toNearest);
  context.setMode();

  if (system._BB()) {
    //Note: iQue divmode is still 1:1.5 but the value reported is different
    scc.configuration.systemClockRatio = 1;

    scc.coprocessor.revision = 0x40;
    scc.coprocessor.implementation = 0x0B;
  }

  if constexpr(Accuracy::CPU::Recompiler) {
    auto buffer = ares::Memory::FixedAllocator::get().tryAcquire(63_MiB);
    recompiler.allocator.resize(63_MiB, bump_allocator::executable, buffer);
    recompiler.reset();
  }
}

}
