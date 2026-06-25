auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  synchronizationLabel.setText("Synchronization ").setFont(Font().setBold());
  for(auto& opt : array<string[2]>{"Sync to Audio", "Sync to Video"}) {
    ComboButtonItem item{&synchronizationList};
    item.setText(opt);
    if((settings.video.blocking) && (opt == "Sync to Video")) {
      item.setSelected();
    } 
  }
  synchronizationList.onChange([&] {
    Program::Guard guard;
    auto selected = synchronizationList.selected().text();
    if(selected == "Sync to Audio") {
      settings.audio.blocking = true;
      settings.video.blocking = false;
      settings.video.flush = false;
    } else if(selected == "Sync to Video") {
      settings.video.blocking = true;
      settings.video.flush = true;
      settings.audio.blocking = false;
    }
    ruby::audio.setBlocking(settings.audio.blocking);
    ruby::video.setBlocking(settings.video.blocking);
    ruby::video.setFlush(settings.video.flush);
  });

  synchronizationLayout.setAlignment(0.5).setPadding(12_sx, 0);
  synchronizationHint.setText("Synchronize to audio (Recommended) or to your monitor's refresh rate").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  commonSettingsLabel.setText("Emulator Options").setFont(Font().setBold());

  runAhead.setText("Run-Ahead").setEnabled(co_serializable()).setChecked(settings.general.runAhead && co_serializable()).onToggle([&] {
    settings.general.runAhead = runAhead.checked() && co_serializable();
    program.runAheadUpdate();
  });
  runAheadLayout.setAlignment(1).setPadding(12_sx, 0);
    runAheadHint.setText("Removes one frame of input lag, but doubles system requirements").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  autoSaveMemory.setText("Auto-Save Memory Periodically").setChecked(settings.general.autoSaveMemory).onToggle([&] {
    settings.general.autoSaveMemory = autoSaveMemory.checked();
  });
  autoSaveMemoryLayout.setAlignment(1).setPadding(12_sx, 0);
    autoSaveMemoryHint.setText("Helps safeguard game saves from being lost").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  noFilePromptOption.setText("Disable requests for loading additional media").setChecked(settings.general.noFilePrompt).onToggle([&] {
    settings.general.noFilePrompt = noFilePromptOption.checked();
    
  });
  noFilePromptLayout.setAlignment(1).setPadding(12_sx, 0);
    noFilePromptHint.setText("Suppress secondary media file load prompts").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  rewindSettingsLabel.setText("Rewind Settings").setFont(Font().setBold());
  rewind.setText("Enable Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    rewindFrequencyOption.setEnabled(settings.general.rewind);
    rewindLengthOption.setEnabled(settings.general.rewind);
    rewindMute.setEnabled(settings.general.rewind);
    program.rewindReset();
  }).doToggle();
  rewindLayout.setAlignment(1).setPadding(12_sx, 0);
    rewindHint.setText("Allows you to reverse time via the rewind hotkey (may impact performance)").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  rewindSettingsLayout.setAlignment(0.5).setPadding(12_sx, 0);
  rewindFrequencyLabel.setText("Frequency:");
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 20 frames"));
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 40 frames"));
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 60 frames"));
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 80 frames"));
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 100 frames"));
  rewindFrequencyOption.append(ComboButtonItem().setText("Every 120 frames"));
  if(settings.rewind.frequency == 20) rewindFrequencyOption.item(0).setSelected();
  if(settings.rewind.frequency == 40) rewindFrequencyOption.item(1).setSelected();
  if(settings.rewind.frequency == 60) rewindFrequencyOption.item(2).setSelected();
  if(settings.rewind.frequency == 80) rewindFrequencyOption.item(3).setSelected();
  if(settings.rewind.frequency == 100) rewindFrequencyOption.item(4).setSelected();
  if(settings.rewind.frequency == 120) rewindFrequencyOption.item(5).setSelected();
  rewindFrequencyOption.onChange([&] {
    settings.rewind.frequency = rewindFrequencyOption.selected().offset() * 20 + 20;
    program.rewindReset();
  });

  rewindLengthLabel.setText("Length:");
  rewindLengthOption.append(ComboButtonItem().setText( "10 states"));
  rewindLengthOption.append(ComboButtonItem().setText( "20 states"));
  rewindLengthOption.append(ComboButtonItem().setText( "40 states"));
  rewindLengthOption.append(ComboButtonItem().setText( "80 states"));
  rewindLengthOption.append(ComboButtonItem().setText("160 states"));
  rewindLengthOption.append(ComboButtonItem().setText("320 states"));
  if(settings.rewind.length ==  10) rewindLengthOption.item(0).setSelected();
  if(settings.rewind.length ==  20) rewindLengthOption.item(1).setSelected();
  if(settings.rewind.length ==  40) rewindLengthOption.item(2).setSelected();
  if(settings.rewind.length ==  80) rewindLengthOption.item(3).setSelected();
  if(settings.rewind.length == 160) rewindLengthOption.item(4).setSelected();
  if(settings.rewind.length == 320) rewindLengthOption.item(5).setSelected();
  rewindLengthOption.onChange([&] {
    settings.rewind.length = 10 << rewindLengthOption.selected().offset();
    program.rewindReset();
  });

  rewindMute.setText("Mute while rewinding").setChecked(settings.rewind.mute).onToggle([&] {
    settings.rewind.mute = rewindMute.checked();
  });
}
