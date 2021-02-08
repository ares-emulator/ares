//Super Accelerator (SA-1)

struct SA1 : WDC65816, Thread {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  //sa1.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto interrupt() -> void override;
  auto main() -> void;
  auto step() -> void;

  auto lastCycle() -> void override;
  auto interruptPending() const -> bool override;
  auto triggerIRQ() -> void;
  auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  auto power() -> void;

  //dma.cpp
  struct DMA {
    enum CDEN : u32 { DmaNormal = 0, DmaCharConversion = 1 };
    enum SD : u32 { SourceROM = 0, SourceBWRAM = 1, SourceIRAM = 2 };
    enum DD : u32 { DestIRAM = 0, DestBWRAM = 1 };
    u32 line;
  };

  auto dmaNormal() -> void;
  auto dmaCC1() -> void;
  auto dmaCC1Read(n24 address) -> n8;
  auto dmaCC2() -> void;

  //memory.cpp
  auto idle() -> void override;
  auto idleJump() -> void override;
  auto idleBranch() -> void override;
  auto read(n24 address) -> n8 override;
  auto write(n24 address, n8 data) -> void override;
  auto readVBR(n24 address, n8 data = 0) -> n8;
  auto readDisassembler(n24 address) -> n8 override;

  //io.cpp
  auto readIOCPU(n24 address, n8 data) -> n8;
  auto readIOSA1(n24 address, n8 data) -> n8;
  auto writeIOCPU(n24 address, n8 data) -> void;
  auto writeIOSA1(n24 address, n8 data) -> void;
  auto writeIOShared(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct ROM : ReadableMemory {
    //rom.cpp
    auto conflict() const -> bool;

    auto read(n24 address, n8 data = 0) -> n8 override;
    auto write(n24 address, n8 data) -> void override;

    auto readCPU(n24 address, n8 data = 0) -> n8;
    auto writeCPU(n24 address, n8 data) -> void;

    auto readSA1(n24 address, n8 data = 0) -> n8;
    auto writeSA1(n24 address, n8 data) -> void;
  } rom;

  struct BWRAM : WritableMemory {
    //bwram.cpp
    auto conflict() const -> bool;

    auto read(n24 address, n8 data = 0) -> n8 override;
    auto write(n24 address, n8 data) -> void override;

    auto readCPU(n24 address, n8 data = 0) -> n8;
    auto writeCPU(n24 address, n8 data) -> void;

    auto readSA1(n24 address, n8 data = 0) -> n8;
    auto writeSA1(n24 address, n8 data) -> void;

    auto readLinear(n24 address, n8 data = 0) -> n8;
    auto writeLinear(n24 address, n8 data) -> void;

    auto readBitmap(n20 address, n8 data = 0) -> n8;
    auto writeBitmap(n20 address, n8 data) -> void;

    bool dma;
  } bwram;

  struct IRAM : WritableMemory {
    //iram.cpp
    auto conflict() const -> bool;

    auto read(n24 address, n8 data = 0) -> n8 override;
    auto write(n24 address, n8 data) -> void override;

    auto readCPU(n24 address, n8 data) -> n8;
    auto writeCPU(n24 address, n8 data) -> void;

    auto readSA1(n24 address, n8 data = 0) -> n8;
    auto writeSA1(n24 address, n8 data) -> void;
  } iram;

private:
  DMA dma;

  struct Status {
    n8 counter;

    bool interruptPending;

    n16 scanlines;
    n16 vcounter;
    n16 hcounter;
  } status;

  struct IO {
    //$2200 CCNT
    bool sa1_irq;
    bool sa1_rdyb;
    bool sa1_resb;
    bool sa1_nmi;
    n8 smeg;

    //$2201 SIE
    bool cpu_irqen;
    bool chdma_irqen;

    //$2202 SIC
    bool cpu_irqcl;
    bool chdma_irqcl;

    //$2203,$2204 CRV
    n16 crv;

    //$2205,$2206 CNV
    n16 cnv;

    //$2207,$2208 CIV
    n16 civ;

    //$2209 SCNT
    bool cpu_irq;
    bool cpu_ivsw;
    bool cpu_nvsw;
    n8 cmeg;

    //$220a CIE
    bool sa1_irqen;
    bool timer_irqen;
    bool dma_irqen;
    bool sa1_nmien;

    //$220b CIC
    bool sa1_irqcl;
    bool timer_irqcl;
    bool dma_irqcl;
    bool sa1_nmicl;

    //$220c,$220d SNV
    n16 snv;

    //$220e,$220f SIV
    n16 siv;

    //$2210 TMC
    bool hvselb;
    bool ven;
    bool hen;

    //$2212,$2213
    n16 hcnt;

    //$2214,$2215
    n16 vcnt;

    //$2220 CXB
    bool cbmode;
    n32 cb;

    //$2221 DXB
    bool dbmode;
    n32 db;

    //$2222 EXB
    bool ebmode;
    n32 eb;

    //$2223 FXB
    bool fbmode;
    n32 fb;

    //$2224 BMAPS
    n8 sbm;

    //$2225 BMAP
    bool sw46;
    n8 cbm;

    //$2226 SBWE
    bool swen;

    //$2227 CBWE
    bool cwen;

    //$2228 BWPA
    n8 bwp;

    //$2229 SIWP
    n8 siwp;

    //$222a CIWP
    n8 ciwp;

    //$2230 DCNT
    bool dmaen;
    bool dprio;
    bool cden;
    bool cdsel;
    bool dd;
    n8 sd;

    //$2231 CDMA
    bool chdend;
    n8 dmasize;
    n8 dmacb;

    //$2232-$2234 SDA
    n24 dsa;

    //$2235-$2237 DDA
    n24 dda;

    //$2238,$2239 DTC
    n16 dtc;

    //$223f BBF
    bool bbf;

    //$2240-224f BRF
    n8 brf[16];

    //$2250 MCNT
    bool acm;
    bool md;

    //$2251,$2252 MA
    n16 ma;

    //$2253,$2254 MB
    n16 mb;

    //$2258 VBD
    bool hl;
    n8 vb;

    //$2259-$225b VDA
    n24 va;
    n8 vbit;

    //$2300 SFR
    bool cpu_irqfl;
    bool chdma_irqfl;

    //$2301 CFR
    bool sa1_irqfl;
    bool timer_irqfl;
    bool dma_irqfl;
    bool sa1_nmifl;

    //$2302,$2303 HCR
    n16 hcr;

    //$2304,$2305 VCR
    n16 vcr;

    //$2306-230a MR
    n64 mr;

    //$230b OF
    bool overflow;
  } io;
};

extern SA1 sa1;
