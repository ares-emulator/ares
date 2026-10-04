#include <nall/nall.hpp>
#include <nall/main.hpp>

#include <ares/ares.hpp>
#include <component/processor/armv6m/armv6m.hpp>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

using namespace nall;

namespace {

auto expect(bool condition, const char* message) -> bool {
  if(condition) return true;
  std::cerr << "FAIL: " << message << "\n";
  return false;
}

struct Processor : ares::ARMv6M {
  std::vector<u8> memory = std::vector<u8>(0x20'000);
  u64 clocks = 0;
  u32 reads = 0;
  u32 debuggerReads = 0;
  u32 writes = 0;

  auto step(u32 amount) -> void override { clocks += amount; }

  auto readMemory(u32 mode, n32 address, n32& data) -> Fault {
    auto size = mode & Byte ? 1u : mode & Half ? 2u : 4u;
    auto offset = (u32)address;
    if(offset > memory.size() || size > memory.size() - offset) return mode & Prefetch ? Fault::Fetch : Fault::Read;
    data = 0;
    for(u32 byte = 0; byte < size; byte++) data |= (u32)memory[offset + byte] << byte * 8;
    return Fault::None;
  }

  auto get(u32 mode, n32 address, n32& data) -> Fault override {
    reads++;
    return readMemory(mode, address, data);
  }

  auto getDebugger(u32 mode, n32 address, n32& data) -> Fault override {
    debuggerReads++;
    return readMemory(mode, address, data);
  }

  auto set(u32 mode, n32 address, n32 data) -> Fault override {
    writes++;
    auto size = mode & Byte ? 1u : mode & Half ? 2u : 4u;
    auto offset = (u32)address;
    if(offset > memory.size() || size > memory.size() - offset) return Fault::Write;
    for(u32 byte = 0; byte < size; byte++) memory[offset + byte] = data >> byte * 8;
    return Fault::None;
  }

  auto emit(u32 address, u16 prefix, maybe<u16> suffix = {}) -> void {
    memory[address + 0] = prefix;
    memory[address + 1] = prefix >> 8;
    if(suffix) {
      memory[address + 2] = *suffix;
      memory[address + 3] = *suffix >> 8;
    }
  }

  auto disassemble(u32 address) -> string {
    auto output = disassembleInstruction(n32{address});
    output.trimRight(" ");
    return output;
  }
};

using Family = ares::ARMv6M::InstructionFamily;

auto expectedDecode(u16 opcode) -> ares::ARMv6M::InstructionDecode {
  auto decode = [&](Family family, bool valid = true, u32 width = 2) {
    return ares::ARMv6M::InstructionDecode{family, width, valid};
  };
  if((opcode & 0xe000) == 0x0000 && (opcode & 0x1800) != 0x1800) return decode(Family::ShiftImmediate);
  if((opcode & 0xf800) == 0x1800) return decode(Family::AddSubtract);
  if((opcode & 0xe000) == 0x2000) return decode(Family::Immediate);
  if((opcode & 0xfc00) == 0x4000) return decode(Family::DataProcessing);
  if((opcode & 0xfc00) == 0x4400) return decode(Family::HighRegister);
  if((opcode & 0xf800) == 0x4800) return decode(Family::LoadLiteral);
  if((opcode & 0xf000) == 0x5000) return decode(Family::LoadStoreRegister);
  if((opcode & 0xe000) == 0x6000) return decode(Family::LoadStoreImmediate);
  if((opcode & 0xf000) == 0x8000) return decode(Family::LoadStoreHalf);
  if((opcode & 0xf000) == 0x9000) return decode(Family::LoadStoreStack);
  if((opcode & 0xf000) == 0xa000) return decode(Family::AddAddress);
  if((opcode & 0xff00) == 0xb000) return decode(Family::AdjustStack);
  if((opcode & 0xffe8) == 0xb660) return decode(Family::ChangeInterrupt);
  if(opcode == 0xb650 || opcode == 0xb658) return decode(Family::SetEndian, opcode == 0xb650);
  if((opcode & 0xff00) == 0xb200) return decode(Family::Extend);
  if((opcode & 0xfe00) == 0xb400) return decode(Family::Push);
  if((opcode & 0xff00) == 0xba00) return decode(Family::Reverse, (opcode >> 6 & 3) != 2);
  if((opcode & 0xff00) == 0xbe00) return decode(Family::Breakpoint);
  if((opcode & 0xff00) == 0xbf00) return decode(Family::Hint, !(opcode & 15) || (opcode >> 4 & 15) < 14);
  if((opcode & 0xfe00) == 0xbc00) return decode(Family::Pop);
  if((opcode & 0xf000) == 0xc000) return decode(Family::LoadStoreMultiple, (opcode & 0xff) != 0);
  if((opcode & 0xf000) == 0xd000) return decode(Family::BranchConditional, (opcode >> 8 & 15) < 14);
  if((opcode & 0xf800) == 0xe000) return decode(Family::Branch);
  if((opcode & 0xf800) == 0xf000) return decode(Family::BranchLink, true, 4);
  return decode(Family::Undefined, false);
}

auto classifierContract() -> bool {
  bool passed = true;
  u32 recognized = 0;
  for(u32 value = 0; value <= 0xffff; value++) {
    auto expected = expectedDecode(value);
    auto actual = ares::ARMv6M::decodeInstruction((u16)value);
    if(expected.family != Family::Undefined) recognized++;
    if(actual.family != expected.family || actual.width != expected.width || actual.valid != expected.valid) {
      std::cerr << "FAIL: classifier diverged at 0x" << std::hex << value << "\n";
      passed = false;
      break;
    }
  }
  passed &= expect(recognized == 59'666, "recognized first-halfword count diverged");
  auto validBL = ares::ARMv6M::decodeInstruction(0xf000, n16{0xf800});
  auto invalidBL = ares::ARMv6M::decodeInstruction(0xf000, n16{0x0000});
  passed &= expect(validBL.family == Family::BranchLink && validBL.width == 4 && validBL.valid,
    "valid BL suffix was rejected");
  passed &= expect(invalidBL.family == Family::BranchLink && invalidBL.width == 4 && !invalidBL.valid,
    "invalid BL suffix was accepted");
  return passed;
}

auto disassemblyCoverageContract() -> bool {
  Processor cpu;
  bool passed = true;
  for(u32 value = 0; value <= 0xffff; value++) {
    auto decoded = ares::ARMv6M::decodeInstruction((u16)value);
    cpu.emit(0x1000, value, decoded.width == 4 ? maybe<u16>{0xf800} : maybe<u16>{});
    auto output = cpu.disassemble(0x1000);
    auto placeholder = output == "undefined" || output == "unavailable" || output == "truncated" || !output;
    if(placeholder == decoded.valid) {
      std::cerr << "FAIL: disassembly coverage diverged at 0x" << std::hex << value
        << " with text '" << output.data() << "'\n";
      passed = false;
      break;
    }
  }
  return passed;
}

auto goldenDisassemblyContract() -> bool {
  struct Case { u16 prefix; u16 suffix; bool hasSuffix; const char* expected; };
  static const Case cases[] = {
    {0x0008, 0, false, "lsl r0,r1,#0"}, {0x0808, 0, false, "lsr r0,r1,#32"},
    {0x1008, 0, false, "asr r0,r1,#32"}, {0x1888, 0, false, "add r0,r1,r2"},
    {0x1ec8, 0, false, "sub r0,r1,#3"}, {0x2005, 0, false, "mov r0,#0x05"},
    {0x2805, 0, false, "cmp r0,#0x05"}, {0x3005, 0, false, "add r0,#0x05"},
    {0x3805, 0, false, "sub r0,#0x05"}, {0x4408, 0, false, "add r0,r1"},
    {0x4508, 0, false, "cmp r0,r1"}, {0x4608, 0, false, "mov r0,r1"},
    {0x46f7, 0, false, "mov pc,lr"}, {0x4708, 0, false, "bx r1"},
    {0x4770, 0, false, "bx lr"}, {0x4788, 0, false, "blx r1"},
    {0x5090, 0, false, "str r0,[r2,r2]"}, {0x5290, 0, false, "strh r0,[r2,r2]"},
    {0x5490, 0, false, "strb r0,[r2,r2]"}, {0x5690, 0, false, "ldsb r0,[r2,r2]"},
    {0x5890, 0, false, "ldr r0,[r2,r2]"}, {0x5a90, 0, false, "ldrh r0,[r2,r2]"},
    {0x5c90, 0, false, "ldrb r0,[r2,r2]"}, {0x5e90, 0, false, "ldsh r0,[r2,r2]"},
    {0x6048, 0, false, "str r0,[r1,#0x04]"}, {0x6848, 0, false, "ldr r0,[r1,#0x04]"},
    {0x7048, 0, false, "strb r0,[r1,#0x01]"}, {0x7848, 0, false, "ldrb r0,[r1,#0x01]"},
    {0x8048, 0, false, "strh r0,[r1,#0x02]"}, {0x8848, 0, false, "ldrh r0,[r1,#0x02]"},
    {0x9001, 0, false, "str r0,[sp,#0x004]"}, {0x9801, 0, false, "ldr r0,[sp,#0x004]"},
    {0xa001, 0, false, "adr r0,0x00001008"}, {0xa801, 0, false, "add r0,sp,#0x004"},
    {0xb001, 0, false, "add sp,#0x004"}, {0xb081, 0, false, "sub sp,#0x004"},
    {0xb662, 0, false, "cpsie i"}, {0xb672, 0, false, "cpsid i"},
    {0xb650, 0, false, "setend le"}, {0xb208, 0, false, "sxth r0,r1"},
    {0xb248, 0, false, "sxtb r0,r1"}, {0xb288, 0, false, "uxth r0,r1"},
    {0xb2c8, 0, false, "uxtb r0,r1"}, {0xb503, 0, false, "push {r0,r1,lr}"},
    {0xba08, 0, false, "rev r0,r1"}, {0xba48, 0, false, "rev16 r0,r1"},
    {0xbac8, 0, false, "revsh r0,r1"}, {0xbe5a, 0, false, "bkpt #0x5a"},
    {0xbf00, 0, false, "nop"}, {0xbf10, 0, false, "yield"},
    {0xbf20, 0, false, "wfe"}, {0xbf30, 0, false, "wfi"},
    {0xbf40, 0, false, "sev"}, {0xbf50, 0, false, "hint #0x05"},
    {0xbf08, 0, false, "it eq"}, {0xbf04, 0, false, "itt eq"},
    {0xbf0c, 0, false, "ite eq"}, {0xbd03, 0, false, "pop {r0,r1,pc}"},
    {0xc203, 0, false, "stmia r2!,{r0,r1}"}, {0xca18, 0, false, "ldmia r2!,{r3,r4}"},
    {0xd001, 0, false, "beq 0x00001006"}, {0xd0ff, 0, false, "beq 0x00001002"},
    {0xe000, 0, false, "b 0x00001004"}, {0xe7ff, 0, false, "b 0x00001002"},
    {0xf000, 0xf800, true, "bl 0x00001004"}, {0xf7ff, 0xfffe, true, "bl 0x00001000"},
    {0xb658, 0, false, "undefined"}, {0xba80, 0, false, "undefined"},
    {0xc000, 0, false, "undefined"}, {0xde00, 0, false, "undefined"},
    {0xdf00, 0, false, "undefined"}, {0xe800, 0, false, "undefined"},
    {0xf000, 0x0000, true, "undefined"},
  };

  static const char* aluNames[] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror",
    "tst", "neg", "cmp", "cmn", "orr", "mul", "bic", "mvn",
  };

  bool passed = true;
  for(auto test : cases) {
    Processor cpu;
    cpu.emit(0x1000, test.prefix, test.hasSuffix ? maybe<u16>{test.suffix} : maybe<u16>{});
    auto actual = cpu.disassemble(0x1000);
    if(actual != test.expected) {
      std::cerr << "FAIL: 0x" << std::hex << test.prefix << " expected '" << test.expected
        << "' got '" << actual.data() << "'\n";
      passed = false;
    }
  }

  for(u32 operation = 0; operation < 16; operation++) {
    Processor cpu;
    cpu.emit(0x1000, 0x4008 | operation << 6);
    string expected{aluNames[operation], " r0,r1"};
    passed &= expect(cpu.disassemble(0x1000) == expected, "data-processing mnemonic diverged");
  }

  Processor literal;
  literal.emit(0x1000, 0x4801);
  literal.memory[0x1008] = 0x12;
  literal.memory[0x1009] = 0x34;
  literal.memory[0x100a] = 0x56;
  literal.memory[0x100b] = 0x78;
  passed &= expect(literal.disassemble(0x1000) == "ldr r0,[pc,#0x00001008] =0x78563412",
    "literal address or debugger value formatting diverged");

  Processor aligned;
  aligned.emit(0x1000, 0xa001);
  passed &= expect(aligned.disassemble(0x1001) == "adr r0,0x00001008",
    "odd requested PC was not aligned before decoding");
  return passed;
}

auto observationContract() -> bool {
  Processor cpu;
  cpu.power();
  cpu.emit(0x1000, 0x2005);
  cpu.r(15) = 0x1000;
  for(u32 index = 0; index < 16; index++) cpu.r(index) = 0x1111'0000 + index;
  cpu.r(15) = 0x1000;
  cpu.psr().n = 1;
  cpu.psr().z = 0;
  cpu.psr().c = 1;
  cpu.psr().v = 0;
  cpu.psr().it = 0x18;

  serializer before;
  cpu.serialize(before);
  std::vector<u8> stateBefore(before.data(), before.data() + before.size());
  auto memoryBefore = cpu.memory;
  auto context = cpu.disassembleContext();
  auto instruction = cpu.disassembleInstruction();
  serializer after;
  cpu.serialize(after);

  bool passed = true;
  passed &= expect(context ==
    "r0:11110000 r1:11110001 r2:11110002 r3:11110003 r4:11110004 r5:11110005 r6:11110006 r7:11110007 "
    "r8:11110008 r9:11110009 r10:1111000a r11:1111000b r12:1111000c sp:1111000d lr:1111000e pc:00001000 "
    "xpsr:NzCv/it:18", "context formatting diverged");
  passed &= expect(instruction.beginsWith("mov r0,#0x05"), "default-PC disassembly diverged");
  passed &= expect(cpu.reads == 0 && cpu.writes == 0 && cpu.clocks == 0,
    "disassembly used execution callbacks or advanced time");
  passed &= expect(cpu.debuggerReads == 1, "disassembly did not use exactly one debugger read");
  passed &= expect(cpu.memory == memoryBefore, "disassembly changed memory");
  passed &= expect(after.size() == stateBefore.size()
    && !std::memcmp(after.data(), stateBefore.data(), stateBefore.size()), "disassembly changed serialized CPU state");

  Processor unavailable;
  unavailable.memory.resize(0x1000);
  passed &= expect(unavailable.disassemble(0x1000) == "unavailable", "unmapped fetch text diverged");
  Processor truncated;
  truncated.memory.resize(0x1002);
  truncated.emit(0x1000, 0xf000);
  passed &= expect(truncated.disassemble(0x1000) == "truncated", "truncated BL text diverged");
  return passed;
}

}

auto nall::main(Arguments) -> void {
  bool passed = true;
  passed &= classifierContract();
  passed &= disassemblyCoverageContract();
  passed &= goldenDisassemblyContract();
  passed &= observationContract();
  if(passed) std::cout << "PASS: ARMv6-M decode, disassembly, context, and observation contracts\n";
  std::cout.flush();
  std::cerr.flush();
  std::_Exit(passed ? 0 : 1);
}
