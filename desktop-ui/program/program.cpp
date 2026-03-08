#include "../desktop-ui.hpp"
#include "platform.cpp"
#include "load.cpp"
#include "states.cpp"
#include "rewind.cpp"
#include "status.cpp"
#include "utility.cpp"
#include "drivers.cpp"
#include "nci.cpp"

Program program;
thread worker;

auto Program::create() -> void {
  ares::platform = this;

  videoDriverUpdate();
  audioDriverUpdate();
  inputDriverUpdate();

  if(kiosk) {
    if(startFullScreen) videoFullScreenToggle();
    if(startPseudoFullScreen) videoPseudoFullScreenToggle();
  }

  nci.open();

  _isRunning = true;
  worker = thread::create(std::bind_front(&Program::emulatorRunLoop, this));
  program.rewindReset();

  if(!startGameLoad.empty()) {
    Program::Guard guard;
    auto gameToLoad = startGameLoad.front();
    startGameLoad.erase(startGameLoad.begin());
    if(startSystem) {
      for(auto &emulator: emulators) {
        if(emulator->name == startSystem) {
          if(load(emulator, gameToLoad)) {
            if(!kiosk) {
              if(startFullScreen) videoFullScreenToggle();
              if(startPseudoFullScreen) videoPseudoFullScreenToggle();
            }
          }
          return;
        }
      }
      return;
    }

    if(auto emulator = identify(gameToLoad)) {
      if(load(emulator, gameToLoad)) {
        if(!kiosk) {
          if(startFullScreen) videoFullScreenToggle();
          if(startPseudoFullScreen) videoPseudoFullScreenToggle();
        }
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
      nci.updateLoop();
      usleep(20 * 1000);
      continue;
    }

    if(emulator && nall::GDB::server.isHalted()) {
      ruby::audio.clear();
      nall::GDB::server.updateLoop(); // sleeps internally
      nci.updateLoop();
      continue;
    }

    bool defocused = settings.input.defocus == "Pause" && !ruby::video.fullScreen() && !presentation.focused();

    if(!emulator || (paused && !program.requestFrameAdvance) || defocused) {
      ruby::audio.clear();
      nall::GDB::server.updateLoop();
      nci.updateLoop();
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
    nci.updateLoop();

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

  if(pendingKioskExit) {
    pendingKioskExit = false;
    quit();
    return;
  }
  
  if(requestFullscreenToggle) {
    requestFullscreenToggle = false;
    videoFullScreenToggle();
  }

  if(requestQuit) {
    requestQuit = false;
    quit();
    return;
  }

  if(requestUnload) {
    requestUnload = false;
    if(emulator) {
      Program::Guard guard;
      unload();
      if(settings.video.adaptiveSizing) presentation.resizeWindow();
      presentation.showIcon(true);
    }
  }

  if(requestLoad) {
    requestLoad = false;
    Program::Guard guard;
    string path = requestLoadPath;
    string system = requestLoadSystem;
    requestLoadPath = "";
    requestLoadSystem = "";

    std::shared_ptr<Emulator> emu;
    if(system) {
      for(auto& e : emulators) {
        if(e->name == system) { emu = e; break; }
      }
    } else {
      emu = identify(path);
    }

    if(emu) {
      load(emu, path);
    }
  }

  inputManager.poll();
  inputManager.pollHotkeys();

  updateMessage();

  //If Platform::video() changed the screen resolution, resize the presentation window here.
  //Window operations must be performed from the main thread.
  
  if(_needsResize) {
    if(settings.video.adaptiveSizing && !startPseudoFullScreen) presentation.resizeWindow();
    _needsResize = false;
  }

  if(toolsWindowConstructed) {
    memoryEditor.liveRefresh();
    graphicsViewer.liveRefresh();
    propertiesViewer.liveRefresh();
    tapeViewer.liveRefresh();
  }
}

auto Program::quit() -> void {
  nci.close();
  Program::Guard guard;
  _quitting = true;
  if(lock.owns_lock()) {
    lock.unlock();
  }
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
