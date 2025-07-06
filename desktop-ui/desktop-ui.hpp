#include <ruby/ruby.hpp>
//using namespace ruby;

#include <hiro/hiro.hpp>
using namespace hiro;

#include <ares/ares.hpp>
#include <ares/resource/resource.hpp>
#include <nall/gdb/server.hpp>
#include <mia/mia.hpp>

#include <nall/instance.hpp>
#include <nall/encode/png.hpp>
#include <nall/hash/crc16.hpp>

namespace ruby {
  extern Video video;
  extern Audio audio;
  extern Input input;
}

#include "resource/resource.hpp"
#include "input/input.hpp"
#include "emulator/emulator.hpp"
#include "game-browser/game-browser.hpp"
#include "program/program.hpp"
#include "presentation/presentation.hpp"
#include "settings/settings.hpp"
#include "tools/tools.hpp"

/// Wrapper for the synchronization primitives used to protect access to the emulator program state, with RAII semantics. A `Program::Guard` instance should be created whenever an operation is performed that modifies resources owned by the emulator/program thread.
class Program::Guard {
public:
  Guard() {
    if(program._isRunning && !program._programThread) {
      program._interruptDepth += 1;
      if(program._interruptDepth == 1) {
        lock.lock();
        program._interruptWaiting = true;
        program._programConditionVariable.notify_one();
        program._programConditionVariable.wait(lock, [] { return program._interruptWorking; });
      }
    }
  }
  
  ~Guard() {
    if(program._isRunning && !program._programThread) {
      program._interruptDepth -= 1;
      if(program._interruptDepth == 0) {
        program._interruptWorking = false;
        program._interruptWaiting = false;
        lock.unlock();
        program._programConditionVariable.notify_one();
      }
    }
  }
  
private:
  std::unique_lock<std::mutex> lock{program._programMutex, std::defer_lock};
};

auto locate(const string& name) -> string;
