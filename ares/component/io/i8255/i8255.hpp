#pragma once

namespace ares {
  struct I8255 {
    //i8255.cpp
    auto power() -> void;
    auto read(n2 port) -> n8;
    auto write(n2 port, n8 data) -> void;
    auto writeControl(n8 data) -> void;
    auto serialize(serializer&) -> void;

    virtual auto readPortA() -> n8 { return 0xff; };
    virtual auto readPortB() -> n8 { return 0xff; };
    virtual auto readPortC() -> n8 { return 0xff; };
    virtual auto writePortA(n8 data) -> void {};
    virtual auto writePortB(n8 data) -> void {};
    virtual auto writePortC(n8 data) -> void {};

    struct IO {
      n1 portCLowerInput;
      n1 portBInput;
      n1 groupBMode;
      n1 portCHigherInput;
      n1 portAInput;
      n2 groupAMode;
      n1 ioMode;
    } io;

    struct State {
      n8 portA;
      n8 portB;
      n8 portC;
    } state;
  };
}

