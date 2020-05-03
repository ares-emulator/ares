//{
  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleContext() -> string;

  struct Disassembler {
    CPU& self;

    //disassembler.cpp
    auto instruction() -> string;

    bool colors = true;
  } disassembler{*this};
//};
