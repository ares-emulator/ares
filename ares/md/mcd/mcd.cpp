#define QOI_IMPLEMENTATION
#include <md/md.hpp>
#include <string.h>
#include <string>
#include <optional>
#include "json.hpp"

namespace nlohmann {

///////////////////////////////////////////////////////////////////////////////
// std::optional
///////////////////////////////////////////////////////////////////////////////
template <class T>
void optional_to_json(nlohmann::json &j, const char *name, const std::optional<T> &value) {
    if (value)
        j[name] = *value;
}
template <class T>
void optional_from_json(const nlohmann::json &j, const char *name, std::optional<T> &value) {
    const auto it = j.find(name);
    if (it != j.end())
        value = it->get<T>();
    else
        value = std::nullopt;
}

///////////////////////////////////////////////////////////////////////////////
// all together
///////////////////////////////////////////////////////////////////////////////
template <typename>
constexpr bool is_optional = false;
template <typename T>
constexpr bool is_optional<std::optional<T>> = true;

template <typename T>
void extended_to_json(const char *key, nlohmann::json &j, const T &value) {
    if constexpr (is_optional<T>)
        nlohmann::optional_to_json(j, key, value);
    else
        j[key] = value;
}
template <typename T>
void extended_from_json(const char *key, const nlohmann::json &j, T &value) {
    if constexpr (is_optional<T>)
        nlohmann::optional_from_json(j, key, value);
    else
        j.at(key).get_to(value);
}

}

#define EXTEND_JSON_TO(v1) extended_to_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);
#define EXTEND_JSON_FROM(v1) extended_from_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);

#define NLOHMANN_JSONIFY_ALL_THINGS(Type, ...)                                          \
  inline void to_json(nlohmann::json &nlohmann_json_j, const Type &nlohmann_json_t) {   \
      NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_TO, __VA_ARGS__))            \
  }                                                                                     \
  inline void from_json(const nlohmann::json &nlohmann_json_j, Type &nlohmann_json_t) { \
      NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_FROM, __VA_ARGS__))          \
  }


namespace ares::MegaDrive {

static n16 Unmapped = 0;

MCD mcd;
#include "bus-internal.cpp"
#include "bus-external.cpp"
#include "io-internal.cpp"
#include "io-external.cpp"
#include "irq.cpp"
#include "cdc.cpp"
#include "cdc-transfer.cpp"
#include "cdd.cpp"
#include "cdd-dac.cpp"
#include "megald.cpp"
#include "timer.cpp"
#include "gpu.cpp"
#include "pcm.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto MCD::load(Node::Object parent, string sourceFile) -> void {
  node = parent->append<Node::Object>("Mega CD");

  tray = node->append<Node::Port>("Disc Tray");
  tray->setFamily("Mega CD");
  tray->setType("Compact Disc");
  tray->setHotSwappable(true);
  tray->setAllocate([&](auto name) { return allocate(tray); });
  tray->setConnect([&] { return connect(); });
  tray->setDisconnect([&] { return disconnect(); });

  bios.allocate   (128_KiB >> 1);
  pram.allocate   (512_KiB >> 1);
  wram.allocate   (256_KiB >> 1);
  bram.allocate   (  8_KiB >> 0);
  cdc.ram.allocate( 16_KiB >> 1);

  cdd.load(node);
  pcm.load(node);
  debugger.load(node);

  if(MegaLD()) {
    ld.load(sourceFile);
  }

  if(auto fp = system.pak->read("bios.rom")) {
    for(auto address : range(bios.size())) bios.program(address, fp->readm(2));
  }
}

auto MCD::unload() -> void {
  if(!node) return;
  disconnect();

  debugger = {};
  cdd.unload(node);
  pcm.unload(node);
  ld.unload();

  bios.reset();
  pram.reset();
  wram.reset();
  bram.reset();
  cdc.ram.reset();

  tray.reset();
  node.reset();
}

auto MCD::allocate(Node::Port parent) -> Node::Peripheral {
  return disc = parent->append<Node::Peripheral>("Mega CD Disc");
}

auto MCD::connect() -> void {
  if(auto fp = system.pak->read("backup.ram")) {
    bram.load(fp);
  }

  if(!disc->setPak(pak = platform->pak(disc))) return;

  information = {};
  information.title = pak->attribute("title");

  fd = pak->read("cd.rom");
  if(!fd) return disconnect();

  cdd.insert();
}

auto MCD::disconnect() -> void {
  if(!disc) return;

  save();
  cdd.eject();
  disc.reset();
  fd.reset();
  pak.reset();
  information = {};
}

auto MCD::save() -> void {
  if(auto fp = system.pak->write("backup.ram")) {
    bram.save(fp);
  }
}

auto MCD::main() -> void {
  if(io.halt) return wait(16);

  if(irq.pending) {
    if(1 > r.i && gpu.irq.lower()) {
      debugger.interrupt("GPU");
      return interrupt(Vector::Level1, 1);
    }
    if(2 > r.i && external.irq.lower()) {
      debugger.interrupt("External");
      return interrupt(Vector::Level2, 2);
    }
    if(3 > r.i && timer.irq.lower()) {
      debugger.interrupt("Timer");
      return interrupt(Vector::Level3, 3);
    }
    if(4 > r.i && cdd.irq.lower()) {
      debugger.interrupt("CDD");
      return interrupt(Vector::Level4, 4);
    }
    if(5 > r.i && cdc.irq.lower()) {
      debugger.interrupt("CDC");
      return interrupt(Vector::Level5, 5);
    }
    if(6 > r.i && irq.subcode.lower()) {
      debugger.interrupt("IRQ");
      return interrupt(Vector::Level6, 6);
    }
    if(irq.reset.lower()) {
      debugger.interrupt("Reset");
      r.a[7] = read(1, 1, 0) << 16 | read(1, 1, 2) << 0;
      r.pc   = read(1, 1, 4) << 16 | read(1, 1, 6) << 0;
      prefetch();
      prefetch();
      return;
    }
  }

  debugger.instruction();
  instruction();
}

auto MCD::step(u32 clocks) -> void {
  gpu.step(clocks);
  counter.divider += clocks;
  while(counter.divider >= 384) {
    counter.divider -= 384;
    cdc.clock();
    cdd.clock();
    timer.clock();
    pcm.clock();
  }
  counter.dma += clocks;
  while(counter.dma >= 6) {
    counter.dma -= 6;
    cdc.transfer.dma();
  }
  counter.pcm += clocks;
  while(counter.pcm >= frequency() / 44100.0) {
    counter.pcm -= frequency() / 44100.0;
    cdd.sample();
  }

  Thread::step(clocks);
}

auto MCD::idle(u32 clocks) -> void {
  step(clocks);
}

auto MCD::wait(u32 clocks) -> void {
  step(clocks);
  Thread::synchronize(cpu);
}

auto MCD::power(bool reset) -> void {
  Thread::create(12'500'000, {&MCD::main, this});
  if(!reset) irq = {};
  resetCpu();
  n32 vec4 = io.vectorLevel4;
  io = {};
  io.vectorLevel4 = reset ? vec4 : n32(~0);
  counter = {};
  led = {};
  external = {};
  communication = {};
  cdc.power(reset);
  timer.power(reset);
  gpu.power(reset);
  pcm.power(reset);
  resetPeripheral(reset);
  if (MegaLD()) {
    ld.power(reset);
  }
}

auto MCD::resetCpu() -> void {
  M68000::power();
  irq.reset.enable = 1;
  irq.reset.raise();
}

// A peripheral reset is expected to take ~100ms according to the dev manual.
// The subcpu continues executing normally during this process.
// The exact operations that occur for this reset are not known.
auto MCD::resetPeripheral(bool reset) -> void {
  cdd.power(reset); // reset cd drive (bios requirement)
}

}
