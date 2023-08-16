#include <ares/debug-server/server.hpp>

// @TODO: why does nall need this?
#include <cstdlib>
#include <utility>

using string = ::nall::string;
using string_view = ::nall::string_view;

namespace {
  constexpr bool GDB_LOG_MESSAGES = false;

  constexpr u32 MAX_REQUESTS_PER_UPDATE = 10;
  constexpr u32 MAX_PACKET_SIZE = 4096;
  constexpr u32 DEF_BREAKPOINT_SIZE = 64;
  constexpr bool NON_STOP_MODE = false; // @TODO: broken, only useful for multi-thread debugging

  auto gdbCalcChecksum(const string &payload) -> u8 {
    u8 checksum = 0;
    for(char c : payload)checksum += c;
    return checksum;
  }
}

namespace ares::GDB {
  Server server{};

  auto Server::isHalted(u64 pc) -> bool {
    bool needHalts = forceHalt || breakpoints.contains(pc);

    if(needHalts) {
      forceHalt = true; // breakpoints may get deleted after a signal, but we have to stay stopped

      if(!haltSignalSent) {
        haltSignalSent = true;
        sendSignal(Signal::TRAP);
      }
    }
    return needHalts;
  }

  auto Server::processCommand(const string& cmd, bool &shouldReply) -> string
  {
    auto cmdParts = cmd.split(":");
    auto cmdName = cmdParts[0];
    char cmdPrefix = cmdName.size() > 0 ? cmdName[0] : ' ';

    if constexpr(GDB_LOG_MESSAGES) {
      printf("GDB <: %s\n", cmdBuffer.data());
    }

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
        if(hooks.regReadGeneral) {
          return hooks.regReadGeneral();
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
          if(!hooks.read) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 count = cmdName.slice(sepIdx+1, cmdName.size()-sepIdx).hex();
          return hooks.read(address, count);
        }
      break;

      case 'M': // write memory (e.g.: "M801ef90a,4:01000000")
        {
          if(!hooks.write) {
            return "";
          }

          auto sepIdxMaybe = cmdName.find(",");
          u32 sepIdx = sepIdxMaybe ? sepIdxMaybe.get() : 1;

          u64 address = cmdName.slice(1, sepIdx-1).hex();
          u64 unitSize = cmdName.slice(sepIdx+1, 1).hex();
          u64 value = cmdParts.size() > 1 ? cmdParts[1].hex() : 0;

          hooks.write(address, unitSize, value);
          return "";
        }

      break;

      case 'p': // read specific register (e.g.: "p15")
        if(hooks.regRead) {
          u32 regIdx = cmdName.slice(1).integer();
          return hooks.regRead(regIdx);
        } else {
          return "00000000";
        }
      break;

      case 'q':
        // This tells the client what we can and can't do
        if(cmdName == "qSupported"){ return {
          "PacketSize=", MAX_PACKET_SIZE, 
          ";fork-events-;swbreak+;hwbreak+", 
          NON_STOP_MODE ? ";QNonStop+" : "",
          hooks.targetXML ? ";xmlRegisters+;qXfer:features:read+" : "" // (see: https://marc.info/?l=gdb&m=149901965961257&w=2)
        };}

        // handshake-command, most return dummy values to convince gdb to connect
        if(cmdName == "qTStatus")return forceHalt ? "T1" : "";
        if(cmdName == "qAttached")return "1"; // we are always attached, since a game is running
        if(cmdName == "qOffsets")return "Text=0;Data=0;Bss=0;";

        if(cmdName == "qSymbol")return "OK"; // client offers us symbol-names -> we don't care

        // client asks us about existing breakpoints (may happen after a re-connect) -> ignore since we clear them on connect
        if(cmdName == "qTfP")return "";
        if(cmdName == "qTsP")return "";

        // extended target features (gdb extension), most return XML data
        if(cmdName == "qXfer" && cmdParts.size() > 4) 
        {
          if(cmdParts[1] == "features" && cmdParts[2] == "read") {
            // informs the client about arch/registers (https://sourceware.org/gdb/onlinedocs/gdb/Target-Description-Format.html#Target-Description-Format)
            if(cmdParts[3] == "target.xml") {
              return hooks.targetXML ? string{"l", hooks.targetXML()} : string{""};
            }
          }
        }

        /* // This is correct according to the docs, but makes GDB and CLion hang (@TODO: check why)
        if(cmdName == "qfThreadInfo")return {"m", joinIntVec(threadIds, ",")};
        if(cmdName == "qsThreadInfo")return {"l"};
        if(cmdName == "qThreadExtraInfo,1")return "Runnable"; // ("Runnable", "Blocked on Mutex") // @TODO: parse this properly, uses "," instead of ":" for params
        if(cmdName == "qC")return {"QC", mainThreadId};
        */

        // Wrong responses, but they make gdb and CLion work.
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

        if(hooks.emuCacheInvalidate) { // for re-compiler, otherwise breaks might be skipped
          hooks.emuCacheInvalidate(address);
        }
        return "OK";
      }
    }

    printf("Unknown-Command: %s (data: %s)\n", cmdName.data(), cmdBuffer.data());
    return "";
  }

  auto Server::onText(string_view text) -> void {

    if(cmdBuffer.size() == 0) {
      cmdBuffer.reserve(text.size());
    }

    for(char c : text) 
    {
      switch(c) 
      {
        case '$':
          insideCommand = true;
          break;

        case '#': // end of message + 2-char checksum after that
          insideCommand = false;

          if(cmdBuffer == "D") {
            printf("GDB ending session, disconnecting client\n");
            sendText("+");
            disconnectClient();
            resumeProgram();
          } else {
            ++messageCount;
            bool shouldReply = true;
            auto cmdRes = processCommand(cmdBuffer, shouldReply);
            if(shouldReply) {
              sendPayload(cmdRes);
            } else {
              sendText("+"); // acknowledge always needed
            }
          }

          cmdBuffer = "";
          break;

        case '+': break; // "OK" response -> ignore

        case '\x03': // CTRL+C (same as "vCtrlC" packet) -> force halt
          if constexpr(GDB_LOG_MESSAGES) {
            printf("GDB <: CTRL+C [0x03]");
          }
          haltProgram();
          break;

        default:
          if(insideCommand) {
            cmdBuffer.append(c);
          }
      }
    }  
  }

  auto Server::updateLoop() -> void {

    // @TODO: refactor
    constexpr u32 LOOP_COUNT = 100;
    constexpr u32 LOOP_COUNT_HALT = 100;

    if(isHalted()) 
    {
      for(u32 frame=0; frame<10; ++frame) {
        for(u32 i=0; i<LOOP_COUNT_HALT; ++i) {
          messageCount = 0;
          update();
          if(!isHalted())return;
          if(messageCount > 0) {
            i = LOOP_COUNT_HALT;
          }
        }
        usleep(1);
      }
      return;
    }

    for(u32 i=0; i<LOOP_COUNT; ++i) {
      messageCount = 0;
      update();
      if(messageCount > 0) {
        i = LOOP_COUNT;
      }
    }
  }

  auto Server::sendSignal(u8 code) -> void {
    sendPayload({"S", hex(code, 2)});
  }

  auto Server::sendPayload(const string& payload) -> void {
    string msg{"+$", payload, '#', hex(gdbCalcChecksum(payload), 2, '0')};
    if constexpr(GDB_LOG_MESSAGES) {
      printf("GDB >: %.*s\n", msg.size() > 100 ? 100 : msg.size(), msg.data());
    }
    sendText(msg);
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
    hooks.read.reset();
    hooks.write.reset();
    hooks.regReadGeneral.reset();
    hooks.regRead.reset();
    hooks.emuCacheInvalidate.reset();
    hooks.targetXML.reset();

    resetClientData();
  }

  auto Server::resetClientData() -> void {
    breakpoints.reset();
    breakpoints.reserve(DEF_BREAKPOINT_SIZE);

    insideCommand = false;
    cmdBuffer = "";
    haltSignalSent = false;
    forceHalt = false;

    currentThreadC = -1;
  }

};