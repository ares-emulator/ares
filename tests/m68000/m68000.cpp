#include <nall/nall.hpp>
#include <nall/map.hpp>
#include <nall/string/markup/json.hpp>
using namespace nall;

#include <nall/main.hpp>

#include <ares/ares.hpp>
#include <component/processor/m68000/m68000.hpp>

template<typename T, typename U> struct MemMap {
  auto reset() -> void {
    _m.reset();
  }

  auto operator[](T index) -> U& {
    if(auto found = _m.find(index)) return found();
    _m.insert(index, {});
    return _m.find(index)();
  }

  map<T, U> _m;
};

struct TestState {
  u32 a[8];
  u32 d[8];
  u32 pc;
  u32 sr;
  u32 usp;
  u32 ssp;
  array<u32[2]> prefetch;
  std::vector<array<u32[2]>> ram;
};

struct TestCase {
  string name;
  TestState initial;
  TestState final;
  u32 length;
};

enum TestResult {
  pass, fail, skip
};

using TestResults = array<u32[3]>;

struct CPU : ares::M68000 {
  u32 clock = 0;
  MemMap<u32, u8> memory;

  auto power() -> void {
    M68000::power();
    clock = 0;
    memory.reset();
  }

  auto idle(u32 clocks) -> void override { clock += clocks; }
  auto wait(u32 clocks) -> void override { clock += clocks; }
  auto read(n1 upper, n1 lower, n24 address, n16 /*data*/) -> n16 override {
    n16 data;
    if(lower) data.byte(0) = memory[address | 1];
    if(upper) data.byte(1) = memory[address & ~1];
    return data;
  }
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    if(lower) memory[address | 1] = data.byte(0);
    if(upper) memory[address & ~1] = data.byte(1);
  }
  auto lockable() -> bool override { return true; }  //tests expect working bus lock

  auto run(const TestCase& test, bool logErrors) -> TestResult;
} cpu;

auto CPU::run(const TestCase& test, bool logErrors) -> TestResult {
  power();

  const auto& is = test.initial;
  for(auto n : range(8)) {
    r.a[n] = is.a[n];
    r.d[n] = is.d[n];
  }
  r.sp = is.usp;
  r.pc = is.pc;
  writeSR(is.sr);  //this may swap a[7] and sp

  const auto& im = is.ram;
  for(auto n : range(im.size())) {
    memory[im[n][0]] = im[n][1];
  }

  const auto& fs = test.final;
  //check if final pc is at address error vector
  if(fs.pc - 4 == (memory[0xc] << 24 | memory[0xd] << 16 | memory[0xe] << 8 | memory[0xf])) {
    return skip;  //address errors nyi
  }

  r.ir  = is.prefetch[0];
  r.irc = is.prefetch[1];

  instruction();

  std::vector<string> errors;
  auto error = [&](auto&&... p) -> void {
    errors.push_back(string{std::forward<decltype(p)>(p)...});
  };

  if(!r.s) swap(r.a[7], r.sp);  //swap back to match test format

  for(auto n : range(8)) {
    if(r.a[n] != fs.a[n]) {
      error("a", n, ": ", hex(r.a[n], 8), " != ", hex(fs.a[n], 8));
    }
    if(r.d[n] != fs.d[n]) {
      error("d", n, ": ", hex(r.d[n], 8), " != ", hex(fs.d[n], 8));
    }
  }
  if(r.sp != fs.usp) {
    error("usp: ", hex(r.sp, 8), " != ", hex(fs.usp, 8));
  }
  if(r.pc != fs.pc) {
    error("pc: ", hex(r.pc, 8), " != ", hex(fs.pc, 8));
  }
  auto sr = readSR();
  auto srMask = ~0u;  //restrict flag comparison if desired
  if((sr & srMask) != (fs.sr & srMask)) {
    error("sr: ", hex(sr, 4), " != ", hex(fs.sr, 4));
  }
  if(r.ir != fs.prefetch[0]) {
    error("prefetch[0]: ", hex(r.ir, 4), " != ", hex(fs.prefetch[0], 4));
  }
  if(r.irc != fs.prefetch[1]) {
    error("prefetch[1]: ", hex(r.irc, 4), " != ", hex(fs.prefetch[1], 4));
  }
  if(clock != test.length) {
    error("length: ", clock, " != ", test.length);
  }

  //todo: detect missed/extra writes; should probably just use transaction log
  const auto& fm = fs.ram;
  for(auto n : range(fm.size())) {
    u32 addr = fm[n][0];
    u8 data = fm[n][1];
    u8 actual = memory[addr];
    if(actual != data) {
      error(hex(addr, 8), ": ", hex(actual, 2), " != ", hex(data, 2));
    }
  }

  if(!errors.empty()) {
    if(logErrors) {
      print("\n");
      print(test.name, "\n");
      print(disassembleInstruction(is.pc - 4), "\n");
      for(auto& error : errors) {
        print(error, "\n");
      }
      print("\n");
    }
    return fail;
  }

  return pass;
}

auto fromNode(const nall::Markup::Node& node, TestState& state) -> void {
  state = {};
  node["a0"].value(state.a[0]);
  node["a1"].value(state.a[1]);
  node["a2"].value(state.a[2]);
  node["a3"].value(state.a[3]);
  node["a4"].value(state.a[4]);
  node["a5"].value(state.a[5]);
  node["a6"].value(state.a[6]);
  node["ssp"].value(state.a[7]);
  node["d0"].value(state.d[0]);
  node["d1"].value(state.d[1]);
  node["d2"].value(state.d[2]);
  node["d3"].value(state.d[3]);
  node["d4"].value(state.d[4]);
  node["d5"].value(state.d[5]);
  node["d6"].value(state.d[6]);
  node["d7"].value(state.d[7]);
  node["pc"].value(state.pc);
  node["sr"].value(state.sr);
  node["usp"].value(state.usp);
  auto prefetch = node["prefetch"];
  prefetch[0].value(state.prefetch[0]);
  prefetch[1].value(state.prefetch[1]);
  auto ram = node["ram"];
  state.ram.resize(ram.size());
  for(auto n : range(ram.size())) {
    ram[n][0].value(state.ram[n][0]);
    ram[n][1].value(state.ram[n][1]);
  }
}

auto fromNode(const nall::Markup::Node& node, TestCase& test) -> void {
  test = {};
  node["name"].value(test.name);
  fromNode(node["initial"], test.initial);
  fromNode(node["final"], test.final);
  //todo: transaction log
  node["length"].value(test.length);
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
