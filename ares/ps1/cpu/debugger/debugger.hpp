//struct CPU {
  struct Debugger {
    CPU& self;
    Debugger(CPU& self) : self(self), disassembler(self) {}

    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto instruction() -> void;
    auto exception(u8 code) -> void;
    auto interrupt(u8 mask) -> void;
    auto branch() -> void;

    struct Memory {
      Node::Debugger::Memory ram;
      Node::Debugger::Memory scratchpad;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification exception;
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification message;
      Node::Debugger::Tracer::Notification function;
    } tracer;

  private:
    auto messageChar(char) -> void;
    auto messageText(u32) -> void;
    auto message() -> void;
    auto function() -> void;

    struct Disassembler {
      CPU& self;
      Disassembler(CPU& self) : self(self) {}

      auto disassemble(u32 address, u32 instruction) -> string;
      template<typename... P> auto hint(P&&... p) const -> string;

      bool showColors = true;
      bool showValues = true;

      auto EXECUTE() -> std::vector<string>;
      auto SPECIAL() -> std::vector<string>;
      auto REGIMM() -> std::vector<string>;
      auto SCC() -> std::vector<string>;
      auto GTE() -> std::vector<string>;
      auto immediate(s64 value, u8 bits = 0) const -> string;
      auto ipuRegisterName(u8 index) const -> string;
      auto ipuRegisterValue(u8 index) const -> string;
      auto ipuRegisterIndex(u8 index, s16 offset) const -> string;
      auto sccRegisterName(u8 index) const -> string;
      auto sccRegisterValue(u8 index) const -> string;
      auto gteDataRegisterName(u8 index) const -> string;
      auto gteDataRegisterValue(u8 index) const -> string;
      auto gteControlRegisterName(u8 index) const -> string;
      auto gteControlRegisterValue(u8 index) const -> string;

      u32 address;
      u32 instruction;
    } disassembler;
  } debugger{*this};
//};
