#include <ares/debug-server/server.hpp>
#include <ares/debug-server/server-interface.hpp>

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

using string = ::nall::string;
using string_view = ::nall::string_view;


namespace ares::DebugServer {
  Server server{};
}

namespace {
  constexpr u32 MAX_REQUESTS_PER_UPDATE = 10;
  bool insideCommand{false};
  string cmdBuffer{""};

  auto calcGdbChecksum(const string &payload) {
    u8 checksum = 0;
    for(char c : payload) {
      checksum += c;
    }
    return hex(checksum, 2, '0');
  }

  auto encodeGdb(const string &payload, bool success = true) {
    string res{"+$"};
    res.append(payload);
    res.append("#");
    res.append(calcGdbChecksum(payload));
    return res;
  }

  auto processCommand(const string& cmd) -> string 
  {
    u32 threadId = 1; // dummy data

    auto cmdParts = cmd.split(":");
    auto cmdName = cmdParts[0];
    char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';

    printf("CMD: %s\n", cmdBuffer.data());

    switch(cmdPrefix)
    {
      case 'q':
        // This tell the client what we can and can't do:
        // NOT Supported: software-breakpoints (use instructions for that)
        // Supported: hardware-breakpoints, XML-Register info (important for arch-detection)
        if(cmdName == "qSupported")return "fork-events-;swbreak-;hwbreak+;xmlRegisters=mips"; // @TODO check if xmlRegisters even does anything (let cores se this)

        // handshake-command, most return dummy values to get convince gdb to connect
        if(cmdName == "qTStatus")return "";
        if(cmdName == "qAttached")return "1"; // we are always attached, since a game is running
        if(cmdName == "qOffsets")return "Text=0;Data=0;Bss=0;";

        // thread info, VSCode requires non-zero values (even though we have no concepts of threads)
        if(cmdName == "qsThreadInfo")return {"m", threadId};
        if(cmdName == "qfThreadInfo")return "l";
        if(cmdName == "qC")return {"QC", threadId};

        printf("Command: %s\n", cmdBuffer.data());
        break;

      case 'v':
        if(cmdName == "vMustReplyEmpty")return ""; // handshake-command / keep-alive
        if(cmdName == "vKill")return ""; // kills process, this may fire before gbd attaches to the current process -> ignore
        if(cmdName == "vAttach")return "S05"; // attaches to the process, we must return a fake trap-exception to make gdb happy

        printf("Command: %s\n", cmdBuffer.data());
        break;

      case '!': break; // informs us that "extended remote-debugging" is used

      case '?': // why did we halt? (if we halted)
        return "T05"; // @TODO keep track of that
      break;

      case 'g': // dump all general registers
        if(ares::DebugInterface::cmdRegReadGeneral) {
          return ares::DebugInterface::cmdRegReadGeneral();
        } else {
          return "0000000000000000000000000000000000000000";
        }
      break;

      case 'p': // read specific register
        if(ares::DebugInterface::cmdRegRead) {
          u32 regIdx = cmdName.slice(1).integer();
          return ares::DebugInterface::cmdRegRead(regIdx);
        } else {
          return "00000000";
        }
      break;

      case 'm': // read memory (e.g.: "m80005A00,4")
        {
          if(!ares::DebugInterface::cmdRead) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 count = cmdName.slice(sepIdx+1, cmdName.size()-sepIdx).hex();
          return ares::DebugInterface::cmdRead(address, count, 1);
        }
      break;

      case 'M': // write memory (e.g.: "M801ef90a,4:01000000")
        {
          if(!ares::DebugInterface::cmdWrite) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 unitSize = cmdName.slice(sepIdx+1, 1).hex();
          u64 value = cmdParts.size() > 1 ? cmdParts[1].hex() : 0;

          ares::DebugInterface::cmdWrite(address, unitSize, value);
          return "";
        }

      break;

      case 'k': break; // "kill" -> see "vKill", can be ignored
      case 'H': return "OK"; // set thread number, can be ignored

      case 'c': // continue, let game run until next trap-exception
        return "S05"; // @TODO do this fr
      break;
    }

    printf("Command: %s\n", cmdBuffer.data());
    return "";
  }
}

namespace ares::DebugInterface {
  // server-interface.hpp
  function<string(u32 address, u32 unitCount, u32 unitSize)> cmdRead;
  function<void(u32 address, u32 unitSize, u64 value)> cmdWrite;
  function<string()> cmdRegReadGeneral;
  function<string(u32 regIdx)> cmdRegRead;
}

namespace ares::DebugServer {

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
            sendText(
              encodeGdb(processCommand(cmdBuffer))
            );
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


  auto Server::reset() -> void {
    ares::DebugInterface::reset();
  }

};