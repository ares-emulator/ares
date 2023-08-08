#pragma once

/**
 * Provides an interface between cores and the debugger.
 * This indirection is done to avoid pulling in the whole server + TCP dependency for each core.
 * 
 * This creates a plug-and-play system for cores to register command handlers.
 * All commands are optional.
 */
namespace ares::DebugInterface {
  extern function<string(u32 address, u32 unitCount, u32 unitSize)> commandRead;
  extern function<void(u32 address, u32 unitSize, u64 value)> commandWrite;

  inline auto reset() -> void {
    commandRead = nullptr;
    commandWrite = nullptr;
  }
}