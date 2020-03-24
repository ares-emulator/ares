VideoSettings::VideoSettings(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);

  driverHeader.setText("Video Driver").setFont(Font().setBold());
  driverLabel.setText("Driver:");
  for(auto& driver : Video::hasDrivers()) {
    ComboButtonItem item{&driverOption};
    item.setText(driver);
    if(driver == settings.video.driver) item.setSelected();
  }
  changeButton.setText("Change").onActivate([&] { eventChange(); });

  settingsHeader.setText("Driver Settings").setFont(Font().setBold());
  monitorLabel.setText("Monitor:");
  monitorOption.append(ComboButtonItem().setText(settings.video.monitor)).onChange([&] {
    settings.video.monitor = monitorOption.selected().text();
    refresh();
  });
  formatLabel.setText("Format:");
  formatOption.append(ComboButtonItem().setText(settings.video.format)).onChange([&] {
    settings.video.format = formatOption.selected().text();
    refresh();
  });
  exclusiveOption.setText("Exclusive").setChecked(settings.video.exclusive).onToggle([&] {
    settings.video.exclusive = exclusiveOption.checked();
    refresh();
  });
  blockingOption.setText("Blocking").setChecked(settings.video.blocking).onToggle([&] {
    settings.video.blocking = blockingOption.checked();
    refresh();
  });
  flushOption.setText("Flush").setChecked(settings.video.flush).onToggle([&] {
    settings.video.flush = flushOption.checked();
    refresh();
  });

  colorHeader.setText("Color Adjustment").setFont(Font().setBold());
  colorLayout.setSize({3, 3});
  colorLayout.column(0).setAlignment(1.0);
  luminanceLabel.setText("Luminance:");
  luminanceValue.setAlignment(0.5);
  luminanceSlider.setLength(101).setPosition(settings.video.luminance * 100.0).onChange([&] {
    settings.video.luminance = luminanceSlider.position() / 100.0;
    luminanceValue.setText({luminanceSlider.position(), "%"});
    emulator.videoUpdateColors();
  }).doChange();
  saturationLabel.setText("Saturation:");
  saturationValue.setAlignment(0.5);
  saturationSlider.setLength(201).setPosition(settings.video.saturation * 100.0).onChange([&] {
    settings.video.saturation = saturationSlider.position() / 100.0;
    saturationValue.setText({saturationSlider.position(), "%"});
    emulator.videoUpdateColors();
  }).doChange();
  gammaLabel.setText("Gamma:");
  gammaValue.setAlignment(0.5);
  gammaSlider.setLength(101).setPosition((settings.video.gamma - 1.0) * 100.0).onChange([&] {
    settings.video.gamma = 1.0 + gammaSlider.position() / 100.0;
    gammaValue.setText({100 + gammaSlider.position(), "%"});
    emulator.videoUpdateColors();
  }).doChange();
  viewport.setVisible(false);
}

auto VideoSettings::show() -> void {
  setVisible(true);
}

auto VideoSettings::hide() -> void {
  setVisible(false);
}

auto VideoSettings::refresh() -> void {
  emulator.videoUpdate();

  monitorOption.reset();
  for(auto& monitor : Video::hasMonitors()) {
    ComboButtonItem item{&monitorOption};
    item.setText(monitor.name);
    if(monitor.name == settings.video.monitor) item.setSelected();
  }
  monitorOption.setEnabled(monitorOption.itemCount() > 1);

  formatOption.reset();
  for(auto& format : videoInstance.hasFormats()) {
    ComboButtonItem item{&formatOption};
    item.setText(format);
    if(format == settings.video.format) item.setSelected();
  }
//formatOption.setEnabled(formatOption.itemCount() > 1);
//temporarily disabled; Emulator::Node::Sprite needs to be able to handle non-ARGB8888 formats first.
  formatOption.setEnabled(false);

  optionsLayout.resize();

  exclusiveOption.setEnabled(videoInstance.hasExclusive()).setChecked(videoInstance.exclusive());
  blockingOption.setEnabled(videoInstance.hasBlocking()).setChecked(videoInstance.blocking());
  flushOption.setEnabled(videoInstance.hasFlush()).setChecked(videoInstance.flush());
}

auto VideoSettings::eventChange() -> void {
  settings.video.driver = driverOption.selected().text();
  refresh();
  driverHeader.setText({"Video Driver (", settings.video.driver, ")"});
  settingsLayout.setEnabled(true);
}
