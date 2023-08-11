#include <ares/debug-server/server.hpp>

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

using string = ::nall::string;
using string_view = ::nall::string_view;

namespace {
  constexpr u32 MAX_REQUESTS_PER_UPDATE = 10;
  constexpr u32 MAX_PACKET_SIZE = 4096;
  constexpr u32 DEF_BREAKPOINT_SIZE = 64;

  auto gdbCalcChecksum(const string &payload) -> string {
    u8 checksum = 0;
    for(char c : payload)checksum += c;
    return hex(checksum, 2, '0');
  }

  auto gdbWrapPayload(const string &payload, bool success = true) -> string {
    return {"+$", payload, '#', gdbCalcChecksum(payload)};
  }
}

namespace ares::GDB {
  Server server{};

  auto Server::isHalted(u64 pc) -> bool {
    if(pc >= 0x80100680 && pc <= 0x80100780) {
      //printf("PC: %08X\n", (u32)pc);
    }

    if(forceHalt)return true;

    for(auto br : breakpoints) {
      if(pc == br) {
        if(waitForSignal) {
          waitForSignal = false;
          forceHalt = true;
          sendSignal(Signal::TRAP);
          printf("HALT!\n");
        }
        return true;
      }
    }
    return false;
  }

  auto Server::processCommand(const string& cmd, bool &shouldReply) -> string
  {
    u32 threadId = 1; // dummy data

    auto cmdParts = cmd.split(":");
    auto cmdName = cmdParts[0];
    char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';

    printf("CMD: %s\n", cmdBuffer.data());

    switch(cmdPrefix)
    {
      case '!': break; // informs us that "extended remote-debugging" is used

      case '?': // why did we halt? (if we halted)
        return "T05"; // @TODO keep track of that

      case 'c': // continue, only reply once we have hit a breakpoint
        if(fakeSignal) {
          fakeSignal = false;
          return "S05";
        }
        shouldReply = false;
        waitForSignal = true;
        forceHalt = false;
        return "";

      case 'g': // dump all general registers
        if(hooks.cmdRegReadGeneral) {
          return hooks.cmdRegReadGeneral();
        } else {
          return "0000000000000000000000000000000000000000";
        }
      break;

      case 'H': return "OK"; // set thread number, can be ignored
      case 'k': break; // "kill" -> see "vKill", can be ignored

      case 'm': // read memory (e.g.: "m80005A00,4")
        {
          if(!hooks.cmdRead) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 count = cmdName.slice(sepIdx+1, cmdName.size()-sepIdx).hex();
          return hooks.cmdRead(address, count, 1);
        }
      break;

      case 'M': // write memory (e.g.: "M801ef90a,4:01000000")
        {
          if(!hooks.cmdWrite) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 unitSize = cmdName.slice(sepIdx+1, 1).hex();
          u64 value = cmdParts.size() > 1 ? cmdParts[1].hex() : 0;

          hooks.cmdWrite(address, unitSize, value);
          return "";
        }

      break;

      case 'p': // read specific register (e.g.: "p15")
        if(hooks.cmdRegRead) {
          u32 regIdx = cmdName.slice(1).integer();
          return hooks.cmdRegRead(regIdx);
        } else {
          return "00000000";
        }
      break;

      case 'q':
        // This tells the client what we can and can't do
        if(cmdName == "qSupported")return {"PacketSize=", MAX_PACKET_SIZE, ";fork-events-;swbreak+;hwbreak+"};

        // handshake-command, most return dummy values to convince gdb to connect
        if(cmdName == "qTStatus")return "";
        if(cmdName == "qAttached")return "1"; // we are always attached, since a game is running
        if(cmdName == "qOffsets")return "Text=0;Data=0;Bss=0;";

        if(cmdName == "qSymbol")return "OK"; // client offers us symbol-names -> we don't care

        // thread info, VSCode requires non-zero values (even though we have no concepts of threads)
        if(cmdName == "qsThreadInfo")return {"m", threadId};
        if(cmdName == "qfThreadInfo")return "l";
        if(cmdName == "qC")return {"QC", threadId};

        printf("Unknown-Command: %s\n", cmdBuffer.data());
        break;


      case 'v':
        if(cmdName == "vMustReplyEmpty")return ""; // handshake-command / keep-alive (must return the same as an unknown command would)
        if(cmdName == "vKill")return ""; // kills process, this may fire before gbd attaches to the current process -> ignore
        if(cmdName == "vAttach")return "S05"; // attaches to the process, we must return a fake trap-exception to make gdb happy
        if(cmdName == "vCont?")return "vCont;c;t"; // tells client what continue-commands we support (continue;stop)

        printf("Unknown-Command: %s\n", cmdBuffer.data());
        break;

      case 'Z': // insert breakpoint (e.g. "Z0,801a0ef4,4")
      case 'z': // remove breakpoint (e.g. "z0,801a0ef4,4")
      {
        bool isInsert = cmdPrefix == 'Z';
        bool isHardware = cmdName[1] == '1'; // 0=software, 1=hardware
        auto sepIdxMaybe = cmdName.findFrom(3, ",");
        u32 sepIdx = sepIdxMaybe ? (sepIdxMaybe.get()+3) : 0;

        u64 address = cmdName.slice(3, sepIdx-1).hex();
        u32 kind = cmdName.slice(sepIdx+1, 1).integer();
        
        if(isInsert) {
          breakpoints.append(address);
        } else {
          breakpoints.removeByValue(address);
        }
        return "OK";
      }
    }

    printf("Unknown-Command: %s\n", cmdBuffer.data());
    return "";
  }

  auto Server::onText(string_view text) -> void {
    for(u32 i=0; i<text.size(); ++i) 
    {
      char c = text[i];
      switch(c) 
      {
        case '$':
          insideCommand = true;
          break;

        case '#': // end of message + 2-char checksum after that
          insideCommand = false;
          i+=2; // skip checksum (we are using TCP after all)

          if(cmdBuffer == "D") {
            printf("GDB ending session, disconnecting client\n");
            sendText("+");
            disconnectClient();
          } else {
            bool shouldReply = true;
            auto cmdRes = processCommand(cmdBuffer, shouldReply);
            if(shouldReply) {
              sendText(gdbWrapPayload(cmdRes));
            } else {
              sendText("+"); // acknowledge always needed
            }
          }

          cmdBuffer = "";
          break;

        case '+': break; // "OK" response -> ignore

        default:
          if(insideCommand) {
            cmdBuffer.append(c);
          }
      }
    }  
  }

  auto Server::sendSignal(u8 code) -> void {
    sendText(gdbWrapPayload({"S", hex(code, 2)}));
  }

  auto Server::onConnect() -> void {
    resetClientData();
    forceHalt = true; // new connections must immediately halt
  }

  auto Server::reset() -> void {
    hooks.cmdRead = nullptr;
    hooks.cmdWrite = nullptr;
    hooks.cmdRegReadGeneral = nullptr;
    hooks.cmdRegRead = nullptr;

    resetClientData();
  }

  auto Server::resetClientData() -> void {
    breakpoints.reset();
    breakpoints.reserve(DEF_BREAKPOINT_SIZE);

    insideCommand = false;
    cmdBuffer = "";
    waitForSignal = false;
    forceHalt = false;
    fakeSignal = true;
  }

};