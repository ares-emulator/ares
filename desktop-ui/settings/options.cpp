auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

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

  homebrewMode.setText("Homebrew Development Mode").setChecked(settings.general.homebrewMode).onToggle([&] {
    settings.general.homebrewMode = homebrewMode.checked();
  });
  homebrewModeLayout.setAlignment(1).setPadding(12_sx, 0);
      homebrewModeHint.setText("Activate core-specific features to help homebrew developers").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  forceInterpreter.setText("Force Intepreter").setChecked(settings.general.forceInterpreter).onToggle([&] {
    settings.general.forceInterpreter = forceInterpreter.checked();
  });
  forceInterpreterLayout.setAlignment(1).setPadding(12_sx, 0);
      forceInterpreterHint.setText("(Slow) Enable interpreter for cores that default to a recompiler").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64SettingsLabel.setText("Nintendo 64 Settings").setFont(Font().setBold());

  nintendo64ExpansionPakOption.setText("4MB Expansion Pak").setChecked(settings.nintendo64.expansionPak).onToggle([&] {
    settings.nintendo64.expansionPak = nintendo64ExpansionPakOption.checked();
  });
  nintendo64ExpansionPakLayout.setAlignment(1).setPadding(12_sx, 0);
      nintendo64ExpansionPakHint.setText("Enable/Disable the 4MB Expansion Pak").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  
}
