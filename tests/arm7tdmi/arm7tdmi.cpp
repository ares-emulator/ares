#include <nall/nall.hpp>
#include <nall/string/markup/json.hpp>
using namespace nall;

#include <nall/main.hpp>

#include <ares/ares.hpp>
#include <component/processor/arm7tdmi/arm7tdmi.hpp>

namespace Access {
enum {
  Sequential = 1,
  Code = 2,
  Dma = 4,
  Lock = 8
};
}

struct TestState {
  u32 R[16];
  u32 R_fiq[7];
  u32 R_svc[2];
  u32 R_abt[2];
  u32 R_irq[2];
  u32 R_und[2];
  u32 CPSR;
  u32 SPSR[5];
  u32 pipeline[2];
  u32 access;
};

struct Transaction {
  u32 kind;
  u32 size;
  u32 addr;
  u32 data;
  u32 cycle;
  u32 access;
};

struct TestCase {
  u32 index;
  TestState initial;
  TestState final;
  vector<Transaction> transactions;
  u32 opcode;
  u32 base_addr;
};

enum TestResult {
  pass, fail, skip
};

using TestResults = array<u32[3]>;

struct CPU : ares::ARM7TDMI {
  u32 clock = 0;
  int tindex = 0;
  vector<string> errors;
  maybe<const TestCase&> test;

  auto power(const TestCase& test) -> void {
    ARM7TDMI::power();
    clock = 0;
    tindex = 0;
    errors.reset();
    this->test = test;
  }

  auto step(u32 clocks) -> void override { clock += clocks; }
  auto sleep() -> void override {}
  auto get(u32 mode, n32 address) -> n32 override {
    if(!(mode & Prefetch)) mode |= Load;  //todo: fix this in ares
    if(mode & Prefetch && mode & Word) address &= ~3;  //test cases use incorrect address for misaligned fetches
    if(auto data = matchTransaction(mode, address)) return *data;
    error("read: mode ", hex(mode, 3L), " address ", hex(address, 8L), "\n");
    return 0;
  }
  auto getDebugger(u32 mode, n32 address) -> n32 override {
    //the ares disassembler can call this for non-opcode data (pc-relative loads),
    //some of which could be resolved by scanning the transaction log if desired.
    if(address == test->base_addr) return test->opcode;
    return 0;
  }
  auto set(u32 mode, n32 address, n32 word) -> void override {
    mode |= Store;  //todo: fix this in ares
    if(matchTransaction(mode, address, word)) return;
    error("write: mode ", hex(mode, 3L), " address ", hex(address, 8L), " word ", hex(word, 8L), "\n");
  }

  auto memSize(u32 mode) -> u32 {
    if(mode & Byte) return 1;
    if(mode & Half) return 2;
    if(mode & Word) return 4;
    print("unrecognized mode ", hex(mode, 3L), "\n");
    abort();
    return 4;
  }

  auto matchTransaction(u32 mode, n32 address, n32 word = 0) -> maybe<u32> {
    maybe<u32> result;
    u32 size = memSize(mode);
    u32 data = word & (~0u >> (32 - size * 8));
    if(tindex < test->transactions.size()) {
      auto& t = test->transactions[tindex++];
      if((mode & Prefetch) && t.kind == 0 && t.size == size && t.addr == address) {
        result = t.data;
      }
      if((mode & Load) && t.kind == 1 && t.size == size && t.addr == address) {
        result = t.data;
      }
      if((mode & Store) && t.kind == 2 && t.size == size && t.addr == address) {
        if(t.data == data) {
          result = t.data;
        } else {
          error("write: data ", hex(data, 2 * size), " != ", hex(t.data, 2 * size), ")\n");
        }
      }
      if(result) {
        //don't check sequential unless we got a result, otherwise it's just noise
        if(!(!nonsequential) != !(t.access & Access::Sequential)) {
          error("write: nonsequential ", !(!nonsequential), " != ", !(t.access & Access::Sequential), "\n");
          result = nothing;
        }
      }
    }
    return result;
  }

  auto run(const TestCase& test, bool logErrors) -> TestResult;

  template<typename... P>
  auto error(P&&... p) -> void {
    errors.append(string{std::forward<P>(p)...});
  }
} cpu;

auto CPU::run(const TestCase& test, bool logErrors) -> TestResult {
  const auto& is = test.initial;
  const auto& fs = test.final;

  power(test);

  processor.r0 = is.R[0];
  processor.r1 = is.R[1];
  processor.r2 = is.R[2];
  processor.r3 = is.R[3];
  processor.r4 = is.R[4];
  processor.r5 = is.R[5];
  processor.r6 = is.R[6];
  processor.r7 = is.R[7];
  processor.r8 = is.R[8];
  processor.r9 = is.R[9];
  processor.r10 = is.R[10];
  processor.r11 = is.R[11];
  processor.r12 = is.R[12];
  processor.r13 = is.R[13];
  processor.r14 = is.R[14];
  processor.r15 = is.R[15];
  processor.fiq.r8 = is.R_fiq[0];
  processor.fiq.r9 = is.R_fiq[1];
  processor.fiq.r10 = is.R_fiq[2];
  processor.fiq.r11 = is.R_fiq[3];
  processor.fiq.r12 = is.R_fiq[4];
  processor.fiq.r13 = is.R_fiq[5];
  processor.fiq.r14 = is.R_fiq[6];
  processor.svc.r13 = is.R_svc[0];
  processor.svc.r14 = is.R_svc[1];
  processor.abt.r13 = is.R_abt[0];
  processor.abt.r14 = is.R_abt[1];
  processor.irq.r13 = is.R_irq[0];
  processor.irq.r14 = is.R_irq[1];
  processor.und.r13 = is.R_und[0];
  processor.und.r14 = is.R_und[1];
  processor.cpsr = is.CPSR;
  processor.fiq.spsr = is.SPSR[0];
  processor.svc.spsr = is.SPSR[1];
  processor.abt.spsr = is.SPSR[2];
  processor.irq.spsr = is.SPSR[3];
  processor.und.spsr = is.SPSR[4];

  const bool thumb = processor.cpsr.t;
  const u32 length = thumb ? 2 : 4;

  //tests/v1/arm_mrs.json
  //mrs pc,...
  //test seems bugged, should reload pipeline but doesn't...
  if(!thumb && (test.opcode & 0b0000'11111011'0000'1111'0000'1111'0000) == 0b0000'00010000'0000'1111'0000'0000'0000) {
    return skip;
  }

  //tests/v1/arm_mul_mla.json
  //writes to rd should fail if said register is r15
  //r15 tests also incorrectly apply +4 offsets to rn, rm, and rs
  //since the multiplier is known to read in the same register(s?) multiple times,
  //this may result in corrupted results on hardware when the lower 8 bits of r15 are 0xfc (needs testing)
  if(!thumb && (test.opcode & 0b00001111110000000000000011110000) == 0b0000'000000'00000000000000'1001'0000) {
    if((test.opcode & 0x000F0000) == 0x000F0000) return skip;
    if((test.opcode & 0x0000F000) == 0x0000F000) return skip;
    if((test.opcode & 0x00000F00) == 0x00000F00) return skip;
    if((test.opcode & 0x0000000F) == 0x0000000F) return skip;
  }

  //tests/v1/arm_mull_mlal.json
  //writes to rl and rh should fail if said register is r15
  //r15 tests also incorrectly apply +4 offsets to rm and rs
  //since the multiplier is known to read in the same register(s?) multiple times,
  //this may result in corrupted results on hardware when the lower 8 bits of r15 are 0xfc (needs testing)
  if(!thumb && (test.opcode & 0b00001111100000000000000011110000) == 0b0000'00001'000000000000000'1001'0000) {
    if((test.opcode & 0x000F0000) == 0x000F0000) return skip;
    if((test.opcode & 0x0000F000) == 0x0000F000) return skip;
    if((test.opcode & 0x00000F00) == 0x00000F00) return skip;
    if((test.opcode & 0x0000000F) == 0x0000000F) return skip;
  }

  //tests/v1/arm_ldr_str_immediate_offset.json
  //r15 tests incorrectly apply +4 offset on writeback to rn
  if(!thumb && (test.opcode & 0b00001110000011110000000000000000) == 0b0000'010'00000'1111'0000000000000000) {
    if((test.opcode & 0x0F000000) == 0x04000000) return skip;
    if((test.opcode & 0x00020000) == 0x00020000) return skip;
  }

  //tests/v1/arm_ldr_str_register_offset.json
  //r15 tests incorrectly apply +4 offset on writeback to rn
  if(!thumb && (test.opcode & 0b00001110000011110000000000010000) == 0b0000'011'00000'1111'00000000000'0'0000) {
    if((test.opcode & 0x0F000000) == 0x04000000) return skip;
    if((test.opcode & 0x00020000) == 0x00020000) return skip;
  }

  //tests/v1/arm_swp.json
  //r15 as rn incorrectly reads from +4 offset in test data
  if(!thumb && (test.opcode & 0b00001111101111110000000011110000) == 0b0000'00010'0'001111'00000000'1001'0000) return skip;

  //tests/v1/thumb_undefined_bcc.json
  //condition code AL is invalid in Thumb mode
  if(thumb && (test.opcode & 0b1111111100000000) == 0b1101'1110'00000000) return skip;

  pipeline.reload = false;
  nonsequential = !(is.access & Access::Sequential);

  pipeline.decode.address = test.base_addr;
  pipeline.decode.instruction = is.pipeline[0];
  pipeline.decode.thumb = processor.cpsr.t;
  pipeline.decode.irq = !processor.cpsr.i;

  pipeline.fetch.address = test.base_addr + length;
  pipeline.fetch.instruction = is.pipeline[1];

  processor.r15.data -= length;

  instruction();

  const bool freload = pipeline.reload;
  const bool fthumb = processor.cpsr.t;
  const u32 flength = fthumb ? 2 : 4;

  if(pipeline.reload) reload();
  processor.r15.data += flength;

  TestResult result = pass;

  //other PSR bits are not settable on real hardware
  u32 cpsrMask = ~0x0fffff00u;

  //mul carry nyi (documented as "unpredictable")
  //thumb MUL
  if(( thumb && (test.opcode & 0b1111111111000000) == 0b0100001101000000)
  //arm UMULL
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000100'0000000000000'1001'0000)
  //arm SMLAL
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000111'0000000000000'1001'0000)
  //arm UMLAL
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000101'0000000000000'1001'0000)
  //arm SMULL
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000110'0000000000000'1001'0000)
  //arm MLA
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000001'0000000000000'1001'0000)
  //arm MUL
  || (!thumb && (test.opcode & 0b00001111111000000000000011110000) == 0b0000'0000000'0000000000000'1001'0000)
  ) {
    cpsrMask &= ~(1u << 29);  //carry
    result = skip;  //can still fail, but otherwise count as a skip
  }

  //test cases for MSR to SPSR handle upper bit of mode incorrectly
  u32 spsrMask = cpsrMask & ~0x00000010;

  //r15 bit 0 appears to always be clear on real hardware
  u32 r15Mask = ~1u;

  if(processor.r0 != fs.R[0]) error("r0: ", hex(u32(processor.r0), 8), " != ", hex(fs.R[0], 8));
  if(processor.r1 != fs.R[1]) error("r1: ", hex(u32(processor.r1), 8), " != ", hex(fs.R[1], 8));
  if(processor.r2 != fs.R[2]) error("r2: ", hex(u32(processor.r2), 8), " != ", hex(fs.R[2], 8));
  if(processor.r3 != fs.R[3]) error("r3: ", hex(u32(processor.r3), 8), " != ", hex(fs.R[3], 8));
  if(processor.r4 != fs.R[4]) error("r4: ", hex(u32(processor.r4), 8), " != ", hex(fs.R[4], 8));
  if(processor.r5 != fs.R[5]) error("r5: ", hex(u32(processor.r5), 8), " != ", hex(fs.R[5], 8));
  if(processor.r6 != fs.R[6]) error("r6: ", hex(u32(processor.r6), 8), " != ", hex(fs.R[6], 8));
  if(processor.r7 != fs.R[7]) error("r7: ", hex(u32(processor.r7), 8), " != ", hex(fs.R[7], 8));
  if(processor.r8 != fs.R[8]) error("r8: ", hex(u32(processor.r8), 8), " != ", hex(fs.R[8], 8));
  if(processor.r9 != fs.R[9]) error("r9: ", hex(u32(processor.r9), 8), " != ", hex(fs.R[9], 8));
  if(processor.r10 != fs.R[10]) error("r10: ", hex(u32(processor.r10), 8), " != ", hex(fs.R[10], 8));
  if(processor.r11 != fs.R[11]) error("r11: ", hex(u32(processor.r11), 8), " != ", hex(fs.R[11], 8));
  if(processor.r12 != fs.R[12]) error("r12: ", hex(u32(processor.r12), 8), " != ", hex(fs.R[12], 8));
  if(processor.r13 != fs.R[13]) error("r13: ", hex(u32(processor.r13), 8), " != ", hex(fs.R[13], 8));
  if(processor.r14 != fs.R[14]) error("r14: ", hex(u32(processor.r14), 8), " != ", hex(fs.R[14], 8));
  if((processor.r15 & r15Mask) != (fs.R[15] & r15Mask)) error("r15: ", hex(u32(processor.r15), 8), " != ", hex(fs.R[15], 8));
  if(processor.fiq.r8 != fs.R_fiq[0]) error("fiq.r8: ", hex(u32(processor.fiq.r8), 8), " != ", hex(fs.R_fiq[0], 8));
  if(processor.fiq.r9 != fs.R_fiq[1]) error("fiq.r9: ", hex(u32(processor.fiq.r9), 8), " != ", hex(fs.R_fiq[1], 8));
  if(processor.fiq.r10 != fs.R_fiq[2]) error("fiq.r10: ", hex(u32(processor.fiq.r10), 8), " != ", hex(fs.R_fiq[2], 8));
  if(processor.fiq.r11 != fs.R_fiq[3]) error("fiq.r11: ", hex(u32(processor.fiq.r11), 8), " != ", hex(fs.R_fiq[3], 8));
  if(processor.fiq.r12 != fs.R_fiq[4]) error("fiq.r12: ", hex(u32(processor.fiq.r12), 8), " != ", hex(fs.R_fiq[4], 8));
  if(processor.fiq.r13 != fs.R_fiq[5]) error("fiq.r13: ", hex(u32(processor.fiq.r13), 8), " != ", hex(fs.R_fiq[5], 8));
  if(processor.fiq.r14 != fs.R_fiq[6]) error("fiq.r14: ", hex(u32(processor.fiq.r14), 8), " != ", hex(fs.R_fiq[6], 8));
  if(processor.svc.r13 != fs.R_svc[0]) error("svc.r13: ", hex(u32(processor.svc.r13), 8), " != ", hex(fs.R_svc[0], 8));
  if(processor.svc.r14 != fs.R_svc[1]) error("svc.r14: ", hex(u32(processor.svc.r14), 8), " != ", hex(fs.R_svc[1], 8));
  if(processor.abt.r13 != fs.R_abt[0]) error("abt.r13: ", hex(u32(processor.abt.r13), 8), " != ", hex(fs.R_abt[0], 8));
  if(processor.abt.r14 != fs.R_abt[1]) error("abt.r14: ", hex(u32(processor.abt.r14), 8), " != ", hex(fs.R_abt[1], 8));
  if(processor.irq.r13 != fs.R_irq[0]) error("irq.r13: ", hex(u32(processor.irq.r13), 8), " != ", hex(fs.R_irq[0], 8));
  if(processor.irq.r14 != fs.R_irq[1]) error("irq.r14: ", hex(u32(processor.irq.r14), 8), " != ", hex(fs.R_irq[1], 8));
  if(processor.und.r13 != fs.R_und[0]) error("und.r13: ", hex(u32(processor.und.r13), 8), " != ", hex(fs.R_und[0], 8));
  if(processor.und.r14 != fs.R_und[1]) error("und.r14: ", hex(u32(processor.und.r14), 8), " != ", hex(fs.R_und[1], 8));
  if((processor.cpsr & cpsrMask) != (fs.CPSR & cpsrMask)) error("cpsr: ", hex(u32(processor.cpsr), 8), " != ", hex(fs.CPSR, 8));
  if((processor.fiq.spsr & spsrMask) != (fs.SPSR[0] & spsrMask)) error("fiq.spsr: ", hex(u32(processor.fiq.spsr), 8), " != ", hex(fs.SPSR[0], 8));
  if((processor.svc.spsr & spsrMask) != (fs.SPSR[1] & spsrMask)) error("svc.spsr: ", hex(u32(processor.svc.spsr), 8), " != ", hex(fs.SPSR[1], 8));
  if((processor.abt.spsr & spsrMask) != (fs.SPSR[2] & spsrMask)) error("abt.spsr: ", hex(u32(processor.abt.spsr), 8), " != ", hex(fs.SPSR[2], 8));
  if((processor.irq.spsr & spsrMask) != (fs.SPSR[3] & spsrMask)) error("irq.spsr: ", hex(u32(processor.irq.spsr), 8), " != ", hex(fs.SPSR[3], 8));
  if((processor.und.spsr & spsrMask) != (fs.SPSR[4] & spsrMask)) error("und.spsr: ", hex(u32(processor.und.spsr), 8), " != ", hex(fs.SPSR[4], 8));
  if(pipeline.decode.instruction != fs.pipeline[0]) error("pipeline[0]: ", hex(u32(pipeline.decode.instruction), 8), " != ", hex(fs.pipeline[0], 8));
  if(pipeline.fetch.instruction != fs.pipeline[1]) error("pipeline[1]: ", hex(u32(pipeline.decode.instruction), 8), " != ", hex(fs.pipeline[1], 8));
  if(nonsequential != !(fs.access & Access::Sequential)) error("nonsequential: ", nonsequential, " != ", !(fs.access & Access::Sequential));

  if(tindex != test.transactions.size()) {
    error("transactions: ", tindex, " != ", test.transactions.size(), "\n");
  }

  if(errors) {
    if(logErrors) {
      print("\n");
      print("test: ", test.index, "\n");
      print("reload: ", freload, "\n");
      print("thumb: ", thumb, "\n");
      print("fthumb: ", fthumb, "\n");
      print(hex(test.base_addr, 8L), "  ", hex(test.opcode, 2 * length), "  ", disassembleInstruction(n32(test.base_addr), boolean(thumb)), "\n");
      for(auto& error : errors) {
        print(error, "\n");
      }
      print("\n");
    }
    return fail;
  }

  return result;
}

template<typename T>
auto fromArray(const nall::Markup::Node& node, T& arr) -> void {
  for(auto n : range(node.size())) {
    node[n].value(arr[n]);
  }
}

auto fromNode(const nall::Markup::Node& node, TestState& state) -> void {
  state = {};
  fromArray(node["R"], state.R);
  fromArray(node["R_fiq"], state.R_fiq);
  fromArray(node["R_svc"], state.R_svc);
  fromArray(node["R_abt"], state.R_abt);
  fromArray(node["R_irq"], state.R_irq);
  fromArray(node["R_und"], state.R_und);
  node["CPSR"].value(state.CPSR);
  fromArray(node["SPSR"], state.SPSR);
  fromArray(node["pipeline"], state.pipeline);
  node["access"].value(state.access);
}

auto fromNode(const nall::Markup::Node& node, Transaction& transaction) -> void {
  transaction = {};
  node["kind"].value(transaction.kind);
  node["size"].value(transaction.size);
  node["addr"].value(transaction.addr);
  node["data"].value(transaction.data);
  node["cycle"].value(transaction.cycle);
  node["access"].value(transaction.access);
}

auto fromNode(const nall::Markup::Node& node, TestCase& test) -> void {
  test = {};
  fromNode(node["initial"], test.initial);
  fromNode(node["final"], test.final);
  auto transactions = node["transactions"];
  test.transactions.resize(transactions.size());
  for(auto n : range(transactions.size())) {
    fromNode(transactions[n], test.transactions[n]);
  }
  node["opcode"].value(test.opcode);
  node["base_addr"].value(test.base_addr);
}

template<typename T>
auto printArray(string name, const T& arr) -> void {
  print("  ", name, ":\n");
  for(auto x : arr) {
    print("    ", hex(x, 8L), ",\n");
  }
}

auto printState(string name, const TestState& state) -> void {
  print(name, ":\n");
  printArray("R", state.R);
  printArray("R_fiq", state.R_fiq);
  printArray("R_svc", state.R_svc);
  printArray("R_abt", state.R_abt);
  printArray("R_irq", state.R_irq);
  printArray("R_und", state.R_und);
  print("  CPSR: ", hex(state.CPSR, 8L), "\n");
  printArray("SPSR", state.SPSR);
  printArray("pipeline", state.pipeline);
  print("  access: ", hex(state.access, 8L), "\n");
}

auto printTest(const TestCase& test) -> void {
  printState("initial", test.initial);
  printState("final", test.final);
  print("transactions:\n");
  for(auto n : range(test.transactions.size())) {
    auto& t = test.transactions[n];
    print("  [", n, "]:\n");
    print("    kind: ", hex(t.kind, 8L), "\n");
    print("    size: ", hex(t.size, 8L), "\n");
    print("    addr: ", hex(t.addr, 8L), "\n");
    print("    data: ", hex(t.data, 8L), "\n");
    print("    cycle: ", hex(t.cycle, 8L), "\n");
    print("    access: ", hex(t.access, 8L), "\n");
  }
  print("opcode: ", hex(test.opcode, 8L), "\n");
  print("base_addr: ", hex(test.base_addr, 8L), "\n");
}

auto printResults(const TestResults& results) -> void {
  print("pass ", results[pass], " / fail ", results[fail], " / skip ", results[skip], "\n");
}

auto test(string path) -> TestResults {
  auto root = JSON::unserialize(string::read(path));
  if(!root) {
    print("failed to read ", path, "\n");
    return {};
  }

  print(path, "\n");

  TestResults results = {};
  bool logErrors = true;
  for(auto n : range(root.size())) {
    TestCase test;
    fromNode(root[n], test);
    test.index = n;

    //printTest(test);

    auto result = cpu.run(test, logErrors);
    results[result]++;
    if(result == fail) {
      logErrors = false;  //don't print subsequent failures
    }
  }

  printResults(results);
  return results;
}

auto addDirectory(std::vector<string>& files, string location) -> void {
  auto filenames = directory::files(location, "*.json");
  if(filenames.empty()) {
    print("no tests found in ", location, "\n");
    return;
  }
  for(auto& filename : filenames) {
    files.push_back({location, filename});
  }
}

auto nall::main(Arguments arguments) -> void {
  std::vector<string> files;

  if(arguments) {
    for(auto argument : arguments) {
      if(directory::exists(argument)) {
        addDirectory(files, argument);
      } else if(file::exists(argument)) {
        files.push_back(argument);
      } else {
        print("unknown argument: ", argument, "\n");
      }
    }
  } else {
    addDirectory(files, "tests/v1/");
  }

  TestResults totals = {};
  for(auto& file : files) {
    auto results = test(file);
    for(auto n : range(3)) totals[n] += results[n];
  }

  if(files.size() > 1) {
    print("\nTOTAL\n");
    printResults(totals);
  }
}
