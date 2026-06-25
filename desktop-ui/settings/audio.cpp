auto AudioSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  effectsLabel.setText("Sound Effects").setFont(Font().setBold());
  effectsLayout.setSize({3, 2}).setPadding(12_sx, 0);
  effectsLayout.column(0).setAlignment(1.0);

  volumeLabel.setText("Volume:");
  volumeValue.setAlignment(0.5);
  volumeSlider.setLength(201).setPosition(settings.audio.volume * 100.0).onChange([&] {
    settings.audio.volume = volumeSlider.position() / 100.0;
    volumeValue.setText({volumeSlider.position(), "%"});
  }).doChange();

  balanceLabel.setText("Balance:");
  balanceValue.setAlignment(0.5);
  balanceSlider.setLength(101).setPosition((settings.audio.balance * 50.0) + 50.0).onChange([&] {
    settings.audio.balance = ((s32)balanceSlider.position() - 50.0) / 50.0;
    balanceValue.setText({balanceSlider.position(), "%"});
  }).doChange();

  audioLabel.setText("Device Settings").setFont(Font().setBold());
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

  audioDynamicToggle.setText("Dynamic rate").onToggle([&] {
    Program::Guard guard;
    settings.audio.dynamic = audioDynamicToggle.checked();
    ruby::audio.setDynamic(settings.audio.dynamic);
  });

  audioDeviceLayout.setPadding(12_sx, 0);
  audioPropertyLayout.setPadding(12_sx, 0);
}

auto AudioSettings::audioRefresh() -> void {
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
  audioDynamicToggle.setChecked(ruby::audio.dynamic()).setEnabled(ruby::audio.hasDynamic());
  VerticalLayout::resize();
}

auto AudioSettings::audioDriverUpdate() -> bool {
  Program::Guard guard;
  program.audioDriverUpdate();
  audioRefresh();
  return true;
}
