auto DriverSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  videoLabel.setText("Video").setFont(Font().setBold());
  videoDriverList.onChange([&] {
    bool enabled = false;
    if(videoDriverList.selected().text() != settings.video.driver) { enabled = true; }
    videoDriverAssign.setEnabled(enabled);
  });
  videoDriverLabel.setText("Driver:");
  videoDriverAssign.setText("Apply").setEnabled(false).onActivate([&] {
    settings.video.driver = videoDriverList.selected().text();
    if (videoDriverUpdate()) {
      videoDriverAssign.setEnabled(false);
    }
  });
  videoMonitorLabel.setText("Fullscreen monitor:");
  videoMonitorList.onChange([&] {
    settings.video.monitor = videoMonitorList.selected().text();
    program.videoMonitorUpdate();
    videoRefresh();
  });
  videoFormatLabel.setText("Format:");
  videoFormatList.onChange([&] {
    settings.video.format = videoFormatList.selected().text();
    program.videoFormatUpdate();
    videoRefresh();
  });
#if !defined(PLATFORM_MACOS)
  videoExclusiveToggle.setText("Exclusive mode").onToggle([&] {
    settings.video.exclusive = videoExclusiveToggle.checked();
    ruby::video.setExclusive(settings.video.exclusive);
  });
#endif
  videoBlockingToggle.setText("Synchronize").onToggle([&] {
    settings.video.blocking = videoBlockingToggle.checked();
    ruby::video.setBlocking(settings.video.blocking);
  });
  videoFlushToggle.setText("GPU sync").onToggle([&] {
    settings.video.flush = videoFlushToggle.checked();
    ruby::video.setFlush(settings.video.flush);
  });
#if defined(PLATFORM_MACOS)
  videoColorSpaceToggle.setText("Force sRGB").onToggle([&] {
    settings.video.forceSRGB = videoColorSpaceToggle.checked();
    ruby::video.setForceSRGB(settings.video.forceSRGB);
  });
  videoThreadedRendererToggle.setText("Threaded").onToggle([&] {
    settings.video.threadedRenderer = videoThreadedRendererToggle.checked();
    ruby::video.setThreadedRenderer(settings.video.threadedRenderer);
  });
  videoNativeFullScreenToggle.setText("Use native fullscreen").onToggle([&] {
    settings.video.nativeFullScreen = videoNativeFullScreenToggle.checked();
    ruby::video.setNativeFullScreen(settings.video.nativeFullScreen);
    videoRefresh();
  });
#endif

  audioLabel.setText("Audio").setFont(Font().setBold());
  audioDriverList.onChange([&] {
    bool enabled = false;
    if(audioDriverList.selected().text() != settings.audio.driver) { enabled = true; }
    audioDriverAssign.setEnabled(enabled);
  });
  audioDriverLabel.setText("Driver:");
  audioDriverAssign.setText("Apply").setEnabled(false).onActivate([&] {
    settings.audio.driver = audioDriverList.selected().text();
    if (audioDriverUpdate()) {
      audioDriverAssign.setEnabled(false);
    }
  });
  audioDeviceLabel.setText("Output device:");
  audioDeviceList.onChange([&] {
    settings.audio.device = audioDeviceList.selected().text();
    program.audioDeviceUpdate();
    audioRefresh();
  });
  audioFrequencyLabel.setText("Frequency:");
  audioFrequencyList.onChange([&] {
    settings.audio.frequency = audioFrequencyList.selected().text().natural();
    program.audioFrequencyUpdate();
    audioRefresh();
  });
  audioLatencyLabel.setText("Latency:");
  audioLatencyList.onChange([&] {
    settings.audio.latency = audioLatencyList.selected().text().natural();
    program.audioLatencyUpdate();
    audioRefresh();
  });
  audioExclusiveToggle.setText("Exclusive mode").onToggle([&] {
    lock_guard<recursive_mutex> programLock(program.programMutex);
    settings.audio.exclusive = audioExclusiveToggle.checked();
    ruby::audio.setExclusive(settings.audio.exclusive);
  });
  audioBlockingToggle.setText("Synchronize").onToggle([&] {
    lock_guard<recursive_mutex> programLock(program.programMutex);
    settings.audio.blocking = audioBlockingToggle.checked();
    ruby::audio.setBlocking(settings.audio.blocking);
  });
  audioDynamicToggle.setText("Dynamic rate").onToggle([&] {
    lock_guard<recursive_mutex> programLock(program.programMutex);
    settings.audio.dynamic = audioDynamicToggle.checked();
    ruby::audio.setDynamic(settings.audio.dynamic);
  });

  inputLabel.setText("Input").setFont(Font().setBold());
  inputDriverList.onChange([&] {
    bool enabled = false;
    if(inputDriverList.selected().text() != settings.input.driver) { enabled = true; }
    inputDriverAssign.setEnabled(enabled);
  });
  inputDriverLabel.setText("Driver:");
  inputDriverAssign.setText("Apply").setEnabled(false).onActivate([&] {
    settings.input.driver = inputDriverList.selected().text();
    if (inputDriverUpdate()) {
      inputDriverAssign.setEnabled(false);
    }
  });
  inputDefocusLabel.setText("When focus is lost:");
  inputDefocusPause.setText("Pause emulation").onActivate([&] {
    settings.input.defocus = "Pause";
  });
  inputDefocusBlock.setText("Block input").onActivate([&] {
    settings.input.defocus = "Block";
  });
  inputDefocusAllow.setText("Allow input").onActivate([&] {
    settings.input.defocus = "Allow";
  });
  if(settings.input.defocus == "Pause") inputDefocusPause.setChecked();
  if(settings.input.defocus == "Block") inputDefocusBlock.setChecked();
  if(settings.input.defocus == "Allow") inputDefocusAllow.setChecked();

  driverApplyHint.setText("Note: You must click on the 'Apply' button to update and save changes to driver selection.").setFont(Font().setSize(8.0)).setForegroundColor(SystemColor::Sublabel);
    
  videoDriverLayout.setPadding(12_sx, 0);
  videoPropertyLayout.setPadding(12_sx, 0);
  videoToggleLayout.setPadding(12_sx, 0);
  audioDriverLayout.setPadding(12_sx, 0);
  audioDeviceLayout.setPadding(12_sx, 0);
  audioPropertyLayout.setPadding(12_sx, 0);
  audioToggleLayout.setPadding(12_sx, 0);
  inputDriverLayout.setPadding(12_sx, 0);
  inputDefocusLayout.setPadding(12_sx, 0);
}

auto DriverSettings::videoRefresh() -> void {
  videoDriverList.reset();
  for(auto& driver : ruby::video.hasDrivers()) {
    ComboButtonItem item{&videoDriverList};
    item.setText(driver);
    if(driver == ruby::video.driver()) item.setSelected();
    if(settings.video.driver == ruby::video.driver()) {
      videoDriverAssign.setEnabled(false);
    }
  }
  videoMonitorList.reset();
  for(auto& monitor : ruby::video.hasMonitors()) {
    ComboButtonItem item{&videoMonitorList};
    item.setText(monitor.name);
    if(monitor.name == ruby::video.monitor()) item.setSelected();
  }
  videoFormatList.reset();
  for(auto& format : ruby::video.hasFormats()) {
    ComboButtonItem item{&videoFormatList};
    item.setText(format);
    if(format == ruby::video.format()) item.setSelected();
  }
  videoMonitorList.setEnabled(videoMonitorList.itemCount() > 1 && ruby::video.hasMonitor());
  videoFormatList.setEnabled(0 && videoFormatList.itemCount() > 1);
#if !defined(PLATFORM_MACOS)
  videoExclusiveToggle.setChecked(ruby::video.exclusive()).setEnabled(ruby::video.hasExclusive());
#endif
  videoBlockingToggle.setChecked(ruby::video.blocking()).setEnabled(ruby::video.hasBlocking());
#if defined(PLATFORM_MACOS)
  videoColorSpaceToggle.setChecked(ruby::video.forceSRGB()).setEnabled(ruby::video.hasForceSRGB());
  videoThreadedRendererToggle.setChecked(ruby::video.threadedRenderer()).setEnabled(ruby::video.hasThreadedRenderer());
  videoNativeFullScreenToggle.setChecked(ruby::video.nativeFullScreen()).setEnabled(ruby::video.hasNativeFullScreen());
#endif
  videoFlushToggle.setChecked(ruby::video.flush()).setEnabled(ruby::video.hasFlush());
  VerticalLayout::resize();
}

auto DriverSettings::videoDriverUpdate() -> bool {
  lock_guard<recursive_mutex> programLock(program.programMutex);
  if(emulator && settings.video.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return false;
  program.videoDriverUpdate();
  videoRefresh();
  return true;
}

auto DriverSettings::audioRefresh() -> void {
  audioDriverList.reset();
  for(auto& driver : ruby::audio.hasDrivers()) {
    ComboButtonItem item{&audioDriverList};
    item.setText(driver);
    if(driver == ruby::audio.driver()) item.setSelected();
    if(settings.audio.driver == ruby::audio.driver()) {
      audioDriverAssign.setEnabled(false);
    }
  }
  audioDeviceList.reset();
  for(auto& device : ruby::audio.hasDevices()) {
    ComboButtonItem item{&audioDeviceList};
    item.setText(device);
    if(device == ruby::audio.device()) item.setSelected();
  }
  audioFrequencyList.reset();
  for(auto& frequency : ruby::audio.hasFrequencies()) {
    ComboButtonItem item{&audioFrequencyList};
    item.setText(frequency);
    if(frequency == ruby::audio.frequency()) item.setSelected();
  }
  audioLatencyList.reset();
  for(auto& latency : ruby::audio.hasLatencies()) {
    ComboButtonItem item{&audioLatencyList};
    item.setText(latency);
    if(latency == ruby::audio.latency()) item.setSelected();
  }
  audioDeviceList.setEnabled(audioDeviceList.itemCount() > 1);
  audioExclusiveToggle.setChecked(ruby::audio.exclusive()).setEnabled(ruby::audio.hasExclusive());
  audioBlockingToggle.setChecked(ruby::audio.blocking()).setEnabled(ruby::audio.hasBlocking());
  audioDynamicToggle.setChecked(ruby::audio.dynamic()).setEnabled(ruby::audio.hasDynamic());
  VerticalLayout::resize();
}

auto DriverSettings::audioDriverUpdate() -> bool {
  lock_guard<recursive_mutex> programLock(program.programMutex);
  if(emulator && settings.audio.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return false;
  program.audioDriverUpdate();
  audioRefresh();
  return true;
}

auto DriverSettings::inputRefresh() -> void {
  inputDriverList.reset();
  for(auto& driver : ruby::input.hasDrivers()) {
    ComboButtonItem item{&inputDriverList};
    item.setText(driver);
    if(driver == ruby::input.driver()) item.setSelected();
    if(settings.input.driver == ruby::input.driver()) {
      inputDriverAssign.setEnabled(false);
    }
  }
  VerticalLayout::resize();
}

auto DriverSettings::inputDriverUpdate() -> bool {
  lock_guard<recursive_mutex> programLock(program.programMutex);
  if(emulator && settings.input.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return false;
  program.inputDriverUpdate();
  inputRefresh();
  return true;
}
