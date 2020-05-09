//{
  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  struct Disassembler {
    CPU& self;
    Disassembler(CPU& self) : self(self) {}

    //disassembler.cpp
    auto disassemble(u32 address, u32 instruction) -> string;
    template<typename... P> auto hint(P&&... p) const -> string;

    bool showColors = true;
    bool showValues = true;

  private:
    auto EXECUTE() -> vector<string>;
    auto SPECIAL() -> vector<string>;
    auto REGIMM() -> vector<string>;
    auto SCC() -> vector<string>;
    auto FPU() -> vector<string>;
    auto immediate(i64 value, uint bits = 0) const -> string;
    auto cpuRegister(uint index) const -> string;
    auto sccRegister(uint index) const -> string;
    auto fpuRegister(uint index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};
//};
