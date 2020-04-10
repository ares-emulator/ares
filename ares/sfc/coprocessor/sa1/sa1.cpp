SA1 sa1;
#include "rom.cpp"
#include "bwram.cpp"
#include "iram.cpp"
#include "dma.cpp"
#include "memory.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto SA1::load(Node::Object parent) -> void {
  node = parent->append<Node::Component>("SA1");

  debugger.load(node);
}

auto SA1::unload() -> void {
  debugger = {};
  node = {};

  rom.reset();
  iram.reset();
  bwram.reset();

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

//override R65816::interrupt() to support SA-1 vector location I/O registers
auto SA1::interrupt() -> void {
  read(r.pc.d);
  idle();
  if(!r.e) push(r.pc.b);
  push(r.pc.h);
  push(r.pc.l);
  push(r.e ? r.p & ~0x10 : r.p);
  r.p.i = 1;
  r.p.d = 0;
  r.pc.d = r.vector;  //PC bank set to 0x00
}

auto SA1::main() -> void {
  if(r.wai) return instructionWait();
  if(r.stp) return instructionStop();

  if(io.sa1_rdyb || io.sa1_resb) {
    //SA-1 co-processor is asleep
    step();
    return;
  }

  if(status.interruptPending) {
    status.interruptPending = false;
    debugger.interrupt("IRQ");
    interrupt();
    return;
  }

  debugger.instruction();
  instruction();
}

auto SA1::step() -> void {
  Thread::step(2);
  Thread::synchronize(cpu);

  //adjust counters:
  //note that internally, status counters are in clocks;
  //whereas IO register counters are in dots (4 clocks = 1 dot)
  if(io.hvselb == 0) {
    //HV timer
    status.hcounter += 2;
    if(status.hcounter >= 1364) {
      status.hcounter = 0;
      if(++status.vcounter >= status.scanlines) {
        status.vcounter = 0;
      }
    }
  } else {
    //linear timer
    status.hcounter += 2;
    status.vcounter += status.hcounter >> 11;
    status.hcounter &= 0x07ff;
    status.vcounter &= 0x01ff;
  }

  //test counters for timer IRQ
  switch(io.hen << 0 | io.ven << 1) {
  case 0: break;
  case 1: if(status.hcounter == io.hcnt << 2) triggerIRQ(); break;
  case 2: if(status.vcounter == io.vcnt && status.hcounter == 0) triggerIRQ(); break;
  case 3: if(status.vcounter == io.vcnt && status.hcounter == io.hcnt << 2) triggerIRQ(); break;
  }
}

inline auto SA1::lastCycle() -> void {
  if(io.sa1_nmi && !io.sa1_nmicl) {
    status.interruptPending = true;
    r.vector = io.cnv;
    io.sa1_nmifl = true;
    io.sa1_nmicl = 1;
    r.wai = false;
  } else if(!r.p.i) {
    if(io.timer_irqen && !io.timer_irqcl) {
      status.interruptPending = true;
      r.vector = io.civ;
      io.timer_irqfl = true;
      r.wai = false;
    } else if(io.dma_irqen && !io.dma_irqcl) {
      status.interruptPending = true;
      r.vector = io.civ;
      io.dma_irqfl = true;
      r.wai = false;
    } else if(io.sa1_irq && !io.sa1_irqcl) {
      status.interruptPending = true;
      r.vector = io.civ;
      io.sa1_irqfl = true;
      r.wai = false;
    }
  }
}

inline auto SA1::interruptPending() const -> bool {
  return status.interruptPending;
}

inline auto SA1::triggerIRQ() -> void {
  io.timer_irqfl = true;
  if(io.timer_irqen) io.timer_irqcl = 0;
}

auto SA1::power() -> void {
  WDC65816::power();

  Thread::create(system.cpuFrequency(), {&SA1::main, this});
  cpu.coprocessors.append(this);

  bwram.dma = false;
  for(uint address : range(iram.size())) {
    iram.write(address, 0x00);
  }

  status.counter = 0;

  status.interruptPending = false;

  status.scanlines = Region::PAL() ? 312 : 262;
  status.vcounter  = 0;
  status.hcounter  = 0;

  dma.line = 0;

  //$2200 CCNT
  io.sa1_irq  = false;
  io.sa1_rdyb = false;
  io.sa1_resb = true;
  io.sa1_nmi  = false;
  io.smeg     = 0;

  //$2201 SIE
  io.cpu_irqen   = false;
  io.chdma_irqen = false;

  //$2202 SIC
  io.cpu_irqcl   = false;
  io.chdma_irqcl = false;

  //$2203,$2204 CRV
  io.crv = 0x0000;

  //$2205,$2206 CNV
  io.cnv = 0x0000;

  //$2207,$2208 CIV
  io.civ = 0x0000;

  //$2209 SCNT
  io.cpu_irq  = false;
  io.cpu_ivsw = false;
  io.cpu_nvsw = false;
  io.cmeg     = 0;

  //$220a CIE
  io.sa1_irqen   = false;
  io.timer_irqen = false;
  io.dma_irqen   = false;
  io.sa1_nmien   = false;

  //$220b CIC
  io.sa1_irqcl   = false;
  io.timer_irqcl = false;
  io.dma_irqcl   = false;
  io.sa1_nmicl   = false;

  //$220c,$220d SNV
  io.snv = 0x0000;

  //$220e,$220f SIV
  io.siv = 0x0000;

  //$2210
  io.hvselb = false;
  io.ven    = false;
  io.hen    = false;

  //$2212,$2213 HCNT
  io.hcnt = 0x0000;

  //$2214,$2215 VCNT
  io.vcnt = 0x0000;

  //$2220-2223 CXB, DXB, EXB, FXB
  io.cbmode = 0;
  io.dbmode = 0;
  io.ebmode = 0;
  io.fbmode = 0;

  io.cb = 0x00;
  io.db = 0x01;
  io.eb = 0x02;
  io.fb = 0x03;

  //$2224 BMAPS
  io.sbm = 0x00;

  //$2225 BMAP
  io.sw46 = false;
  io.cbm  = 0x00;

  //$2226 SWBE
  io.swen = false;

  //$2227 CWBE
  io.cwen = false;

  //$2228 BWPA
  io.bwp = 0x0f;

  //$2229 SIWP
  io.siwp = 0x00;

  //$222a CIWP
  io.ciwp = 0x00;

  //$2230 DCNT
  io.dmaen = false;
  io.dprio = false;
  io.cden  = false;
  io.cdsel = false;
  io.dd    = 0;
  io.sd    = 0;

  //$2231 CDMA
  io.chdend  = false;
  io.dmasize = 0;
  io.dmacb   = 0;

  //$2232-$2234 SDA
  io.dsa = 0x000000;

  //$2235-$2237 DDA
  io.dda = 0x000000;

  //$2238,$2239 DTC
  io.dtc = 0x0000;

  //$223f BBF
  io.bbf = 0;

  //$2240-$224f BRF
  for(auto& n : io.brf) n = 0x00;

  //$2250 MCNT
  io.acm = 0;
  io.md  = 0;

  //$2251,$2252 MA
  io.ma = 0x0000;

  //$2253,$2254 MB
  io.mb = 0x0000;

  //$2258 VBD
  io.hl = false;
  io.vb = 16;

  //$2259-$225b
  io.va   = 0x000000;
  io.vbit = 0;

  //$2300 SFR
  io.cpu_irqfl   = false;
  io.chdma_irqfl = false;

  //$2301 CFR
  io.sa1_irqfl   = false;
  io.timer_irqfl = false;
  io.dma_irqfl   = false;
  io.sa1_nmifl   = false;

  //$2302,$2303 HCR
  io.hcr = 0x0000;

  //$2304,$2305 VCR
  io.vcr = 0x0000;

  //$2306-$230a MR
  io.mr = 0;

  //$230b
  io.overflow = false;
}
