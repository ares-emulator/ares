//{
  struct Disassembler {
    RSP& self;
    Disassembler(RSP& self) : self(self) {}

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
    auto LWC2() -> vector<string>;
    auto SWC2() -> vector<string>;
    auto VU() -> vector<string>;
    auto immediate(i64 value, uint bits = 0) const -> string;
    auto cpuRegisterName(uint index) const -> string;
    auto cpuRegisterValue(uint index) const -> string;
    auto cpuRegisterIndex(uint index, i16 offset) const -> string;
    auto sccRegisterName(uint index) const -> string;
    auto sccRegisterValue(uint index) const -> string;
    auto vuRegisterName(uint index, uint element = 0) const -> string;
    auto vuRegisterValue(uint index, uint element = 0) const -> string;
    auto ccrRegisterName(uint index) const -> string;
    auto ccrRegisterValue(uint index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};
//};
