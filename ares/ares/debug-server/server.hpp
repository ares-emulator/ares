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
      function<string(u32 address, u32 unitCount, u32 unitSize)> cmdRead{nullptr};
      function<void(u32 address, u32 unitSize, u64 value)> cmdWrite{nullptr};

      // Registers
      function<string()> cmdRegReadGeneral{nullptr};
      function<string(u32 regIdx)> cmdRegRead{nullptr};

    } hooks{};

    // Breakpoints
    auto isHalted(u64 pc) -> bool;

  protected:
    auto onText(string_view text) -> void override;
    auto onConnect() -> void override;

  private:
    bool insideCommand{false};
    string cmdBuffer{""};
    bool waitForSignal{false}; // used to only send a signal to the client once
    bool forceHalt{false}; // forces a halt despite no breakpoints being hit
    bool fakeSignal{true}; // flags, forces initial signal (otherwise GDB hangs)

    vector<u64> breakpoints{}; // prefer vector for data-locality
    auto processCommand(const string& cmd, bool &shouldReply) -> string;
    auto sendSignal(u8 code) -> void;
    auto resetClientData() -> void;
};

extern Server server;

}