#include <nall/nall.hpp>
using namespace nall;

#include <nall/main.hpp>

#include <ares/ares.hpp>
#include <component/processor/i8080/i8080.hpp>

bool completed = false;

struct CPU : ares::I8080, ares::I8080::Bus {
  u8 ram[0x10000];

  auto step(u32 clocks) -> void {};
  auto synchronizing() const -> bool { return false; }
  auto read(n16 address) -> n8 { return ram[address]; }
  auto write(n16 address, n8 data) -> void { ram[address] = data; }
  auto in(n16 address) -> n8 { return 0xff; }
  auto out(n16 address, n8 data) -> void {
    u8 port = address.bit(0, 7);
    if(port == 0) { completed = true; }
    else if(port == 1) {
      auto operation = bc.byte.lo;
      if(operation == 2) {
        auto character = de.byte.lo;
        print((char)character);
      }
      if(operation == 9) {
        n16 addr = de.word;
        do {
          print((char)ram[addr++]);
        } while((char)ram[addr] != '$');
      }
    }
  }

  auto power() -> void {
    I8080::bus = this;
    I8080::power();
    PC = 0x0000;
  }
} cpu;

auto test(string binary) {
  auto buffer = file::read(binary);
  if(buffer.size() == 0) {
    print("error: unable to read ", binary, "\n");
    return;
  }
  for(u32 addr : range(buffer.size())) cpu.ram[0x0100 + addr] = buffer[addr];

  cpu.power();
  cpu.PC = 0x0100;

  // out 0,a (test exit condition)
  cpu.ram[0x0000] = 0xd3;
  cpu.ram[0x0001] = 0x00;

  // out 1,a, reti (HLE CP/M syscall)
  cpu.ram[0x0005] = 0xd3;
  cpu.ram[0x0006] = 0x01;
  cpu.ram[0x0007] = 0xc9;

  print("----Starting test: ", binary, "----\n");
  completed = false;
  while(!completed) {
    cpu.instruction();
  }
  print("\n----Completed test: ", binary, "----\n\n");
}

auto nall::main(Arguments arguments) -> void {
  test("tests/CPUTEST.COM");
  test("tests/TST8080.COM");
  test("tests/8080PRE.COM");
  test("tests/8080EXM.COM");
}
