#include <ares/ares.hpp>
#include "i8255.hpp"

namespace ares {
  auto I8255::power() -> void {
    io.portCLowerInput = 1;
    io.portBInput = 1;
    io.groupBMode = 0;
    io.portCHigherInput = 1;
    io.portAInput = 1;
    io.groupAMode = 0;
    io.ioMode = 0;
    state.portA = 0xff;
    state.portB = 0xff;
    state.portC = 0xff;
  }

  auto I8255::read(n2 port) -> n8 {
    switch(port) {
      case 0: return io.portAInput ? readPortA() : state.portA;
      case 1: return io.portBInput ? readPortB() : state.portB;
      case 2: {
        n8 result = readPortC();
        if(!io.portCLowerInput) result.bit(0,3) = state.portC.bit(0,3);
        if(!io.portCHigherInput) result.bit(4,7) = state.portC.bit(4,7);
        return result;
      }
      case 3: return 0xff;
    }

    unreachable;
  }

  auto I8255::write(n2 port, n8 data) -> void {
    switch(port) {
      case 0: state.portA = data; return writePortA(data);
      case 1: state.portB = data; return writePortB(data);
      case 2: state.portC = data; return writePortC(data);
      case 3: return writeControl(data);
    }

    unreachable;
  }

  auto I8255::writeControl(n8 data) -> void {
    io.portCLowerInput  = data.bit(0);
    io.portBInput       = data.bit(1);
    io.groupBMode       = data.bit(2);
    io.portCHigherInput = data.bit(3);
    io.portAInput       = data.bit(4);
    io.groupAMode       = data.bit(5,6);
    io.ioMode           = data.bit(7);

    if(!io.ioMode) debug(unimplemented, "[I8255] BSR Mode");
    if(io.groupAMode != 0) debug(unimplemented, "[I8255] Group A Mode != 0");
    if(io.groupBMode != 0) debug(unimplemented, "[I8255] Group B Mode != 0");
  }

  auto I8255::serialize(serializer& s) -> void {
    s(io.portCLowerInput);
    s(io.portBInput);
    s(io.groupBMode);
    s(io.portCHigherInput);
    s(io.portAInput);
    s(io.groupAMode);
    s(io.ioMode);

    s(state.portA);
    s(state.portB);
    s(state.portC);
  }
}
