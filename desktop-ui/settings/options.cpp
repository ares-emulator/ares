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

  synchronizationLayout.setAlignment(1).setPadding(12_sx, 0);
  synchronizationHint.setText("Synchronize to audio (Recommended) or to your monitor's refresh rate").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  commonSettingsLabel.setText("Emulator Options").setFont(Font().setBold());

  rewind.setText("Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    program.rewindReset();
  }).doToggle();
  rewindLayout.setAlignment(1).setPadding(12_sx, 0);
    rewindHint.setText("Allows you to reverse time via the rewind hotkey").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

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
}
