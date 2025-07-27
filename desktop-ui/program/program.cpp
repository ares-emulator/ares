#include "../desktop-ui.hpp"
#include "platform.cpp"
#include "load.cpp"
#include "states.cpp"
#include "rewind.cpp"
#include "status.cpp"
#include "utility.cpp"
#include "drivers.cpp"

Program program;
thread worker;

auto Program::create() -> void {
  ares::platform = this;

  videoDriverUpdate();
  audioDriverUpdate();
  inputDriverUpdate();

  _isRunning = true;
  worker = thread::create({&Program::emulatorRunLoop, this});

  if(startGameLoad) {
    Program::Guard guard;
    auto gameToLoad = startGameLoad.takeFirst();
    if(startSystem) {
      for(auto &emulator: emulators) {
        if(emulator->name == startSystem) {
          if(load(emulator, gameToLoad)) {
            if(startFullScreen) videoFullScreenToggle();
          }
          return;
        }
      }
    }

    if(auto emulator = identify(gameToLoad)) {
      if(load(emulator, gameToLoad)) {
        if(startFullScreen) videoFullScreenToggle();
      }
    }
  }
}

auto Program::waitForInterrupts() -> void {
  std::unique_lock<std::mutex> lock(_programMutex);
  _interruptWorking = true;
  _programConditionVariable.notify_one();
  _programConditionVariable.wait(lock, [this] { return !_interruptWorking || _quitting; });
}

auto Program::emulatorRunLoop(uintptr_t) -> void {
  thread::setName("dev.ares.worker");
  _programThread = true;
  while(!_quitting) {
    // Allow other threads to carry out tasks between emulator run loop iterations
    if(_interruptWaiting) {
      waitForInterrupts();
      continue;
    }
    if(!emulator) {
      usleep(20 * 1000);
      continue;
    }

    if(emulator && nall::GDB::server.isHalted()) {
      ruby::audio.clear();
      nall::GDB::server.updateLoop(); // sleeps internally
      continue;
    }

    bool defocused = driverSettings.inputDefocusPause.checked() && !ruby::video.fullScreen() && !presentation.focused();

    if(!emulator || (paused && !program.requestFrameAdvance) || defocused) {
      ruby::audio.clear();
      nall::GDB::server.updateLoop();
      usleep(20 * 1000);
      continue;
    }

    rewindRun();

    nall::GDB::server.updateLoop();

    program.requestFrameAdvance = false;
    if(!runAhead || fastForwarding || rewinding) {
      emulator->root->run();
    } else {
      ares::setRunAhead(true);
      emulator->root->run();
      auto state = emulator->root->serialize(false);
      ares::setRunAhead(false);
      emulator->root->run();
      state.setReading();
      emulator->root->unserialize(state);
    }

    nall::GDB::server.updateLoop();

    if(settings.general.autoSaveMemory) {
      static u64 previousTime = chrono::timestamp();
      u64 currentTime = chrono::timestamp();
      if(currentTime - previousTime >= 30) {
        previousTime = currentTime;
        emulator->save();
      }
    }

    if(emulator->latch.changed) {
      emulator->latch.changed = false;
      _needsResize = true;
    }
  }
}

auto Program::main() -> void {
  if(Application::modal()) {
    ruby::audio.clear();
    return;
  }
  
  inputManager.poll();
  inputManager.pollHotkeys();

  updateMessage();

  //If Platform::video() changed the screen resolution, resize the presentation window here.
  //Window operations must be performed from the main thread.
  
  if(_needsResize) {
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
    _needsResize = false;
  }

  memoryEditor.liveRefresh();
  graphicsViewer.liveRefresh();
  propertiesViewer.liveRefresh();
}

auto Program::quit() -> void {
  Program::Guard guard;
  _quitting = true;
  lock.unlock();
  _programConditionVariable.notify_all();
  worker.join();
  program._isRunning = false;
  unload();
  Application::processEvents();
  Application::quit();

  ruby::video.reset();
  ruby::audio.reset();
  ruby::input.reset();
}
