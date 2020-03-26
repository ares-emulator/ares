auto Emulator::videoUpdate() -> void {
  if(videoInstance && videoInstance.driver() != settings.video.driver) {
    videoInstance.reset();
  }

  if(!videoInstance) {
    videoInstance.create(settings.video.driver);
    videoInstance.setContext(program.viewport.handle());
    if(!videoInstance.ready()) {
      videoInstance.reset();
      videoInstance.create(settings.video.driver = "None");
    }
    settingsMenu.updateShaders();
  }

  if(videoInstance.hasMonitor(settings.video.monitor)) {
    videoInstance.setMonitor(settings.video.monitor);
  } else {
    settings.video.monitor = videoInstance.monitor();
  }

  if(videoInstance.hasExclusive()) {
    videoInstance.setExclusive(settings.video.exclusive);
  } else {
    settings.video.exclusive = videoInstance.exclusive();
  }

  if(videoInstance.hasBlocking()) {
    videoInstance.setBlocking(settings.video.blocking);
  } else {
    settings.video.blocking = videoInstance.blocking();
  }

  if(videoInstance.hasFlush()) {
    videoInstance.setFlush(settings.video.flush);
  } else {
    settings.video.flush = videoInstance.flush();
  }

  if(!videoInstance.hasFormat(settings.video.format)) {
    settings.video.format = videoInstance.hasFormats().first();
  }
  videoInstance.setFormat(settings.video.format);

  if(videoInstance.hasShader()) {
    videoInstance.setShader(settings.video.shader);
  }

  videoUpdateColors();
}

auto Emulator::videoUpdateColors() -> void {
  if(!interface) return;
  for(auto screen : root->find<ares::Node::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}

auto Emulator::videoUpdateShader() -> void {
  videoInstance.setShader(settings.video.shader);
}

auto Emulator::videoToggleFullscreen() -> void {
  if(!videoInstance.hasFullScreen()) return;

  //todo: when not in exclusive mode, it would be prudent to hide or minimize the programWindow.
  //however, doing so causes the keypress to exit fullscreen to fall through to another window.
  //if that window binds the same hotkey, it could trigger fullscreen there (eg xfce4-terminal.)
  //so far, nothing I've tried can prevent this from happening, so I leave programWindow visible.

  videoInstance.clear();  //clear the video in the current mode before changing modes
  if(!videoInstance.fullScreen()) {
    videoInstance.setFullScreen(true);
    if(videoInstance.exclusive()) inputInstance.acquire();
  } else {
    videoInstance.setFullScreen(false);
    inputInstance.release();
    program.viewport.setFocused();
  }
}
