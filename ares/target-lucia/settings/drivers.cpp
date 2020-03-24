auto DriverSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  videoLabel.setText("Video").setFont(Font().setBold());
  videoDriverLabel.setText("Driver:");
  videoDriverAssign.setText("Reload").onActivate([&] {
    settings.video.driver = videoDriverList.selected().text();
    videoDriverUpdate();
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
  videoExclusiveToggle.setText("Exclusive mode").onToggle([&] {
    settings.video.exclusive = videoExclusiveToggle.checked();
    ruby::video.setExclusive(settings.video.exclusive);
  });
  videoBlockingToggle.setText("Synchronize").onToggle([&] {
    settings.video.blocking = videoBlockingToggle.checked();
    ruby::video.setBlocking(settings.video.blocking);
  });
  videoFlushToggle.setText("GPU sync").onToggle([&] {
    settings.video.flush = videoFlushToggle.checked();
    ruby::video.setFlush(settings.video.flush);
  });

  audioLabel.setText("Audio").setFont(Font().setBold());
  audioDriverLabel.setText("Driver:");
  audioDriverAssign.setText("Reload").onActivate([&] {
    settings.audio.driver = audioDriverList.selected().text();
    audioDriverUpdate();
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
    settings.audio.exclusive = audioExclusiveToggle.checked();
    ruby::audio.setExclusive(settings.audio.exclusive);
  });
  audioBlockingToggle.setText("Synchronize").onToggle([&] {
    settings.audio.blocking = audioBlockingToggle.checked();
    ruby::audio.setBlocking(settings.audio.blocking);
  });
  audioDynamicToggle.setText("Dynamic rate").onToggle([&] {
    settings.audio.dynamic = audioDynamicToggle.checked();
    ruby::audio.setDynamic(settings.audio.dynamic);
  });

  inputLabel.setText("Input").setFont(Font().setBold());
  inputDriverLabel.setText("Driver:");
  inputDriverAssign.setText("Reload").onActivate([&] {
    settings.input.driver = inputDriverList.selected().text();
    inputDriverUpdate();
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
}

auto DriverSettings::videoRefresh() -> void {
  videoDriverList.reset();
  for(auto& driver : ruby::video.hasDrivers()) {
    ComboButtonItem item{&videoDriverList};
    item.setText(driver);
    if(driver == ruby::video.driver()) item.setSelected();
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
  videoMonitorList.setEnabled(videoMonitorList.itemCount() > 1);
  videoFormatList.setEnabled(0 && videoFormatList.itemCount() > 1);
  videoExclusiveToggle.setChecked(ruby::video.exclusive()).setEnabled(ruby::video.hasExclusive());
  videoBlockingToggle.setChecked(ruby::video.blocking()).setEnabled(ruby::video.hasBlocking());
  videoFlushToggle.setChecked(ruby::video.flush()).setEnabled(ruby::video.hasFlush());
  VerticalLayout::resize();
}

auto DriverSettings::videoDriverUpdate() -> void {
  if(emulator && settings.video.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return;
  program.videoDriverUpdate();
  videoRefresh();
}

auto DriverSettings::audioRefresh() -> void {
  audioDriverList.reset();
  for(auto& driver : ruby::audio.hasDrivers()) {
    ComboButtonItem item{&audioDriverList};
    item.setText(driver);
    if(driver == ruby::audio.driver()) item.setSelected();
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

auto DriverSettings::audioDriverUpdate() -> void {
  if(emulator && settings.audio.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return;
  program.audioDriverUpdate();
  audioRefresh();
}

auto DriverSettings::inputRefresh() -> void {
  inputDriverList.reset();
  for(auto& driver : ruby::input.hasDrivers()) {
    ComboButtonItem item{&inputDriverList};
    item.setText(driver);
    if(driver == ruby::input.driver()) item.setSelected();
  }
  VerticalLayout::resize();
}

auto DriverSettings::inputDriverUpdate() -> void {
  if(emulator && settings.input.driver != "None" && MessageDialog(
    "Warning: incompatible drivers may cause this software to crash.\n"
    "Are you sure you want to change this driver while a game is loaded?"
  ).setAlignment(settingsWindow).question() != "Yes") return;
  program.inputDriverUpdate();
  inputRefresh();
}
