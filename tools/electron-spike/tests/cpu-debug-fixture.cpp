// Core assertions adapted from ares-64 tests/mcp/live_fixture.py at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. No ROM or live user machine.
#include <n64/n64.hpp>
#include "../native/n64-service.hpp"
#include "../native/input-service.hpp"
#include <cstdio>
#include <cstdlib>
using namespace ares::Nintendo64;
using Debug = CPU::DebugControl;
static void check(bool ok, const char* name) {
  if(!ok) { std::fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
}
static constexpr uint64_t base = 0xffffffff80001000ull;
static constexpr uint64_t data = 0xffffffff80002000ull;
static void pc(uint64_t address) { cpu.pipeline.setPc(address); }
static void word(unsigned offset, uint32_t value) {
  rdram.ram.write<Word>(0x1000 + offset, value, RBusDevice::ARES_DEBUGGER);
}
static uint32_t mem(unsigned op, unsigned rt, unsigned offset) {
  return op << 26 | 8 << 21 | rt << 16 | offset;
}
static void steps(unsigned count) {
  auto& d = cpu.debugControl;
  d.resume(); d.requestStepInstructions(count);
  for(unsigned i = 0; i <= count && !d.halted; ++i) cpu.instruction();
  check(d.halted, "bounded instruction stop");
}
int main() {
  std::vector<uint8_t> hidden(2 * 1024 * 1024);
  rdram.hidden.data = hidden.data();
  rdram.ram.allocate(4 * 1024 * 1024); rdram.mapIdentity = 1; rdram.ram.fill();
  auto root = std::make_shared<ares::Core::Object>("CPU fixture");
  // Task 6 input foundation: real ares controller nodes, deterministic clocks.
  InputService input; input.available = true;
  const auto now = InputService::Clock::now();
  auto set = [&](const char* port, const char* mask, const char* frames, const char* milliseconds, const char* owner) {
    return input.execute("n64.input.set", {port, mask, "1234", "-2345", frames, milliseconds}, owner, 1, input.frame, "", now);
  };
  input.service(1, 10, now);
  check(set("1", "1", "2", "0", "owner-a").error.empty(), "input port acquired");
  check(!input.latched[0].overridden(), "new input waits for VI latch");
  input.service(1, 10, now);
  check(!input.latched[0].overridden(), "same VI cannot relatch pending input");
  input.service(1, 11, now);
  check(input.latched[0].buttons == 1 && input.latched[0].x == 1234, "next VI latches held input");
  check(set("1", "2", "0", "0", "owner-b").error == "busy", "foreign port acquisition rejected");
  check(set("2", "2", "0", "0", "owner-b").error.empty(), "independent port ownership");
  input.releaseOwner("owner-a");
  check(!input.pending[0].overridden() && !input.latched[0].overridden() && input.pending[1].owner == "owner-b", "disconnect revokes only owner ports immediately");
  input.start(1, 11);
  check(input.latched[1].buttons == 2 && input.operationRunning, "operation start latches pending input");
  auto port = root->append<ares::Node::Port>("Controller Port 2");
  auto pad = port->append<ares::Node::Peripheral>("Gamepad");
  auto button = pad->append<ares::Node::Input::Button>("B");
  auto axis = pad->append<ares::Node::Input::Axis>("X-Axis");
  auto motor = pad->append<ares::Node::Input::Rumble>("Rumble");
  check(input.apply(button, 1, 11) && button->value() == 1, "injection reaches real controller button");
  check(input.apply(axis, 1, 11) && axis->value() == 1234, "injection reaches real controller axis");
  check(!input.apply(motor, 1, 11), "rumble remains host output");
  input.releaseOwner("owner-b"); button->setValue(1);
  check(input.apply(button, 1, 11) && button->value() == 0, "unoverridden host input neutral during operation");
  input.neutralDuringOperations = false;
  check(!input.apply(button, 1, 11), "opt-in host mixing passes through");
  input.finish(1, 11);
  check(set("3", "4", "1", "0", "owner-c").error.empty(), "frame lease acquired");
  input.service(1, 11, now, true);
  check(input.pending[2].overridden(), "frame lease holds until native target VI");
  input.service(1, 12, now);
  check(!input.pending[2].overridden(), "frame lease expires against native VI coordinate");
  check(set("4", "8", "0", "5", "owner-d").error.empty(), "wall-time lease acquired");
  input.service(1, 12, now, true);
  input.service(1, 12, now + std::chrono::milliseconds(6));
  check(!input.pending[3].overridden() && !input.latched[3].overridden(), "wall lease expires while paused at same frame");
  check(set("1", "1", "0", "0", "owner-a").error.empty(), "unbounded hold acquired");
  input.start(1, 12);
  check(input.takeover() && !input.hasOverrides() && !input.operationRunning && input.takeovers == 1, "human takeover revokes held and operation input");
  check(!input.takeover(), "neutral duplicate input does not create takeover");
  check(set("1", "1", "0", "0", "owner-a").error.empty(), "epoch fixture acquired");
  input.service(2, 0, now);
  check(!input.hasOverrides(), "epoch replacement revokes every hold");
  cpu.load(root);
  cpu.power(false);
  cpu.recompiler.enabled = true;
  cpu.scc.configuration.bigEndian = 1; cpu.context.setMode();
  auto& d = cpu.debugControl;
  nall::string error; uint32_t physical = 0;
  for(uint64_t address : {0x2000ull, 0x80002000ull, 0xa0002000ull, data, 0xffffffffa0002000ull})
    { check(d.normalizeAddress(address, 4, physical, error) && physical == 0x2000, "physical/KSEG aliases"); }
  for(uint64_t address : {0xc0002000ull, 0x100000000ull, 0xffffffffc0002000ull, 0x400000ull, ~0ull})
    check(!d.normalizeAddress(address, 4, physical, error), "unsupported addresses rejected");
  check(!d.normalizeAddress(0x3fffff, 2, physical, error), "range cannot wrap installed RAM");
  check(!d.normalizeAddress(0, 0, physical, error), "empty range rejected");
  cpu.ipu.r[8].u64 = data; cpu.ipu.r[9].u64 = 0x11223344;
  word(0, mem(0x2b, 9, 0));       // sw t1,0(t0)
  word(4, mem(0x23, 10, 0));      // lw t2,0(t0)
  word(8, mem(0x22, 11, 1));      // lwl t3,1(t0)
  word(12, mem(0x26, 11, 4));     // lwr t3,4(t0)
  word(16, 0x10000002);           // beq zero,zero,+2
  word(20, 0x240d0001);           // addiu t5,zero,1 (delay)
  word(24, 0x240e0002);           // skipped
  word(28, 0);                   // nop
  pc(base);
  auto watch = d.addWatchpoint(0xa0002000, 4, Debug::AccessWrite, {}, error);
  check(watch != 0, "write watch added via KSEG1");
  steps(20);
  check(d.stop.reason == Debug::StopReason::Watchpoint, "watchpoint wins over remaining steps");
  check(d.stop.access.pc == base && d.stop.snapshot.pc == base + 4, "write hit is post-instruction");
  check(d.stop.access.value == 0x11223344 && d.stop.access.size == 4 && d.stop.access.hasOld && d.stop.access.oldValue == 0, "write value and old bytes");
  auto frozen = d.stop;
  d.removeWatchpoint(watch);
  steps(1);
  check(cpu.ipu.r[10].u64 == 0x11223344, "load uses dirty CPU cache");
  check(frozen.snapshot.pc == base + 4 && frozen.snapshot.gpr[10] == 0, "prior snapshot immutable");
  auto excluded = d.addWatchpoint(data, 1, Debug::AccessRead, {}, error);
  auto included = d.addWatchpoint(data + 1, 1, Debug::AccessRead, {}, error);
  steps(1);
  check(d.stop.access.watchId == included && d.stop.access.size == 3 && d.stop.access.paddr == 0x2001 && d.stop.access.value == 0x223344, "LWL semantic bytes only");
  check(d.watchpoints[0].hits == 0, "LWL excludes aligned container prefix");
  d.removeWatchpoint(excluded); d.removeWatchpoint(included);
  included = d.addWatchpoint(data + 4, 1, Debug::AccessRead, {}, error);
  excluded = d.addWatchpoint(data + 5, 1, Debug::AccessRead, {}, error);
  steps(1);
  check(d.stop.access.watchId == included && d.stop.access.size == 1 && d.watchpoints[1].hits == 0, "LWR semantic bytes only");
  d.removeWatchpoint(excluded); d.removeWatchpoint(included);
  steps(1);
  check(d.stop.snapshot.pc == base + 20 && d.stop.snapshot.delaySlot, "branch stops with pending delay slot");
  steps(1);
  check(d.stop.snapshot.pc == base + 28 && !d.stop.snapshot.delaySlot && cpu.ipu.r[13].u64 == 1 && cpu.ipu.r[14].u64 == 0, "delay slot is one instruction");
  auto breakpoint = d.addBreakpoint(0xa000101c, {}, error);
  d.resume(); cpu.instruction();
  check(d.halted && d.stop.reason == Debug::StopReason::Breakpoint && d.stop.breakpointId == breakpoint && d.stop.snapshot.pc == base + 28, "alias breakpoint stops before fetch");
  steps(1);
  check(d.stop.reason == Debug::StopReason::InstructionsDone && d.stop.snapshot.pc == base + 32, "resume suppresses same breakpoint once");
  d.removeBreakpoint(breakpoint); d.resume();
  check(!d.active && !d.gate && !d.watchActive && cpu.recompiler.enabled, "debug disable restores executor eligibility");
  // Cache helpers must neither fill on a miss nor write back a dirty resident line.
  auto& line = cpu.dcache.line(data);
  check(line.hit(0x2000) && line.dirty, "fixture has resident dirty cache line");
  check(rdram.ram.read<Word>(0x2000, RBusDevice::ARES_DEBUGGER) == 0, "CPU store stays out of physical RDRAM");
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x11, "CPU view sees dirty bytes");
  cpu.dcache.writeDebug<Byte>(data, 0x2000, 0x55);
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55 && rdram.ram.read<Byte>(0x2000, RBusDevice::ARES_DEBUGGER) == 0, "CPU-view write updates cache only");
  auto& miss = cpu.dcache.line(data + 16);
  check(!miss.valid(), "miss starts absent");
  cpu.dcache.writeDebug<Byte>(data + 16, 0x2010, 0xab);
  check(!miss.valid() && cpu.dcache.readDebug<Byte>(data + 16, 0x2010) == 0xab, "debug miss writes RDRAM without allocation");
  check((d.snapshot().cop0Valid & 2) == 0, "snapshot omits side-effectful Random register");
  auto cpuRead = N64Service::memory("n64.memory.read", {"0x80002000", "cpu", "1"}, false);
  check(cpuRead.error.empty() && cpuRead.json.find("\"hex\":\"55\"") != std::string::npos, "strict service CPU view");
  auto uncached = N64Service::memory("n64.memory.read", {"0xffffffffa0002000", "cpu", "1"}, false);
  check(uncached.error.empty() && uncached.json.find("\"hex\":\"00\"") != std::string::npos, "strict service KSEG1 bypass");
  auto patch = N64Service::memory("n64.memory.write", {"0x2000", "rdram", "77"}, false);
  check(patch.error.empty() && patch.json.find("\"dcache_dirty_lines_overlapping\":1") != std::string::npos, "physical patch reports dirty-cache hazard");
  check(rdram.ram.read<Byte>(0x2000, RBusDevice::ARES_DEBUGGER) == 0x77 && cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55, "physical patch preserves resident cache");
  check(N64Service::memory("n64.memory.write", {"0x3fffff", "rdram", "1122"}, false).error == "out_of_bounds", "service prevalidates whole range");
  check(N64Service::memory("n64.memory.write", {"0x2000", "cpu", "99zz"}, false).error == "invalid_request", "service prevalidates all hex bytes");
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55, "invalid write makes no partial change");
  // Exercise the actual JIT dispatch again after removing the last debug hook.
  word(0x80, 0x1000ffff); word(0x84, 0);
  cpu.icache.power(false); pc(base + 0x80);
  cpu.instruction();
  check(cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1080)] != nullptr, "ordinary dispatch actually creates a recompiler block again");
  rdram.ram.reset();
  std::puts("CPU_DEBUG_FIXTURE_PASS");
}
