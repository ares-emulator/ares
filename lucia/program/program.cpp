#include "../lucia.hpp"
#include "platform.cpp"
#include "load.cpp"
#include "states.cpp"
#include "rewind.cpp"
#include "status.cpp"
#include "utility.cpp"
#include "drivers.cpp"

Program program;

auto Program::create() -> void {
  ares::platform = this;

  videoDriverUpdate();
  audioDriverUpdate();
  inputDriverUpdate();

  driverSettings.videoRefresh();
  driverSettings.audioRefresh();
  driverSettings.inputRefresh();

  if(startGameLoad) {
    if(auto emulator = identify(startGameLoad)) {
      if(load(emulator, startGameLoad)) {
        if(startFullScreen) videoFullScreenToggle();
      }
    }
  }
}

auto Program::main() -> void {
  if(Application::modal()) {
    ruby::audio.clear();
    return;
  }

  updateMessage();
  inputManager.poll();
  inputManager.pollHotkeys();
  bool defocused = driverSettings.inputDefocusPause.checked() && !ruby::video.fullScreen() && !presentation.focused();
  if(emulator && defocused) message.text = "Paused";
  if(!emulator || paused || defocused) {
    ruby::audio.clear();
    usleep(20 * 1000);
    return;
  }

  rewindRun();

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

  if(settings.general.autoSaveMemory) {
    static u64 previousTime = chrono::timestamp();
    u64 currentTime = chrono::timestamp();
    if(currentTime - previousTime >= 30) {
      previousTime = currentTime;
      emulator->save();
    }
  }

  //if Platform::video() changed the screen resolution, resize the presentation window here.
  //window operations must be performed from the main thread.
  if(emulator->latch.changed) {
    emulator->latch.changed = false;
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
  }

  memoryEditor.liveRefresh();
  graphicsViewer.liveRefresh();
  propertiesViewer.liveRefresh();
}

auto Program::quit() -> void {
  unload();
  presentation.setVisible(false);  //makes quitting the emulator feel more responsive
  Application::processEvents();
  Application::quit();

  ruby::video.reset();
  ruby::audio.reset();
  ruby::input.reset();
}
