//{
  struct Disassembler {
    CPU& self;
    Disassembler(CPU& self) : self(self) {}

    //disassembler.cpp
    auto disassemble(u32 address, u32 instruction) -> string;
    template<typename... P> auto hint(P&&... p) const -> string;

    bool showColors = true;
    bool showValues = true;

  //private:
    auto EXECUTE() -> vector<string>;
    auto SPECIAL() -> vector<string>;
    auto REGIMM() -> vector<string>;
    auto SCC() -> vector<string>;
    auto GTE() -> vector<string>;
    auto immediate(i64 value, u8 bits = 0) const -> string;
    auto cpuRegisterName(u8 index) const -> string;
    auto cpuRegisterValue(u8 index) const -> string;
    auto cpuRegisterIndex(u8 index, i16 offset) const -> string;
    auto sccRegisterName(u8 index) const -> string;
    auto sccRegisterValue(u8 index) const -> string;
    auto gteDataRegisterName(u8 index) const -> string;
    auto gteDataRegisterValue(u8 index) const -> string;
    auto gteControlRegisterName(u8 index) const -> string;
    auto gteControlRegisterValue(u8 index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};
//};
