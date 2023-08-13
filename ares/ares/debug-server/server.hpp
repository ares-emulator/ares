#pragma once

#include <nall/tcptext/tcptext-server.hpp>

namespace ares::GDB {

namespace Signal {
  constexpr u8 HUP  = 1;
  constexpr u8 INT  = 2;
  constexpr u8 QUIT = 3;
  constexpr u8 ILL  = 4;
  constexpr u8 TRAP = 5;
  constexpr u8 ABRT = 6;
}

/**
 * GDB based debugging server.
 * This allows for remote debugging over TCP.
 */
class Server : public nall::TCPText::Server {
  public:

    auto reset() -> void;

    struct {
      // Memory
      function<string(u32 address, u32 unitCount, u32 unitSize)> read{};
      function<void(u32 address, u32 unitSize, u64 value)> write{};

      // Registers
      function<string()> regReadGeneral{};
      function<string(u32 regIdx)> regRead{};

      // Emulator
      function<void(u64 address)> emuCacheInvalidate{};
      function<string()> targetXML{};


    } hooks{};

    // Breakpoints
    auto isHalted(u64 pc) -> bool;
    auto hasBreakpoints() const { return breakpoints.size() > 0; }

  protected:
    auto onText(string_view text) -> void override;
    auto onConnect() -> void override;

  private:
    bool insideCommand{false};
    string cmdBuffer{""};
    bool haltSignalSent{false}; // marks if a signal as been sent for new halts (force-halt and breakpoints)
    bool forceHalt{false}; // forces a halt despite no breakpoints being hit
    bool nonStopMode{false};

    s32 currentThreadC{-1}; // selected thread for the next 'c' command

    // client-state:
    vector<u64> breakpoints{}; // prefer vector for data-locality
    vector<u32> threadIds{1};

    auto processCommand(const string& cmd, bool &shouldReply) -> string;
    auto resetClientData() -> void;

    auto sendSignal(u8 code) -> void;

    auto haltProgram() -> void;
    auto resumeProgram() -> void;
};

extern Server server;

}