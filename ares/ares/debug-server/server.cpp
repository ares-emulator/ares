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
  constexpr bool NON_STOP_MODE = false;

  auto gdbCalcChecksum(const string &payload) -> string {
    u8 checksum = 0;
    for(char c : payload)checksum += c;
    return hex(checksum, 2, '0');
  }

  auto gdbWrapPayload(const string &payload, bool success = true) -> string {
    return {"+$", payload, '#', gdbCalcChecksum(payload)};
  }

  auto gdbWrapEventPayload(const string &payload, bool success = true) -> string {
    return {"%", payload, '#', gdbCalcChecksum(payload)};
  }

  auto joinIntVec(const vector<u32> &data, const string &delimiter) -> string{
    string res{};
    for(u32 i=0; i<data.size(); ++i) {
      res.append(integer(data[i]), (i<data.size()-1) ? delimiter : "");
    }
    return res;
  }
}

namespace ares::GDB {
  Server server{};

  auto Server::isHalted(u64 pc) -> bool {
    if(pc >= 0x80100680 && pc <= 0x80100780) {
      //printf("PC: %08X\n", (u32)pc);
    }

    bool needHalts = forceHalt || breakpoints.contains(pc);

    if(needHalts) {
      forceHalt = true; // breakpoints may get deleted after a signal, but we have to stay stopped

      if(!haltSignalSent) {
        haltSignalSent = true;
        sendSignal(Signal::TRAP);
        printf("HALT! (signal)\n");
      }
    }
    return needHalts;
  }

  auto Server::processCommand(const string& cmd, bool &shouldReply) -> string
  {
    auto cmdParts = cmd.split(":");
    auto cmdName = cmdParts[0];
    char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';
    u32 mainThreadId = threadIds.size() > 0 ? threadIds[0] : 1;

    printf("CMD: %s\n", cmdBuffer.data());

    switch(cmdPrefix)
    {
      case '!': return "OK"; // informs us that "extended remote-debugging" is used

      case '?': // handshake: why did we halt?
        haltProgram();
        haltSignalSent = true;
        return "T05"; // needs to be faked, otherwise the GDB-client hangs up and eats 100% CPU

      case 'c': // continue
        // normal stop-mode is only allowed to respond once a signal was raised, non-stop must return OK immediately
        shouldReply = NON_STOP_MODE;
        resumeProgram();
        return "OK";

      case 'g': // dump all general registers
        if(hooks.cmdRegReadGeneral) {
          return hooks.cmdRegReadGeneral();
        } else {
          return "0000000000000000000000000000000000000000";
        }
      break;

      case 'H': // set which thread a 'c' command that may follow belongs to (can be ignored in stop-mode)
        if(cmdName == "Hc0")currentThreadC = 0;
        if(cmdName == "Hc-1")currentThreadC = -1;
        return "OK";
      case 'k': return ""; // "kill" -> see "vKill", can be ignored

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
        if(cmdName == "qSupported"){ return {
          "PacketSize=", MAX_PACKET_SIZE, ";fork-events-;swbreak+;hwbreak+", 
          NON_STOP_MODE ? ";QNonStop+" : ""
        };}

        // handshake-command, most return dummy values to convince gdb to connect
        if(cmdName == "qTStatus")return forceHalt ? "Trunning" : "";
        if(cmdName == "qAttached")return "1"; // we are always attached, since a game is running
        if(cmdName == "qOffsets")return "Text=0;Data=0;Bss=0;";

        if(cmdName == "qSymbol")return "OK"; // client offers us symbol-names -> we don't care

        /* // This is correct according to the docs, but makes GDB and CLion hang (@TODO: check why)
        if(cmdName == "qfThreadInfo")return {"m", joinIntVec(threadIds, ",")};
        if(cmdName == "qsThreadInfo")return {"l"};
        if(cmdName == "qThreadExtraInfo,1")return "Runnable"; // ("Runnable", "Blocked", "Mutex") // @TODO: parse this properly, uses "," instead of ":" for params
        if(cmdName == "qC")return {"QC", mainThreadId};
        */

        // These responses are technically wrong, but they make gdb and CLion work.
        if(cmdName == "qsThreadInfo")return {"m1"};
        if(cmdName == "qfThreadInfo")return "l";
        if(cmdName == "qC")return {"QC1"};

        break;

      case 'Q':
        if(cmdName == "QNonStop") { // 0=stop, 1=non-stop-mode (this allows for async GDB-communication)
          if(cmdParts.size() <= 1)return "E00";
          nonStopMode = cmdParts[1] == "1";

          if(nonStopMode) {
            haltProgram();
          } else {
            resumeProgram();
          }
          return "OK";
        }
        break;

      case 'v': {
        // normalize (e.g. "vAttach;1" -> "vAttach")
        auto sepIdxMaybe = cmdName.find(";");
        auto vName = sepIdxMaybe ? cmdName.slice(0, sepIdxMaybe.get()) : cmdName;

        if(vName == "vMustReplyEmpty")return ""; // handshake-command / keep-alive (must return the same as an unknown command would)
        if(vName == "vKill")return ""; // kills process, this may fire before gbd attaches to the current process -> ignore
        if(vName == "vAttach")return NON_STOP_MODE ? "OK" : "S05"; // attaches to the process, we must return a fake trap-exception to make gdb happy
        if(vName == "vCont?")return "vCont;c;t"; // tells client what continue-commands we support (continue;stop)
        //if(vName == "vStopped")return forceHalt ? "S05" : "";
        if(vName == "vStopped")return "";
        if(vName == "vCtrlC") {
          haltProgram();
          return "OK";
        }

      } break;

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

        if(hooks.cmdEmuCacheInvalidate) { // for re-compiler, otherwise breaks might be skipped
          hooks.cmdEmuCacheInvalidate(address);
        }
        return "OK";
      }
    }

    printf("Unknown-Command: %s (data: %s)\n", cmdName.data(), cmdBuffer.data());
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

        case '\x03': // CTRL+C (same as "vCtrlC" packet) -> force halt
          haltProgram();
          break;

        default:
          if(insideCommand) {
            cmdBuffer.append(c);
          }
      }
    }  
  }

  auto Server::sendSignal(u8 code) -> void {
    //sendText(gdbWrapEventPayload({"Stop:S", hex(code, 2)})); // "Non-Stop-Mode" events, doesn't work yet
    sendText(gdbWrapPayload({"S", hex(code, 2)}));
  }

  auto Server::haltProgram() -> void {
    forceHalt = true;
    haltSignalSent = false;
  }

  auto Server::resumeProgram() -> void {
    forceHalt = false;
    haltSignalSent = false;
  }

  auto Server::onConnect() -> void {
    resetClientData();
    haltProgram(); // new connections must immediately halt
  }

  auto Server::reset() -> void {
    hooks.cmdRead.reset();
    hooks.cmdWrite.reset();
    hooks.cmdRegReadGeneral.reset();
    hooks.cmdRegRead.reset();
    hooks.cmdEmuCacheInvalidate.reset();

    resetClientData();
  }

  auto Server::resetClientData() -> void {
    breakpoints.reset();
    breakpoints.reserve(DEF_BREAKPOINT_SIZE);

    threadIds.reset();
    threadIds.append(1);

    insideCommand = false;
    cmdBuffer = "";
    haltSignalSent = false;
    forceHalt = false;

    currentThreadC = -1;
  }

};