AudioSettings::AudioSettings(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);

  driverHeader.setText("Audio Driver").setFont(Font().setBold());
  driverLabel.setText("Driver:");
  for(auto& driver : Audio::hasDrivers()) {
    ComboButtonItem item{&driverOption};
    item.setText(driver);
    if(driver == settings.audio.driver) item.setSelected();
  }
  driverOption.onChange([&] {
    settings.audio.driver = driverOption.selected().text();
  });
  changeButton.setText("Change").onActivate([&] { eventChange(); });

  settingsHeader.setText("Driver Settings").setFont(Font().setBold());
  settingsLayout.setEnabled(false);
  deviceLabel.setText("Device:");
  deviceOption.append(ComboButtonItem().setText(settings.audio.device)).onChange([&] {
    settings.audio.device = deviceOption.selected().text();
    refresh();
  });
  frequencyLabel.setText("Frequency:");
  frequencyOption.append(ComboButtonItem().setText(settings.audio.frequency)).onChange([&] {
    settings.audio.frequency = frequencyOption.selected().text().natural();
    refresh();
    emulator.audioUpdateEffects();
  });
  latencyLabel.setText("Latency:");
  latencyOption.append(ComboButtonItem().setText(settings.audio.latency)).onChange([&] {
    settings.audio.latency = latencyOption.selected().text().natural();
    refresh();
  });
  exclusiveOption.setText("Exclusive").setChecked(settings.audio.exclusive).onToggle([&] {
    settings.audio.exclusive = exclusiveOption.checked();
    refresh();
  });
  blockingOption.setText("Blocking").setChecked(settings.audio.blocking).onToggle([&] {
    settings.audio.blocking = blockingOption.checked();
    refresh();
  });
  dynamicOption.setText("Dynamic").setChecked(settings.audio.dynamic).onToggle([&] {
    settings.audio.dynamic = dynamicOption.checked();
    refresh();
  });

  effectsHeader.setText("Effects").setFont(Font().setBold());
  effectsLayout.setSize({3, 3});
  effectsLayout.column(0).setAlignment(1.0);
  skewLabel.setText("Skew:");
  skewValue.setAlignment(0.5);
  skewSlider.setLength(10001).setPosition(settings.audio.skew + 5000).onChange([&] {
    settings.audio.skew = (int)skewSlider.position() - 5000;
    skewValue.setText({settings.audio.skew > 0 ? "+" : "", settings.audio.skew});
    emulator.audioUpdateEffects();
  }).doChange();
  volumeLabel.setText("Volume:");
  volumeValue.setAlignment(0.5);
  volumeSlider.setLength(201).setPosition(settings.audio.volume * 100.0).onChange([&] {
    settings.audio.volume = volumeSlider.position() / 100.0;
    volumeValue.setText({volumeSlider.position(), "%"});
    emulator.audioUpdateEffects();
  }).doChange();
  balanceLabel.setText("Balance:");
  balanceValue.setAlignment(0.5);
  balanceSlider.setLength(101).setPosition((settings.audio.balance * 50.0) + 50).onChange([&] {
    settings.audio.balance = ((int)balanceSlider.position() - 50) / 50.0;
    balanceValue.setText({balanceSlider.position(), "%"});
    emulator.audioUpdateEffects();
  }).doChange();
}

auto AudioSettings::show() -> void {
  setVisible(true);
}

auto AudioSettings::hide() -> void {
  setVisible(false);
}

auto AudioSettings::refresh() -> void {
  emulator.audioUpdate();

  deviceOption.reset();
  for(auto& device : audioInstance.hasDevices()) {
    ComboButtonItem item{&deviceOption};
    item.setText(device);
    if(device == settings.audio.device) item.setSelected();
  }
  deviceOption.setEnabled(deviceOption.itemCount() > 1);

  frequencyOption.reset();
  for(auto& frequency : audioInstance.hasFrequencies()) {
    ComboButtonItem item{&frequencyOption};
    item.setText(frequency);
    if(frequency == settings.audio.frequency) item.setSelected();
  }
  frequencyOption.setEnabled(frequencyOption.itemCount() > 1);

  latencyOption.reset();
  for(auto& latency : audioInstance.hasLatencies()) {
    ComboButtonItem item{&latencyOption};
    item.setText(latency);
    if(latency == settings.audio.latency) item.setSelected();
  }
  latencyOption.setEnabled(latencyOption.itemCount() > 1);

  settingsLayout.resize();

  exclusiveOption.setEnabled(audioInstance.hasExclusive());
  blockingOption.setEnabled(audioInstance.hasBlocking());
  dynamicOption.setEnabled(audioInstance.hasDynamic());
}

auto AudioSettings::eventChange() -> void {
  settings.audio.driver = driverOption.selected().text();
  refresh();
  driverHeader.setText({"Audio Driver (", settings.audio.driver, ")"});
  settingsLayout.setEnabled(true);
}
