// Core assertions adapted from ares-64 tests/mcp/live_fixture.py at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. No ROM or live user machine.
#include <n64/n64.hpp>
#include "../native/n64-service.hpp"
#include "../native/machine-service.hpp"
#include "../native/stack-service.hpp"
#include "../native/call-service.hpp"
#include "../native/viewer-service.hpp"
#include "../native/inspection-service.hpp"
#include "../native/rsp-debug-service.hpp"
#include "../native/memory-experiment.hpp"
#include "../native/input-service.hpp"
#include "../native/trace-service.hpp"
#include "../native/dma-service.hpp"
#include "../native/profiler-service.hpp"
#include "../native/tap-service.hpp"
#include "../native/frame-service.hpp"
#include "../native/audio-service.hpp"
#include "../native/recording-service.hpp"
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
  {
    RecordingService video;
    std::vector<uint8_t> rgba{255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    check(video.execute("video.record.start", {"2", "1048576", "60", "1000", "0"}, "one", 1, 0).error.empty(), "record start");
    video.capture(rgba.data(), 2, 2, 1, 1); video.capture(rgba.data(), 2, 2, 1, 2); video.capture(rgba.data(), 2, 2, 1, 3);
    check(video.frames.size() == 2 && video.reason == "frames_limit", "record bounded frame limit");
    check(video.frames[0] == std::vector<uint8_t>({255,0,0,255,255,255,0,0, 0,0,255,0,255,0,0,0}), "record bottom-up BGR row padding");
    check(video.execute("video.record.begin", {}, "two", 1, 2).error == "busy", "record foreign export");
    video.execute("video.record.begin", {}, "one", 1, 2); auto id = std::to_string(video.pin);
    check(video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2).error == "busy", "record pin prevents discard");
    check(video.execute("video.record.read", {id, "31", "2"}, "one", 1, 2).error == "out_of_bounds", "record read bounds");
    video.execute("video.record.release", {id, "0"}, "one", 1, 2);
    check(video.available && video.frames.size() == 2, "failed video export retains frames");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2);
    video.capture(rgba.data(), 2, 2, 2, 3); check(video.reason == "epoch_changed", "record old epoch rejected");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2);
    video.capture(rgba.data(), 2, 2, 1, 3); video.capture(rgba.data(), 3, 2, 1, 4);
    check(video.reason == "dimensions_changed" && video.frames.size() == 1, "record dimension stop");
    video.releaseOwner("one"); check(!video.available && video.frames.empty(), "record disconnect cleanup");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "0"}, "one", 1, 0);
    video.deadline = RecordingService::Clock::now(); video.execute("video.record.status", {}, "two", 1, 0);
    check(video.reason == "seconds_limit", "record wall deadline");
    video.reset(); video.execute("video.record.start", {"10", "1048576", "60", "1000", "0"}, "one", 1, 0);
    std::vector<uint8_t> large(512 * 512 * 4);
    video.capture(large.data(), 512, 512, 1, 1); video.capture(large.data(), 512, 512, 1, 2);
    check(video.reason == "bytes_limit" && video.frames.size() == 1, "record byte bound");
  }

  {
    AudioService audio;
    check(audio.execute("audio.capture", {"2"}, "one", false).error == "unavailable", "audio unavailable without streams");
    check(audio.execute("audio.capture", {"2"}, "one", true).error.empty(), "audio capture start");
    audio.push(2, -2);
    check(audio.samples == std::vector<uint8_t>({0xff, 0x7f, 0x01, 0x80}), "audio stereo PCM clamp/endian");
    auto pin = audio.execute("audio.begin", {}, "one", true);
    check(pin.error.empty() && audio.pin->bytes.size() == 4, "partial audio pin");
    auto id = std::to_string(audio.pin->id);
    audio.push(0.5, -0.5); audio.push(1, 1);
    check(audio.samples.size() == 8 && audio.pin->bytes.size() == 4, "audio cap freezes and pin immutable");
    check(audio.execute("audio.capture", {"0"}, "one", true).error == "busy", "audio pin prevents capture replacement");
    check(audio.execute("audio.read", {id, "0", "4"}, "two", true).error == "busy", "audio foreign read rejected");
    check(audio.execute("audio.capture", {"0"}, "two", true).error == "busy", "audio owner retained");
    check(audio.execute("audio.release", {id, "0"}, "one", true).error.empty() && audio.active, "failed save retains capture");
    audio.execute("audio.begin", {}, "one", true); id = std::to_string(audio.pin->id);
    check(audio.execute("audio.read", {id, "7", "2"}, "one", true).error == "out_of_bounds", "audio read bounds");
    audio.execute("audio.release", {id, "1"}, "one", true);
    check(!audio.active && audio.samples.empty(), "saved capture consumed");
    audio.execute("audio.capture", {"0"}, "one", true); audio.push(1, 1);
    check(audio.samples.empty() && audio.active, "zero audio cap is empty completed capture");
    audio.releaseOwner("two"); check(audio.active, "foreign disconnect preserves audio");
    audio.releaseOwner("one"); check(!audio.active && audio.cap == 0, "owner disconnect revokes audio");
    audio.execute("audio.capture", {"192001"}, "one", true);
    check(!audio.active, "oversized audio capture rejected");
  }

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
  {
    ExecutionService ex; ex.supported = true; bool paused = true;
    auto many = [&](std::vector<std::string> args) { return MemoryExperiment::execute(args, "test", 1, true, ex, paused); };
    check(many({"0", "1000", "0x2010", "rdram", "11", "0x3fffff", "rdram", "1122"}).error == "out_of_bounds", "many validates later address before writing");
    check(rdram.ram.read<Byte>(0x2010, RBusDevice::ARES_DEBUGGER) == 0xab, "many invalid tail leaves first byte unchanged");
    check(many({"1", "0", "0x2010", "rdram", "11"}).error == "invalid_request", "many validates deadline before writing");
    check(many({"0", "1000", "0x2010", "rdram", "11", "0x2011", "cpu", "zz"}).error == "invalid_request", "many validates later hex");
    auto overlap = many({"0", "1000", "0x2010", "rdram", "1122", "0x2011", "rdram", "AA"});
    check(overlap.error.empty() && overlap.json.find("\"matches\":[false,true]") != std::string::npos, "many readbacks follow all overlapping writes");
    auto run = many({"2", "1000", "0x2010", "rdram", "BB"});
    check(run.error.empty() && ex.active && ex.active->kind == "verify_writes", "delayed verification starts owned run");
    check(many({"0", "1000", "0x2010", "rdram", "00"}).error == "busy", "active verification excludes another write");
    rdram.ram.write<Byte>(0x2010, 0xcc, RBusDevice::ARES_DEBUGGER);
    ex.frame = 1; auto final = ex.complete("watchpoint", paused);
    check(final.find("\"hex\":\"cc\"") != std::string::npos && final.find("\"matches\":[false]") != std::string::npos, "early stop freezes delayed readback");
    rdram.ram.write<Byte>(0x2010, 0xdd, RBusDevice::ARES_DEBUGGER);
    check(ex.execute("execution.status", {"1"}, "test", 1, paused).json == final, "final verification remains immutable after later changes");
    run = many({"1", "1000", "0x2010", "rdram", "EE"}); ex.frame = 2; ex.service(paused);
    check(!ex.active && ex.finished.back().second.find("\"matches\":[true]") != std::string::npos, "VI target verifies at completion");
    run = many({"1", "1000", "0x2010", "rdram", "FF"}); ex.active->deadline = ExecutionService::Clock::now(); ex.service(paused);
    check(!ex.active && ex.finished.back().second.find("\"stop_reason\":\"timeout\"") != std::string::npos, "verification timeout reports actual stop");
  }
  // Exercise the actual JIT dispatch again after removing the last debug hook.
  word(0x80, 0x1000ffff); word(0x84, 0);
  cpu.icache.power(false); pc(base + 0x80);
  cpu.instruction();
  check(cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1080)] != nullptr, "ordinary dispatch actually creates a recompiler block again");
  // PC histogram buckets count actual instructions and bound retained records.
  executionCoordinate.frame = 10; pc(base + 0x80);
  d.beginTrace(true, false, 10, 12, base, base + 0x200, 1);
  steps(4); d.endTrace(); d.resume();
  check(d.trace.pcRecords.size() == 1 && d.trace.pcRecords[0].frame == 10 && d.trace.pcRecords[0].count == 2, "PC histogram frame bucket and count");
  check(d.trace.truncated && d.trace.droppedRecords == 1 && d.trace.totalInstructions == 4 && d.trace.instructionsObserved == 4, "trace record cap preserves counters");
  check(d.trace.counters.capacity() == 0 && d.trace.vaddrs.capacity() == 0 && !d.active, "trace disable releases histogram storage and interpreter gate");
  pc(base + 0x80);
  d.beginTrace(true, false, 10, 11, base + 0x80, base + 0x84, 4);
  steps(2); d.endTrace(); d.resume();
  check(d.trace.pcRecords.size() == 1 && d.trace.totalInstructions == 1 && d.trace.instructionsObserved == 2, "half-open executing-PC filter");
  d.beginTrace(true, false, 10, 11, 0, ~0ull, 1);
  cpu.debugTraceInstruction(0xffffffffbfc00000ull, 0x1fc00000, 0);
  cpu.debugTraceInstruction(0xffffffffbfc00004ull, 0x1fc00004, 0);
  check(d.trace.other.size() == 1 && d.trace.overflowObservations == 1 && d.trace.truncated, "non-RDRAM histogram map is bounded before VI flush");
  d.endTrace(); d.resume();
  d.beginTrace(true, false, 10, 11, 0, ~0ull, 1);
  auto index = 0x1080u >> 2;
  d.trace.counters[index] = ~0u - 1; d.trace.touched.push_back(index); d.trace.vaddrs[index] = base + 0x80;
  cpu.debugTraceInstruction(base + 0x80, 0x1080, 0);
  cpu.debugTraceInstruction(base + 0x80, 0x1080, 0);
  check(d.trace.counters[index] == ~0u && d.trace.touched.size() == 1 && d.trace.overflowObservations == 1, "histogram counters saturate with explicit overflow");
  d.endTrace(); d.resume();
  // JALR t8,t8 must capture the old target before t8 receives the link address.
  word(0x100, (24u << 21) | (24u << 11) | 9); word(0x104, 0);
  word(0x180, (24u << 21) | 8); word(0x184, 0);
  cpu.icache.power(false); pc(base + 0x100); cpu.ipu.r[24].u64 = base + 0x180;
  d.beginTrace(false, true, 10, 11, base + 0x100, base + 0x104, 4);
  d.trace.registerMask = (1u << 24) | (1u << 29); d.trace.captureF12 = true;
  d.trace.targetLow = base + 0x180; d.trace.targetHigh = base + 0x184;
  steps(4); d.endTrace(); d.resume();
  check(d.trace.callRecords.size() == 1 && d.trace.callsObserved == 1, "linked-call trace excludes plain JR");
  auto call = d.trace.callRecords[0];
  check(call.pc == base + 0x100 && call.target == base + 0x180 && call.linkRegister == 24 && call.linkValue == base + 0x108, "JALR target uses pre-instruction source");
  check(call.registers[24] == base + 0x180 && cpu.ipu.r[24].u64 == base + 0x108, "call argument snapshot precedes link write");
  TraceService view; view.available = true; view.calls = true; view.operationId = 99; view.epoch = 1;
  view.startFrame = 10; view.endFrame = 11; view.maxRecords = 4; view.registerMask = (1u << 24) | (1u << 29); view.f12 = true;
  view.finish("instructions_done");
  auto callPage = view.page({"99", "0", "64"}, 1);
  check(callPage.error.empty(), "real linked-call record serializes through native trace service");
  std::printf("TRACE_CALL_PAGE %s\n", callPage.json.c_str());

  pc(base + 0x100); cpu.ipu.r[24].u64 = base + 0x180;
  d.beginTrace(false, true, 10, 11, base + 0x100, base + 0x104, 4);
  d.trace.targetLow = base + 0x1a0; d.trace.targetHigh = base + 0x1a4;
  steps(1); d.endTrace(); d.resume();
  check(d.trace.callRecords.empty() && d.trace.callsObserved == 1 && d.trace.excludedTarget == 1, "target filter reports excluded calls");
  check(!d.active, "call-trace disable restores normal executor eligibility");
  auto observed = d.trace.instructionsObserved;
  cpu.recompiler.reset(); pc(base + 0x80);
  for(unsigned n = 0; n < 3; ++n) cpu.instruction();
  auto section = cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1080)];
  check(section && section->blocks[cpu.recompiler.blockIndex(0x1080)] && !d.active && d.trace.instructionsObserved == observed, "trace disable returns to real JIT blocks without capturing");
  // An actual SW to PI_WR_LEN starts the transfer; metadata must match bytes.
  pi.load(root); pi.power(false); cartridge.rom.allocate(16, 0);
  for(unsigned n = 0; n < 16; ++n) cartridge.rom.data[n ^ 3] = 0x80 + n;
  pi.attach(cartridge.romDevice, 0);
  dmaProvenance.enabled = true; dmaProvenance.reset(7);
  word(0x200, 0x3c08a460); word(0x204, 0x24093000); word(0x208, mem(0x2b, 9, 0));
  word(0x20c, 0x3c091000); word(0x210, mem(0x2b, 9, 4)); word(0x214, 0x24090007); word(0x218, mem(0x2b, 9, 12));
  auto dmaWatch = d.addWatchpoint(0x3000, 8, Debug::AccessWrite, {}, error);
  cpu.icache.power(false); pc(base + 0x200); steps(7);
  check(d.stop.reason == Debug::StopReason::InstructionsDone && d.watchpoints[0].hits == 0, "PI DMA is provenance, not a CPU data-watch hit");
  d.removeWatchpoint(dmaWatch);
  check(dmaProvenance.records.size() == 1 && dmaProvenance.current == nullptr, "CPU-initiated PI DMA captured once");
  auto transfer = dmaProvenance.records[0];
  check(transfer.pc == base + 0x218 && transfer.epoch == 7 && transfer.frame == 10 && transfer.fromRom && transfer.romOffset == 0, "DMA initiating PC, epoch, frame and ROM offset");
  check(transfer.requestedLength == 8 && transfer.bytesWritten == 8 && transfer.bytesAttempted == 8 && transfer.ranges.size() == 1 && transfer.ranges[0].address == 0x3000 && transfer.ranges[0].length == 8, "DMA metadata matches aligned write range");
  for(unsigned n = 0; n < 8; ++n) check(rdram.ram.read<Byte>(0x3000 + n, RBusDevice::ARES_DEBUGGER) == 0x80 + n, "real PI bytes match original ROM pattern");
  pi.power(false); pi.io.pbusAddress = 0x10000000; pi.io.dramAddress = 0x3012; pi.io.writeLength = 7; pi.io.originPc = base + 0x218;
  pi.dmaWrite();
  auto partial = dmaProvenance.records.back();
  check(partial.requestedLength == 8 && partial.bytesWritten == 6 && partial.ranges[0].address == 0x3012 && partial.ranges[0].length == 6, "PI misalignment records actual short first-block writes");
  for(unsigned n = 0; n < 6; ++n) check(rdram.ram.read<Byte>(0x3012 + n, RBusDevice::ARES_DEBUGGER) == 0x80 + n, "short PI range agrees with actual RAM");
  check(rdram.ram.read<Byte>(0x3018, RBusDevice::ARES_DEBUGGER) == 0, "PI provenance does not extend through untouched bytes");
  DmaService dmaView; dmaView.available = true;
  auto dmaPage = dmaView.execute({"0", "16", "1024", "2359296"});
  check(dmaPage.error.empty(), "DMA log serializes actual transfers");
  std::printf("DMA_PAGE %s\n", dmaPage.json.c_str());
  // Stress the history boundary with real transfer loops (guest scheduling is not advanced).
  dmaProvenance.reset(8);
  for(unsigned n = 0; n < DmaProvenance::capacity + 1; ++n) {
    pi.power(false); pi.io.pbusAddress = 0x10000000; pi.io.dramAddress = 0x3000; pi.io.writeLength = 7; pi.io.originPc = base + 0x218; pi.dmaWrite();
  }
  check(dmaProvenance.records.size() == DmaProvenance::capacity && dmaProvenance.dropped == 1, "real DMA history is a bounded overwrite ring");
  auto ringPage = dmaView.execute({"0", "16", "1024", "2359296"});
  check(ringPage.error.empty(), "overwritten DMA history serializes");
  std::printf("DMA_RING_PAGE %s\n", ringPage.json.c_str());
  DmaProvenance bounded; bounded.enabled = true; bounded.reset(1);
  bounded.begin(0x10000000, 0, 1000, base, 16, 4 * 1024 * 1024, true);
  for(unsigned n = 0; n < DmaProvenance::maxRanges + 1; ++n) bounded.wrote(n * 16, 1);
  check(bounded.current->ranges.size() == DmaProvenance::maxRanges && bounded.current->rangesTruncated && bounded.current->bytesWritten == DmaProvenance::maxRanges + 1, "DMA range truncation is explicit and byte totals survive");
  bounded.begin(0x10000000, 0, 8, base, 16, 4 * 1024 * 1024, false); bounded.wrote(0, 8);
  check(!bounded.current->writesConfirmed && bounded.current->bytesAttempted == 8, "nonidentity RDRAM writes remain unconfirmed");
  bounded.reset(2); check(bounded.records.empty() && bounded.current == nullptr && bounded.epoch == 2, "epoch reset revokes old provenance");
  // Profiler: original MIPS call/return and exception bodies, not a commercial ROM.
  auto& prof = cpu.profiler;
  word(0x300, 0x0c0004d0); word(0x304, 0);  // jal 80001340; delay
  word(0x308, 0x1000ffff); word(0x30c, 0);  // returned caller loops
  word(0x340, 0x24020001); word(0x344, 0x03e00008); word(0x348, 0);
  cpu.ipu.r[29].u64 = 0xffffffff80003000ull;
  cpu.icache.power(false); pc(base + 0x300); prof.setEnabled(true); steps(6);
  check(prof.stats.size() == 1 && prof.stats.at(0x80001340).callCount == 1, "profiler accounts one real call on return");
  auto functionCost = prof.stats.at(0x80001340);
  check(functionCost.inclCycles == 106 && functionCost.exclCycles == 106 && functionCost.exclBytesIn == 32, "profiler exact master ticks and RDRAM line bytes");
  auto timeBeforeCount = prof.now(); cpu.flushCount();
  check(prof.now() == timeBeforeCount, "profiler COUNT flush does not double count pending time");
  ProfilerService profView; profView.available = true; profView.owner = "fixture";
  auto top = profView.execute("n64.profiler.top", {"continuous", "32"}, "observer", false);
  check(top.error.empty(), "profiler top safe-point serialization"); std::printf("PROFILER_TOP %s\n", top.json.c_str());
  check(profView.execute("n64.profiler.control", {"0"}, "foreign", false).error == "busy", "profiler capture owner blocks foreign toggles");
  prof.onFrame(); check(prof.frameStats.at(0x80001340).callCount == 1 && prof.frameCount == 1, "profiler swap window commits completed calls");
  prof.onFrame(); check(prof.frameStats.empty() && prof.frameCount == 2, "profiler frame window does not reuse earlier data");
  prof.clearStats(); check(prof.stats.empty() && prof.frameCount == 0, "profiler clear rebases statistics");
  // Synchronous syscall cost nests beneath the interrupted function, ERET unwinds it.
  word(0x340, 0x0000000c);
  rdram.ram.write<Word>(0x180, 0x401a7000, RBusDevice::ARES_DEBUGGER); // mfc0 k0,EPC
  rdram.ram.write<Word>(0x184, 0x275a0004, RBusDevice::ARES_DEBUGGER);
  rdram.ram.write<Word>(0x188, 0x409a7000, RBusDevice::ARES_DEBUGGER); // mtc0 k0,EPC
  rdram.ram.write<Word>(0x18c, 0x42000018, RBusDevice::ARES_DEBUGGER); // eret
  cpu.scc.status.vectorLocation = 0; cpu.scc.status.exceptionLevel = 0; cpu.scc.status.errorLevel = 0; cpu.context.setMode();
  cpu.icache.power(false); pc(base + 0x300); steps(10);
  check(prof.stats.count(0xe0000048) && prof.excActive == 0 && prof.callStack.empty(), "profiler syscall and ERET balance synthetic exception frame");
  auto handler = prof.stats.at(0xe0000048); auto interrupted = prof.stats.at(0x80001340);
  check(handler.inclCycles > 0 && interrupted.inclCycles == interrupted.exclCycles + handler.inclCycles, "exception time excluded from interrupted function exclusive cost");
  check(interrupted.exclBytesIn == 32 && handler.exclBytesIn == 32, "exception bus bytes attributed separately");
  // Profiling JIT blocks uses the prologue hook, without debug interpreter forcing.
  prof.setEnabled(false); prof.clearStats(); word(0x340, 0x24020001);
  d.resume(); d.updateActive(); cpu.icache.power(false); pc(base + 0x300);
  prof.setEnabled(true); check(cpu.recompiler.callInstructionPrologue && !d.active, "profiler enables JIT prologue without forcing interpreter");
  cpu.jitClockTarget = cpu.clock + 100000; // This fixture drives instruction(), without the host scheduler.
  for(unsigned n = 0; n < 10 && prof.stats.empty(); ++n) cpu.instruction();
  check(prof.stats.count(0x80001340) && prof.stats.at(0x80001340).callCount == 1 && prof.stats.at(0x80001340).exclBytesIn == 32, "real JIT calls and inline icache miss bytes are profiled");
  cpu.debugger.tracer.instruction->setEnabled(true);
  profView.releaseOwner("old-generation"); check(prof.enabled, "stale profiler owner cannot disable new capture");
  profView.releaseOwner("fixture"); check(!prof.enabled && cpu.recompiler.callInstructionPrologue, "disconnect disables profiler while preserving debugger prologue");
  ares::Platform quietPlatform; auto* previousPlatform = ares::platform; ares::platform = &quietPlatform;
  prof.clearStats(); cpu.icache.power(false); pc(base + 0x300); cpu.jitClockTarget = cpu.clock + 100000;
  cpu.instruction(); cpu.instruction();
  check(cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1300)] != nullptr, "debugger-only JIT block compiled with inline cache refill");
  check(profView.execute("n64.profiler.control", {"1"}, "fixture", false).error.empty(), "enable profiler while instruction tracer already owns shared hook");
  check(cpu.recompiler.callInstructionPrologue && !cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1300)], "profiler toggle invalidates blocks even when shared prologue stays enabled");
  cpu.icache.power(false); pc(base + 0x300); cpu.jitClockTarget = cpu.clock + 100000;
  for(unsigned n = 0; n < 10 && prof.stats.empty(); ++n) cpu.instruction();
  check(prof.stats.count(0x80001340) && prof.stats.at(0x80001340).exclBytesIn == 32, "shared-tracer JIT counts actual cache-refill bytes after enabling profiling");
  profView.releaseOwner("fixture");
  cpu.debugger.tracer.instruction->setEnabled(false); check(!cpu.recompiler.callInstructionPrologue, "both disabled restores JIT without prologues");
  ares::platform = previousPlatform;
  prof.clearStats();
  CPU::Profiler::Frame boundedFrame; boundedFrame.funcAddr = 0x80001000;
  for(unsigned n = 0; n < CPU::Profiler::maxFunctions; ++n) prof.stats.emplace(n, CPU::Profiler::FuncStat{});
  prof.commitFrame(boundedFrame, nullptr, prof.now());
  check(prof.stats.size() == CPU::Profiler::maxFunctions && prof.droppedFunctionUpdates == 1, "profiler function-map cap reports dropped updates");
  prof.power();
  // Tap scheduling uses the coordinator's exact VI target and one cleanup path.
  TapService tap; InputService tapInput; tapInput.available = true;
  ExecutionService tapExecution; tapExecution.supported = true; tapExecution.frameKind = "vi";
  tapExecution.frame = 10; bool tapPaused = true;
  tapExecution.onStart = [&] { tapInput.start(1, tapExecution.frame); };
  tapExecution.onFinish = [&](const std::string& reason) { tap.finish(reason, tapInput, 1, tapExecution.frame); tapInput.finish(1, tapExecution.frame); };
  auto tapStart = tap.execute({"1", "0", "2", "1000", "0"}, "tap-owner", 1, tapPaused, tapInput, tapExecution);
  check(tapStart.error.empty() && tapInput.latched[0].buttons == 1 && tapExecution.active->target == 12, "tap button latched before first slice with exact target");
  tapExecution.frame = 11; tapExecution.service(tapPaused);
  check(tap.active() && tapInput.latched[0].buttons == 1, "tap remains held before target VI");
  tapExecution.frame = 12; tapExecution.service(tapPaused);
  check(!tap.active() && !tapInput.hasOverrides() && tapExecution.finished.back().second.find("\"held_frames\":2") != std::string::npos, "tap finishes exact interval and freezes held count");
  for(const char* reason : {"cancelled", "timeout", "breakpoint", "watchpoint", "paused", "disconnected", "host_input", "reset", "shutdown"}) {
    check(tap.execute({"1", "1", "2", "1000", "1"}, "tap-owner", 1, tapPaused, tapInput, tapExecution).error.empty(), "tap cleanup case starts");
    auto final = tapExecution.complete(reason, tapPaused);
    bool retain = TapService::retains(reason);
    check(!tap.active() && tapInput.pending[0].overridden() == retain && tapInput.latched[0].buttons == 0 && tapInput.pending[0].buttons == 0, "every tap exit releases button and revocation does not reacquire override");
    check(final.find(retain ? "\"ownership\":\"injected\"" : "\"ownership\":\"host\"") != std::string::npos, "tap immutable result matches cleanup ownership");
    tapInput.releaseAll();
  }
  check(tap.execute({"1", "14", "2", "1000", "0"}, "tap-owner", 1, tapPaused, tapInput, tapExecution).error == "invalid_request" && !tapInput.hasOverrides(), "invalid tap makes no input mutation");
  // Native image snapshots: actual VI RDRAM formats and render-thread epoch tags.
  FrameService images;
  vi.io.dramAddress = 0x4000; vi.io.width = 2; vi.io.colorDepth = 2; vi.io.vstart = 0; vi.io.vend = 4; vi.io.yscale = 0x400;
  const uint16_t colors[] = {0xf801, 0x07c1, 0x003f, 0xffff};
  for(unsigned n = 0; n < 4; ++n) rdram.ram.write<Half>(0x4000 + n * 2, colors[n], RBusDevice::ARES_DEBUGGER);
  auto framebuffer = images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true);
  check(framebuffer.error.empty() && images.pins.back().image->width == 2 && images.pins.back().image->height == 2 && images.pins.back().image->nonblack == 4, "VI framebuffer dimensions and nonblack count");
  auto raw = images.execute("video.read", {"1", "0", "16"}, "image-owner", 1, 10, true);
  check(raw.error.empty() && raw.json.find("ff0000ff00ff00ff0000ffffffffffff") != std::string::npos, "RGBA5551 converts big-endian RDRAM pixels to opaque RGBA8");
  check(images.execute("video.read", {"1", "0", "16"}, "foreign-owner", 1, 10, true).error == "busy", "native image pins are connection owned");
  rdram.ram.write<Half>(0x4000, 0, RBusDevice::ARES_DEBUGGER);
  check(images.execute("video.read", {"1", "0", "16"}, "image-owner", 1, 10, true).json == raw.json, "native framebuffer snapshot is immutable after RAM writes");
  check(images.execute("video.read", {"1", "15", "2"}, "image-owner", 1, 10, true).error == "out_of_bounds", "native pixel range bounds");
  check(images.execute("video.release", {"1"}, "image-owner", 1, 10, true).error.empty(), "native framebuffer release");
  vi.io.colorDepth = 3;
  rdram.ram.write<Word>(0x4000, 0x12345600, RBusDevice::ARES_DEBUGGER);
  check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error.empty(), "RGBA8888 framebuffer capture");
  check(images.execute("video.read", {"2", "0", "4"}, "image-owner", 1, 10, true).json.find("123456ff") != std::string::npos, "RGBA8888 preserves RGB and ignores source alpha like reference PNG");
  images.releaseOwner("image-owner"); check(images.pins.empty(), "owner revocation drops native image pins");
  vi.io.colorDepth = 0; check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error == "vi_blank", "blank VI does not fabricate a framebuffer");
  vi.io.colorDepth = 2; vi.io.width = 1; vi.io.vend = 2;
  check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error.empty(), "one-pixel RDRAM framebuffer remains valid (only presented 1x1 is invalid scanout)");
  images.releaseOwner("image-owner");
  const void* screenToken = &images;
  const uint8_t rgba[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
  images.submitted(screenToken, 1, 10); images.observe(screenToken, rgba, 2, 2, 1);
  check(images.latest && images.latest->epoch == 1 && images.latest->frame == 10, "native presented image uses emulation-thread submission tag");
  images.reset(2); images.observe(screenToken, rgba, 2, 2, 2);
  check(!images.latest && images.execute("video.begin", {"presented"}, "image-owner", 2, 0, true).error == "no_frame", "delayed pre-reset refresh cannot become a fresh image");
  images.submitted(screenToken, 2, 0); images.observe(screenToken, rgba, 2, 2, 3);
  check(images.execute("video.begin", {"presented"}, "image-owner", 2, 0, true).error.empty(), "new-epoch presented snapshot accepted");
  images.pins[0].expires = FrameService::Clock::now() - std::chrono::seconds(1);
  check(images.execute("video.read", {std::to_string(images.pins[0].id), "0", "4"}, "image-owner", 2, 0, true).error == "stale_snapshot", "expired native image pin revoked");
  {
    auto mutation = [&](std::vector<std::string> args, bool busy = false, bool paused = true) { return MachineService::mutate(args, 7, 12, busy, paused); };
    auto before = d.snapshot();
    check(mutation({"gpr:8:0x99", "pc:0x80001801"}).error == "unaligned_pc", "register set validates later PC before any GPR mutation");
    check(cpu.ipu.r[8].u64 == before.gpr[8] && cpu.ipu.pc == before.pc, "invalid register set is atomic");
    check(mutation({"gpr:8:0x99", "status:0x100000000"}).error == "invalid_request", "status bounded before mutation");
    check(mutation({"gpr:0:0x0"}).error == "invalid_register", "zero register read-only even writing zero");
    check(mutation({"gpr:8:0x99", "gpr:8:0x100"}).error == "invalid_request", "duplicate register rejected");
    check(mutation({"lo:0x1"}, true).error == "busy" && mutation({"lo:0x1"}, false, false).error == "not_stopped", "register mutation requires operation-free pause");
    d.breakpoints.clear(); d.watchpoints.clear(); d.resume();
    word(0x800, 0x10000002); word(0x804, 0x240d0001); word(0x808, 0); word(0x80c, 0);
    cpu.icache.power(false); pc(base + 0x800); steps(1);
    check(d.stopValid && d.halted && cpu.pipeline.inDelaySlot(), "register fixture stops in a real pending delay slot");
    auto stopped = d.stop; d.suppressBreakOnce = true;
    auto changed = mutation({"gpr:8:0xfedcba9876543210", "gpr:31:0xffffffffffffffff", "pc:0x8000180c", "hi:0x123456789abcdef0", "lo:0xdeadbeef", "llbit:0x1", "status:0x34000000"});
    check(changed.error.empty() && cpu.ipu.r[8].u64 == 0xfedcba9876543210ull && cpu.ipu.r[31].u64 == ~0ull, "lossless 64-bit GPR mutation");
    check(cpu.ipu.pc == 0xffffffff8000180cull && cpu.pipeline.nextpc == 0xffffffff80001810ull && !cpu.pipeline.inDelaySlot(), "PC canonicalizes and resets branch pipeline");
    check(!d.stopValid && !d.halted && !d.suppressBreakOnce, "PC edit clears stale stop/suppression without executing instructions");
    check(cpu.ipu.hi.u64 == 0x123456789abcdef0ull && cpu.ipu.lo.u64 == 0xdeadbeef && cpu.scc.llbit && (uint32_t)cpu.getControlRegister(12) == 0x34000000, "HI LO LLbit and architectural Status setter");
    steps(1); auto retained = d.stop;
    check(mutation({"hi:0x2"}).error.empty() && d.stopValid && d.halted && d.stop.snapshot.hi == retained.snapshot.hi, "non-PC changes preserve immutable stop snapshot");
    check(stopped.snapshot.delaySlot && stopped.snapshot.pc == base + 0x804, "previous delay stop stays immutable");
    d.resume(); cpu.icache.power(false); pc(base + 0x80); cpu.jitClockTarget = cpu.clock + 100000;
    cpu.instruction(); auto sectionIndex = cpu.recompiler.sectionIndex(0x1080);
    check(cpu.recompiler.sections[sectionIndex], "register fixture has an actual JIT translation");
    check(mutation({"lo:0x3"}).error.empty() && !cpu.recompiler.sections[sectionIndex], "register changes drop old-state JIT translations");
    // Disassembly uses resident D-cache rather than allocating or silently using I-cache.
    cpu.dcache.power(false); word(0x900, 0x24020001); word(0x904, 0x1000ffff); word(0x908, 0x01000008);
    auto& codeLine = cpu.dcache.line(base + 0x900); codeLine.fill(0x1900);
    cpu.dcache.writeDebug<Byte>(base + 0x903, 0x1903, 2);
    auto hits = cpu.profile.dcacheHits, misses = cpu.profile.dcacheMisses; auto cpuBeforeRead = d.snapshot();
    auto decoded = MachineService::disassemble({"0x1900", "3", "cpu"}, 7, 12);
    check(decoded.error.empty() && decoded.json.find("\"word\":\"0x24020002\"") != std::string::npos, "disassembly reads dirty cached instruction bytes");
    check(decoded.json.find("\"target\":\"0xffffffff80001904\"") != std::string::npos && decoded.json.find("current_register_context") != std::string::npos, "branch and indirect target provenance");
    auto raw = MachineService::disassemble({"0xa0001900", "1", "cpu"}, 7, 12);
    check(raw.error.empty() && raw.json.find("\"word\":\"0x24020001\"") != std::string::npos, "disassembly KSEG1 bypasses dirty cache");
    check(cpu.profile.dcacheHits == hits && cpu.profile.dcacheMisses == misses && cpu.ipu.pc == cpuBeforeRead.pc, "disassembly preserves CPU/cache counters");
    check(MachineService::disassemble({"0x3ffffc", "2", "rdram"}, 7, 12).error == "out_of_bounds" && MachineService::disassemble({"0x1901", "1", "cpu"}, 7, 12).error == "unaligned_address", "disassembly validates complete aligned range");
    rsp.imem.allocate(4096); rsp.imem.fill(); rsp.ipu.r[3].u32 = 0xdeadbeef; rsp.ipu.pc = 0x100;
    rsp.vpu.r[4].element(7) = 0xabcd; rsp.vpu.acch.element(0) = 0xffff; rsp.status.semaphore = 0;
    rsp.dma.clock = -123; rsp.dma.current.originPc = 0xfedcba9876543210ull;
    auto rspState = MachineService::rspRegisters();
    check(rsp.status.semaphore == 0 && rspState.find("0xdeadbeef") != std::string::npos && rspState.find("43981") != std::string::npos && rspState.find("\"clock\":\"-123\"") != std::string::npos, "RSP snapshot observes scalar/vector/accumulator and signed DMA clock without semaphore side effects");
    rdp.command.start = 0x123456; rdp.command.source = 1; rdp.command.freeze = 1;
    check(MachineService::rdpState().find("\"source\":\"DMEM\"") != std::string::npos && rdp.command.start == 0x123456 && rdp.command.freeze, "RDP observation leaves command/status intact");
    auto viCounter = vi.io.vcounter; auto dmaBusy = pi.io.dmaBusy;
    check(MachineService::peripherals().find("VI_ORIGIN") != std::string::npos && vi.io.vcounter == viCounter && pi.io.dmaBusy == dmaBusy, "peripheral observations do not invoke MMIO side effects");
    std::printf("MACHINE_RSP %s\nMACHINE_RDP %s\nMACHINE_PERIPHERALS %s\nMACHINE_DISASSEMBLY %s\nMACHINE_MUTATION %s\n", rspState.c_str(), MachineService::rdpState().c_str(), MachineService::peripherals().c_str(), decoded.json.c_str(), changed.json.c_str());
    rsp.imem.reset();
  }
  {
    // Reference state header/prefix field order, independently assembled here.
    auto before = cpu.debugControl.snapshot(); auto retired = cpu.debugControl.instructionsRetired;
    auto ram = rdram.ram.read<Word>(0x2000, RBusDevice::ARES_DEBUGGER);
    serializer prefix; u32 signature = ares::SerializerSignature; bool synchronize = true;
    char version[16] = "v153", description[512] = {};
    prefix(signature); prefix(synchronize); prefix(version); prefix(description);
    check(prefix.size() == 533, "state header is 533 bytes");
    prefix(ares::Nintendo64::random); prefix(ares::Nintendo64::queue); prefix(cartridge);
    prefix(controllerPort1); prefix(controllerPort2); prefix(controllerPort3); prefix(controllerPort4);
    auto [offset, size] = ares::Nintendo64::system.inspectStateRdramLayout();
    check(offset == prefix.size() && size == rdram.ram.size, "offline layout matches reference prefix");
    auto after = cpu.debugControl.snapshot();
    check(before.pc == after.pc && std::memcmp(before.gpr, after.gpr, sizeof(before.gpr)) == 0 && std::memcmp(before.cop0, after.cop0, sizeof(before.cop0)) == 0 && retired == cpu.debugControl.instructionsRetired, "offline layout preserves CPU state");
    check(ram == rdram.ram.read<Word>(0x2000, RBusDevice::ARES_DEBUGGER), "offline layout preserves RDRAM");
  }
  {
    // Real RSP interpreter, branch pipeline, DMA slots and configured JIT.
    rsp.load(root); rsp.power(false); rsp.status.halted = false;
    auto& gate = rsp.debugControl;
    rsp.imem.write<Word>(0, 0x24080001); // addiu t0,zero,1
    rsp.imem.write<Word>(4, 0x10000002); // beq zero,zero,0x10
    rsp.imem.write<Word>(8, 0x25080001); // delay slot
    rsp.imem.write<Word>(12, 0x25080040); // skipped
    rsp.imem.write<Word>(16, 0x25080002);
    rsp.imem.write<Word>(20, 0x0000000d); // BREAK
    auto jitEnabled = rsp.recompiler.enabled;
    gate.active = true; gate.pause(0, "fixture"); gate.grant(1, false);
    rsp.instruction();
    check(gate.stopped && gate.instructions == 1 && rsp.ipu.pc == 4 && rsp.ipu.r[8].u32 == 1, "RSP one instruction grant has no dual issue");
    rsp.instruction(); check(gate.instructions == 1 && rsp.ipu.pc == 4, "stopped gate does not execute");
    gate.breakpoints = {4, 16}; gate.grant(0, true);
    for(unsigned i = 0; i < 8 && !gate.stopped; ++i) rsp.instruction();
    check(gate.reason == "rsp_breakpoint" && rsp.ipu.pc == 16 && gate.instructions == 3 && rsp.ipu.r[8].u32 == 2, "RSP skips starting breakpoint once and stops before branch target after delay slot");
    gate.grant(1, false); rsp.instruction();
    check(gate.instructions == 4 && rsp.ipu.r[8].u32 == 4 && rsp.ipu.pc == 20, "RSP step from breakpoint executes exactly once");
    gate.grant(100, false); rsp.instruction();
    check(gate.reason == "rsp_halted" && gate.instructions == 5 && rsp.status.halted && rsp.status.broken, "RSP hardware BREAK ends grant without clearing HALT");
    check(rsp.recompiler.enabled == jitEnabled, "RSP gate preserves configured JIT flag");
    // A stale full slot must progress even if the busy slot is already empty.
    rdram.ram.write<Word>(0x3000, 0x12345678, RBusDevice::ARES_DEBUGGER);
    rsp.dma = {}; rsp.dma.full.read = 1; rsp.dma.pending.dramAddress = 0x3000;
    rsp.dma.pending.length = 0; rsp.dma.pending.pbusAddress = 0x100;
    rsp.dmaStep(0);
    check(!rsp.dma.busy.any() && !rsp.dma.full.any() && rsp.dmem.read<Word>(0x100) == 0x12345678, "RSP debug DMA promotes and drains pending slot");
    rsp.dma = {}; rsp.dma.busy.read = 1; rsp.dma.full.read = 1;
    rsp.dma.current.count = 255; rsp.dma.current.length = 0; rsp.dma.current.dramAddress = 0x3000;
    rsp.dma.pending.count = 255; rsp.dma.pending.length = 0; rsp.dma.pending.dramAddress = 0x3000;
    rsp.dmaStep(0);
    check(!rsp.dma.busy.any() && !rsp.dma.full.any(), "RSP bounded DMA drain handles both maximum-row slots");
    gate = {};
    RspDebugService service; ExecutionService exec; bool paused = true;
    exec.supported = true; exec.frameKind = "vi";
    exec.onFinish = [&](const std::string& reason) { service.finish(reason); };
    auto request = [&](const char* method, std::vector<std::string> args = {}, const char* owner = "rsp-owner") { return service.execute(method, args, owner, 1, true, exec, paused); };
    check(request("n64.rsp.debug", {"1"}).error.empty() && service.active(), "RSP native session enables");
    for(unsigned i = 0; i < 16; ++i) check(request("n64.rsp.break", {"add", N64Service::hex(i * 4)}).error.empty(), "RSP breakpoint table accepts bounded entries");
    check(request("n64.rsp.break", {"add", "0x40"}).error == "table_full" && request("n64.rsp.break", {"add", "0x1"}).error == "invalid_request", "RSP breakpoint table and alignment bounds");
    check(request("n64.rsp.break", {"clear"}, "foreign").error == "busy", "RSP breakpoint mutation owned");
    check(request("n64.rsp.step", {"1"}).error.empty() && exec.active && !gate.stopped, "RSP step is existing execution operation");
    service.expires = RspDebugService::Clock::now(); service.service(exec, paused);
    check(!exec.active && !service.active() && paused && exec.finished.back().second.find("rsp_lease_expired") != std::string::npos, "RSP lease expiry freezes result before releasing gate");
    check(request("n64.rsp.debug", {"1"}).error.empty(), "RSP reacquires after expiry");
    service.releaseOwner("foreign"); check(service.active(), "foreign release leaves RSP owner intact");
    service.releaseOwner("rsp-owner"); check(!service.active(), "RSP owner release clears idle gate");
    rdp.load(root); rdp.power(false);
    auto& capture = rdp.debugCapture;
    executionCoordinate.reset(); executionCoordinate.frame = 42;
    capture.arm(7, 42, 8, true);
    rsp.status.halted = true;
    rsp.dmem.write<Byte>(0xfc0, 0xab); rsp.imem.write<Byte>(0, 0xcd);
    rsp.writeWord(0x04040010, 1, cpu);
    check(capture.tasks.size() == 1 && capture.tasks[0].header[0] == 0xab && capture.tasks[0].dmem[0xfc0] == 0xab, "capture snapshots actual HALT-clear task before reuse");
    auto dmemHash = capture.tasks[0].dmemHash;
    rsp.dmem.write<Byte>(0xfc0, 0x55); rsp.imem.write<Byte>(0, 0x66);
    check(capture.tasks[0].header[0] == 0xab && capture.tasks[0].dmemHash == dmemHash, "task bytes and hashes stay immutable after reuse");
    rdram.ram.write<Dual>(0x3800, 0x0011223344556677ull, RBusDevice::ARES_DEBUGGER);
    rdram.ram.write<Dual>(0x3808, 0x0077665544332211ull, RBusDevice::ARES_DEBUGGER);
    rdp.command.current = rdp.command.start = 0x3800; rdp.command.end = 0x3810; rdp.render();
    check(capture.words.size() == 2 && capture.words[0].value == 0x0011223344556677ull && capture.words[1].value == 0x0077665544332211ull && capture.batches[0].current == 0x3800 && capture.batches[0].task == 0, "actual RDP render captures full submitted words before current advances");
    rdram.ram.write<Dual>(0x3800, 0, RBusDevice::ARES_DEBUGGER);
    check(capture.words[0].value == 0x0011223344556677ull, "RDP words survive source buffer reuse");
    rsp.dmem.write<Dual>(0xff8, 0x00123456789abcdeull);
    rdp.command.source = 1; rdp.command.start = rdp.command.current = 0xff8; rdp.command.end = 0x1000; rdp.render();
    check(capture.words.size() == 3 && capture.words[2].address == 0xff8 && capture.words[2].value == 0x00123456789abcdeull && capture.batches[1].source && !rdp.command.crashed, "actual XBUS render captures DMEM submission");
    executionCoordinate.frame = 43; capture.onBatch(0, 0, 8, false, 0);
    check(capture.words.size() == 3 && capture.batchesOutsideInterval == 1, "exact interval excludes next VI submission");
    executionCoordinate.frame = 42;
    capture.arm(7, 42, 1, false); capture.onBatch(0, 0, 0xfffff8, false, 0);
    check(capture.words.size() == 1 && capture.droppedWords == 0x1ffffe && capture.truncated, "capture counts large dropped range without reading it");
    for(unsigned n = 0; n < RDPDebugCapture::MaxTasks + 1; ++n) capture.onRspStart(n);
    capture.onBatch(0, 0, 8, false, 0);
    check(capture.tasks.size() == 64 && capture.droppedTasks == 1 && capture.batches.back().task == -1, "task overflow never misattributes next batch");
    for(unsigned n = 0; n < RDPDebugCapture::MaxBatches; ++n) capture.onBatch(0, 0, 8, false, 0);
    check(capture.batches.size() == 512 && capture.droppedBatches == 2, "batch storage strictly bounded");
    capture = {}; rdp.unload(); executionCoordinate.reset();
    rsp.unload();
  }
  {
    cpu.dcache.power(false);
    constexpr uint64_t entry = 0xffffffff80030000ull, stack = 0xffffffff80031000ull;
    rdram.ram.write<Word>(0x30000, 0x27bdffe0, RBusDevice::ARES_DEBUGGER); // addiu sp,sp,-32
    rdram.ram.write<Word>(0x30004, 0xffbf0018, RBusDevice::ARES_DEBUGGER); // sd ra,24(sp)
    rdram.ram.write<Word>(0x30008, 0, RBusDevice::ARES_DEBUGGER);
    rdram.ram.write<Word>(0x30010, 0x27bd0020, RBusDevice::ARES_DEBUGGER); // executed epilogue
    rdram.ram.write<Word>(0x30100, 0x0c00c000, RBusDevice::ARES_DEBUGGER); // caller JAL
    rdram.ram.write<Dual>(0x31018, 0xffffffff80030108ull, RBusDevice::ARES_DEBUGGER);
    cpu.ipu.r[29].u64 = stack; cpu.ipu.r[31].u64 = 0xffffffff80030108ull; cpu.pipeline.setPc(entry + 8);
    auto observed = cpu.debugControl.snapshot(); auto hits = cpu.profile.dcacheHits, misses = cpu.profile.dcacheMisses;
    auto frames = StackService::unwind(observed, 8, entry);
    check(frames.frames.size() == 2 && frames.frames[0].ra == 0xffffffff80030108ull && frames.frames[0].frameSize == 32 && frames.frames[0].confidence == 2, "stack saved 64-bit RA and caller prologue");
    check(frames.frames[0].raSaveWidth == 8 && frames.frames[0].callsite == 0xffffffff80030100ull && frames.rawBytes.size() == 256, "stack linked callsite and bounded raw window");
    auto stackJson = StackService::execute({"0x80030000"}, true, 7, 10);
    check(stackJson.error.empty() && StackService::execute({"0x80030001"}, true, 7, 10).error == "invalid_function_start", "stack hint validation");
    auto pre = observed; pre.pc = entry;
    check(StackService::unwind(pre, 8, entry).frames[0].frameSize == 0, "stack excludes unexecuted allocation at current PC");
    auto post = observed; post.pc = entry + 20; post.gpr[29] += 32;
    auto epilogue = StackService::unwind(post, 8, entry);
    check(epilogue.frames[0].epilogueSeen && epilogue.frames[0].frameSize == 0 && epilogue.frames[0].raSource == "live_ra_after_epilogue", "stack recognizes executed epilogue");
    auto bad = observed; bad.pc = 0x04040010;
    check(StackService::unwind(bad, 8, 0).frames.empty(), "stack refuses MMIO code");
    check(cpu.ipu.pc == observed.pc && cpu.ipu.r[29].u64 == observed.gpr[29] && cpu.profile.dcacheHits == hits && cpu.profile.dcacheMisses == misses, "stack leaves CPU and cache counters unchanged");
    InspectionService inspection;
    auto request = [&](const char* method, std::vector<std::string> args, const char* owner = "inspect") { return inspection.execute(method, args, owner, 7, 10, true, false, true); };
    check(request("n64.dump.begin", {"all", "0x0", "0", "8388608"}).error == "size_mismatch" && !inspection.pin, "dump expected size prevalidation");
    // Dirty CPU view and physical backing differ without allocating cache lines.
    auto& line = cpu.dcache.line(0xffffffff80030000ull); line.fill(0x30000);
    cpu.dcache.writeDebug<Byte>(0xffffffff80030000ull, 0x30000, 0xaa);
    auto dump = request("n64.dump.begin", {"cpu", "0x80030000", "8", "0"});
    check(dump.error.empty() && inspection.pin->bytes[0] == 0xaa, "dump uses resident dirty D-cache view");
    std::string id = std::to_string(inspection.pin->id);
    check(request("n64.dump.read", {id, "0", "8"}, "foreign").error == "busy", "dump snapshot ownership");
    rdram.ram.write<Word>(0x30000, 0, RBusDevice::ARES_DEBUGGER);
    check(inspection.pin->bytes[0] == 0xaa, "dump bytes immutable after source changes");
    check(request("n64.dump.read", {id, "7", "2"}).error == "out_of_bounds", "dump complete read range validation");
    request("n64.dump.release", {id});
    check(request("n64.dump.begin", {"cpu", "0xa0030000", "8", "0"}).error.empty() && inspection.pin->bytes[0] == 0, "dump KSEG1 bypasses dirty cache");
    inspection.pin->expires = InspectionService::Clock::now();
    check(request("n64.dump.read", {std::to_string(inspection.pin->id), "0", "1"}).error == "stale_snapshot", "dump snapshot expiry");
    auto probe1 = request("n64.render.probe", {});
    check(probe1.json.find("\"originChanged\":null") != std::string::npos, "probe first observation has no delta");
    vi.io.dramAddress ^= 8; rdp.command.clock += 1;
    auto probe2 = inspection.execute("n64.render.probe", {}, "inspect", 7, 11, true, false, true);
    check(probe2.json.find("\"originChanged\":true") != std::string::npos && probe2.json.find("\"dpcAdvanced\":true") != std::string::npos && probe2.json.find("\"frameAdvanced\":true") != std::string::npos, "probe compares observed coordinates");
    check(request("n64.render.probe", {}, "foreign").json.find("\"originChanged\":null") != std::string::npos, "probe baselines isolated by owner");
    std::printf("INSPECTION_STACK %s\nINSPECTION_DUMP %s\nINSPECTION_PROBE %s\n", stackJson.json.c_str(), dump.json.c_str(), probe2.json.c_str());
  }
  {
    CallService calls; ExecutionService exec; bool paused = true;
    auto& d = cpu.debugControl;
    d.breakpoints.clear(); d.watchpoints.clear(); d.trace = {}; d.resume();
    exec.supported = exec.cpuSupported = true; exec.frameKind = "vi";
    exec.onStart = [&] { d.resume(); };
    exec.onFinish = [&](const std::string&) { d.stepping = false; d.stepRemaining = 0; d.updateActive(); };
    auto request = [&](const char* method, std::vector<std::string> args, const char* owner = "call-owner") { exec.instructions = d.instructionsRetired; return calls.execute(method, args, owner, 9, true, exec, paused); };
    auto drive = [&] {
      for(unsigned i = 0; i < 2000 && exec.active; ++i) {
        cpu.instruction(); exec.instructions = d.instructionsRetired; calls.service(exec, paused);
        if(exec.active && d.halted) exec.complete(N64Service::reason(d.stop.reason), paused);
      }
      check(!exec.active, "bounded call fixture finishes"); return exec.finished.back().second;
    };
    auto resetCaller = [&] {
      cpu.setControlRegister(12, 0x34000000); cpu.pipeline.setPc(0xffffffff80032000ull);
      cpu.ipu.r[29].u64 = 0xffffffff80034000ull; cpu.ipu.r[9].u64 = 0x444;
      cpu.ipu.hi.u64 = 0x123; cpu.ipu.lo.u64 = 0x456; cpu.scc.llbit = 1;
      for(unsigned i = 0; i < 32; ++i) cpu.fpu.r[i].u64 = 0xdeadbeef00000000ull | i;
      cpu.setControlRegisterFPU(31, 0); d.resume(); paused = true;
      cpu.icache.power(false); cpu.dcache.power(false);
    };
    resetCaller();
    auto writeCode = [&](std::initializer_list<uint32_t> words) { unsigned at = 0x33000; for(auto w : words) { rdram.ram.write<Word>(at, w, RBusDevice::ARES_DEBUGGER); at += 4; } cpu.icache.power(false); };
    writeCode({0x0085102d, 0xfcc20000, 0x44870000, 0x00800011, 0x00a00013, 0x3c083000, 0x40886000, 0x03e00008, 0x24030055});
    auto before = d.snapshot(); auto clock = cpu.clock;
    auto start = request("n64.cpu.call", {"0x33000", "9", "60", "1000", "0x1234567890000000", "0x7", "0xffffffff80035000", "0x12345678"});
    check(start.error.empty() && d.callGate.active && cpu.ipu.r[25].u64 == 0xffffffff80033000ull, "call sets t9 entry and private gate");
    auto success = drive();
    check(success.find("function_return") != std::string::npos && success.find("0x1234567890000007") != std::string::npos && success.find("\"v1\":\"0x55\"") != std::string::npos, "call return at exact instruction limit includes completed delay slot");
    check(cpu.ipu.pc == before.pc && cpu.ipu.hi.u64 == before.hi && cpu.ipu.lo.u64 == before.lo && !cpu.scc.llbit, "call restores PC HI LO and clears reservation");
    for(unsigned i = 1; i < 32; ++i) check(cpu.ipu.r[i].u64 == before.gpr[i], "call restores every caller GPR");
    for(unsigned i = 0; i < 32; ++i) check(cpu.fpu.r[i].u64 == before.fpr[i], "call restores FPR bank after FR toggle");
    check((uint32_t)cpu.getControlRegister(12) == before.cop0[12] && !calls.savedValid && !d.callGate.active, "call restores Status and clears successful context");
    check(cpu.dcache.readDebug<Word>(0xffffffff80035000ull, 0x35000) == 0x12345678 && cpu.clock > clock, "call retains dirty memory and elapsed CPU time");
    std::printf("CALL_SUCCESS %s\n", success.c_str());
    resetCaller(); writeCode({0x27bdfff8, 0x03e00008, 0});
    rdram.ram.write<Word>(rdram.ram.size - 4, 0x24090077, RBusDevice::ARES_DEBUGGER);
    check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error.empty(), "invalid SP call starts");
    auto invalid = drive();
    check(invalid.find("invalid_return") != std::string::npos && calls.savedValid && cpu.ipu.r[9].u64 == 0x444, "private return gate rejects wrong SP without fetching sentinel instruction");
    auto stopped = d.stop.snapshot.pc;
    check(request("n64.cpu.callRecover", {"1"}, "foreign").error == "busy", "live owner protects caller recovery");
    check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error == "recovery_pending", "new call cannot overwrite recovery");
    auto recovered = request("n64.cpu.callRecover", {"1"});
    check(recovered.error.empty() && cpu.ipu.pc == before.pc && !d.stopValid && !calls.savedValid, "explicit recovery restores caller and clears stale stop");
    check(invalid.find(N64Service::hex(stopped)) != std::string::npos, "interrupted result retains completion PC after recovery");
    std::printf("CALL_INTERRUPTED %s\nCALL_RECOVERED %s\n", invalid.c_str(), recovered.json.c_str());
    resetCaller(); writeCode({0x080fffff, 0}); // J to 8 MiB gate is patched below for installed RAM
    rdram.ram.write<Word>(0x33000, 0x08000000 | ((rdram.ram.size - 4) >> 2), RBusDevice::ARES_DEBUGGER);
    check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error.empty(), "direct gate jump starts");
    check(drive().find("invalid_return") != std::string::npos, "jump to sentinel without JR is not a function return");
    request("n64.cpu.callRecover", {"1"});
    resetCaller(); writeCode({0x0000000c});
    check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error.empty(), "exception call starts");
    auto exception = drive();
    check(exception.find("\"stop_reason\":\"exception\"") != std::string::npos && exception.find("\"exception_code\":8") != std::string::npos && cpu.scc.status.exceptionLevel, "syscall stops before exception handler and retains actual EXL");
    auto cause = cpu.getControlRegister(13), epc = cpu.getControlRegister(14);
    calls.releaseOwner("call-owner");
    check(request("n64.cpu.callRecover", {"1"}, "new-owner").error.empty() && !cpu.scc.status.exceptionLevel && cpu.getControlRegister(13) == cause && cpu.getControlRegister(14) == epc, "reconnect recovery restores Status but retains Cause");
    resetCaller(); writeCode({0x1000ffff, 0});
    for(const char* reason : {"instruction_limit", "timeout", "cancelled", "paused", "disconnected", "frame_limit"}) {
      check(request("n64.cpu.call", {"0x33000", "2", "1", "1000"}).error.empty(), "bounded interruption call starts");
      std::string result;
      if(std::string(reason) == "instruction_limit") result = drive();
      else if(std::string(reason) == "frame_limit") { ++exec.frame; exec.service(paused); result = exec.finished.back().second; }
      else result = exec.complete(reason, paused);
      check(result.find(std::string("\"stop_reason\":\"") + reason + "\"") != std::string::npos && calls.savedValid && !d.callGate.active, "every interruption preserves caller context and releases gate");
      check(request("n64.cpu.callRecover", {"1"}).error.empty(), "each interruption is explicitly recoverable");
    }
    resetCaller(); writeCode({0, 0x03e00008, 0});
    nall::string callError;
    auto breakId = d.addBreakpoint(0xffffffff80033000ull, {}, callError);
    check(breakId && request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error.empty(), "call accepts existing breakpoint definitions");
    check(drive().find("\"stop_reason\":\"breakpoint\"") != std::string::npos && calls.savedValid, "entry breakpoint retains interrupted caller");
    d.removeBreakpoint(breakId); request("n64.cpu.callRecover", {"1"});
    resetCaller(); writeCode({0x03e00008, 0xfc800000}); // sd zero,0(a0) in return delay slot
    auto watchId = d.addWatchpoint(0xffffffff80035000ull, 8, Debug::AccessWrite, {}, callError);
    check(watchId && request("n64.cpu.call", {"0x33000", "2", "60", "1000", "0xffffffff80035000"}).error.empty(), "call accepts existing watchpoint definitions");
    auto watchStop = drive();
    check(watchStop.find("\"stop_reason\":\"watchpoint\"") != std::string::npos && calls.savedValid, "return delay-slot watchpoint wins over valid return and instruction limit");
    d.removeWatchpoint(watchId); request("n64.cpu.callRecover", {"1"});
    for(bool nmi : {false, true}) {
      resetCaller(); writeCode({0x1000ffff, 0});
      check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error.empty(), "interruptible call starts");
      if(nmi) cpu.exception.nmi(); else cpu.exception.interrupt();
      auto interrupted = drive();
      check(interrupted.find("\"stop_reason\":\"exception\"") != std::string::npos && interrupted.find(nmi ? "\"exception_code\":256" : "\"exception_code\":0") != std::string::npos, "interrupt and NMI stop before handler fetch");
      request("n64.cpu.callRecover", {"1"});
    }
    resetCaller();
    check(request("n64.cpu.call", {"0x33001", "100", "60", "1000"}).error == "unaligned_address" && !calls.savedValid, "entry validation precedes context mutation");
    cpu.ipu.r[29].u64 = 0;
    check(request("n64.cpu.call", {"0x33000", "100", "60", "1000"}).error == "invalid_stack", "call rejects non-RDRAM stack");
    resetCaller(); writeCode({0x1000ffff, 0});
    resetCaller(); cpu.pipeline.setPc(0xffffffff80033000ull); d.requestStepInstructions(1); cpu.instruction();
    check(cpu.pipeline.inDelaySlot() && request("n64.cpu.call", {"0x33000", "1", "1", "1000"}).error == "unsafe_context", "call rejects delay slot without mutation");
    calls.reset(); d.resume();
  }
  {
    for(bool jit : {true, false}) {
      rsp.load(root); rsp.power(false); rdp.load(root); rdp.power(false);
      rsp.recompiler.enabled = jit;
      auto imem = [&](uint32_t address, uint32_t value) { rsp.imem.write<Word>(address, value); };
      imem(0x14, 0x8c190000); imem(0x18, 0x8c180004); imem(0x1c, 0);
      imem(0x20, 0x00196603); imem(0x24, 0x000c5840); imem(0x28, 0x24080060);
      imem(0x2c, 0); imem(0x30, 0); imem(0x34, 0x01000008); imem(0x38, 0);
      imem(0x60, 0x24100001); imem(0x64, 0xac100004);
      imem(0x68, 0x24090100); imem(0x6c, 0x40890000); // DMA DMEM destination
      imem(0x70, 0x24093800); imem(0x74, 0x40890800); // RDRAM source
      imem(0x78, 0x24090000); imem(0x7c, 0x40891000); // eight-byte read
      imem(0x80, 0x10000001); imem(0x84, 0); imem(0x88, 0x0000000d);
      rsp.dmem.write<Word>(0, 0x05010203); rsp.dmem.write<Word>(4, 0x12345678);
      rsp.ipu.pc = 0x14; rsp.branch.setPc(0x14);
      ViewerService viewer;
      auto request = [&](const char* method, std::vector<std::string> args, const char* actor = "viewer") { return viewer.execute(method, args, actor, 9, true, false); };
      check(request("n64.capture.control", {"1", "1"}).error.empty(), "viewer capture channels enable");
      auto& c = commandCapture;
      check(c.sigPc == 0x20 && c.loopPc == 0x14 && c.dispatchPc == 0x34, "F3DEX2 signature resolves bounded live hook PCs");
      check(request("n64.capture.control", {"0", "0"}, "foreign").error == "busy" && c.rspEnabled && c.rdpEnabled, "viewer channel ownership updates are atomic");
      check(viewer.execute("n64.capture.control", {"1", "0"}, "viewer", 9, true, true).error == "busy", "active operation excludes viewer flag mutations");
      rsp.writeWord(0x04040010, 1, cpu);
      for(unsigned i = 0; i < 100 && !rsp.status.halted; ++i) { rsp.instruction(); rsp.dmaStep(128); }
      check(rsp.status.halted && c.rspLive.size() == 2, "actual RSP JIT/interpreter emits dispatch and completed command segments");
      auto command = std::find_if(c.rspLive.begin(), c.rspLive.end(), [](auto& r) { return !r.overhead; });
      check(command != c.rspLive.end() && command->word0 == 0x05010203 && command->word1 == 0x12345678 && command->cycle > 0 && command->bytesIn == 8 && rsp.dmem.read<Word>(4) == 1, "live dispatch preserves command words before handler reuses DMEM");
      check(rsp.recompiler.enabled == jit, "viewer preserves configured RSP JIT flag");
      rdram.ram.write<Dual>(0x3800, 0x0011223344556677ull, RBusDevice::ARES_DEBUGGER);
      rdram.ram.write<Dual>(0x3808, 0x0077665544332211ull, RBusDevice::ARES_DEBUGGER);
      rdp.command.start = rdp.command.current = 0x3800; rdp.command.end = 0x3810; rdp.render();
      check(c.rdpLive.size() == 2 && c.rdpLive[0].word0 == 0x0011223344556677ull, "software RDP decoder records actual viewer commands");
      vi.io.dramAddress = 0x8000; c.afterRun();
      check(c.presentedFrame == 1 && c.rspLatest.size() == 2 && c.rdpLatest.size() == 2 && c.rspLive.empty(), "nonzero origin change commits immutable double buffers");
      auto rspPage = request("n64.capture.rsp", {"0", "0", "64"});
      auto rdpPage = request("n64.capture.rdp", {"0", "0", "64"});
      check(rspPage.error.empty() && rdpPage.error.empty(), "native viewer pages expose committed records");
      if(jit) std::printf("VIEWER_RSP %s\nVIEWER_RDP %s\nVIEWER_CONTROL %s\n", rspPage.json.c_str(), rdpPage.json.c_str(), request("n64.capture.control", {"-1", "-1"}).json.c_str());
      auto generation = c.generation;
      c.rdpCommand(0, 0x99, 0, 1);
      check(c.rdpLatest[0].word0 == 0x0011223344556677ull, "next live frame cannot overwrite committed commands");
      vi.io.dramAddress = 0x9000; c.afterRun();
      check(request("n64.capture.rdp", {std::to_string(generation), "0", "64"}).error == "stale_capture", "viewer pagination rejects newer committed generation");
      for(unsigned i = 0; i < CommandCapture::Capacity + 3; ++i) c.rdpCommand(0, i, 0, 1);
      vi.io.dramAddress = 0xa000; c.afterRun();
      check(c.rdpLatest.size() == c.Capacity && c.rdpCommitted == c.Capacity + 3, "viewer storage bounds and drop counts");
      c.segment = {}; c.segmentOpen = true; c.segmentStart = 0xfffffff0; rsp.pipeline.clocksTotal = 0x10; c.rspFlush();
      check(c.rspLive.back().cycle == 32, "RSP segment timing handles 32-bit clock wrap");
      imem(0x20, 0); c.detectRsp();
      check(c.sigPc == -1 && !c.segmentOpen, "unknown microcode never retains a stale F3D dispatch hook");
      viewer.releaseOwner("viewer"); check(!c.rspEnabled && !c.rdpEnabled, "disconnect disables owned viewer channels");
      viewer.reset(); rsp.unload(); rdp.unload();
    }
  }
  pi.detach(cartridge.romDevice); cartridge.rom.reset();
  rdram.ram.reset();
  std::puts("CPU_DEBUG_FIXTURE_PASS");
}
