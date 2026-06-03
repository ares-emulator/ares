#pragma once

#if defined(ARES_ENABLE_RCHEEVOS)

namespace RA::Platform {

struct Adapter {
  // Base contract for per-system RetroAchievements integration.
  // Each adapter maps emulator state to rcheevos console ID, memory view, and ROM/hash input.
  virtual ~Adapter() = default;
  virtual auto supports(const Emulator& emu) const -> bool = 0;
  virtual auto consoleId(const Emulator& emu) const -> u32 = 0;
  virtual auto readMemory(const Emulator& emu, u32 address, u8* buffer, u32 size) const -> u32 = 0;
  virtual auto romData(const Emulator& emu, std::vector<u8>& out) const -> bool = 0;

protected:
  struct MemoryNodeBinding {
    const char* name = nullptr;
    ares::Node::Debugger::Memory* slot = nullptr;
  };

  enum class ReadMode : u8 {
    Linear,
    LinearSwapped,
    Wrapped,
    WrappedSwapped,
  };

  struct Region {
    u32 start = 0;
    u32 end = 0;
    ReadMode mode = ReadMode::Linear;
    u32 span = 0;
    const ares::Node::Debugger::Memory* memory = nullptr;
  };

  using CompiledMap = std::vector<Region>;

  struct NodeCache {
    ares::Node::Object rootNode;
  };

  static auto supportsAnyName(const Emulator& emu, std::initializer_list<const char*> names) -> bool;
  static auto refreshMemoryNodeCache(
    NodeCache& cache,
    const ares::Node::System* currentRoot,
    std::initializer_list<MemoryNodeBinding> bindings,
    bool* changed = nullptr
  ) -> bool;
  // Shared memory-reader backend for adapter helper variants.
  // - Linear modes clamp to [offset, span) and may short-read at the end.
  // - Wrapped modes always return size bytes by modulo-wrapping over span.
  // - Swapped modes apply addr^1 byte-lane selection (used by 16-bit bus layouts).
  static auto readFromMemoryMode(
    const ares::Node::Debugger::Memory& memory, u32 offset, u8* buffer, u32 size, u32 maxSpan, ReadMode mode
  ) -> u32;
  // Linear read helper with end-clamping (no wrapping, no byte-lane swap).
  static auto readFromMemory(const ares::Node::Debugger::Memory& memory, u32 offset, u8* buffer, u32 size, u32 maxSpan) -> u32;
  // Linear read helper with end-clamping and addr^1 byte-lane swap.
  static auto readFromMemorySwapped(const ares::Node::Debugger::Memory& memory, u32 offset, u8* buffer, u32 size, u32 maxSpan) -> u32;
  // Wrapped read helper (modulo over full memory span, no byte-lane swap).
  static auto readFromMemoryWrapped(const ares::Node::Debugger::Memory& memory, u32 offset, u8* buffer, u32 size) -> u32;
  // Wrapped read helper with modulo addressing and addr^1 byte-lane swap.
  static auto readFromMemoryWrappedSwapped(const ares::Node::Debugger::Memory& memory, u32 offset, u8* buffer, u32 size) -> u32;
  // Compiles a declarative RA memory layout into explicit byte ownership.
  static auto compileMemoryMap(std::initializer_list<Region> regions) -> CompiledMap;
  // Reads one compiled region mapping from RA address space into a debugger memory node.
  static auto readMappedRegion(const Region& region, u32 address, u8* buffer, u32 size) -> u32;
  // Reads through a compiled map, which should already have resolved any optional sources.
  static auto readMemoryMap(const CompiledMap& regions, u32 address, u8* buffer, u32 size) -> u32;
};

auto registerNintendoAdapters(std::vector<Adapter*>& list) -> void;
auto registerSegaAdapters(std::vector<Adapter*>& list) -> void;
auto adapters() -> std::vector<Adapter*>&;
auto selectAdapter(const Emulator& emu) -> Adapter*;

}

#endif
