auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  optionsLabel.setText("Options").setFont(Font().setBold());

  rewind.setText("Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    program.rewindReset();
  }).doToggle();
  rewindHint.setText("Allows you to reverse time via the rewind hotkey").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});

  runAhead.setText("Run-Ahead").setEnabled(co_serializable()).setChecked(settings.general.runAhead && co_serializable()).onToggle([&] {
    settings.general.runAhead = runAhead.checked() && co_serializable();
    program.runAheadUpdate();
  });
  runAheadHint.setText("Removes one frame of input lag, but doubles system requirements").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});

  autoSaveMemory.setText("Auto-Save Memory Periodically").setChecked(settings.general.autoSaveMemory).onToggle([&] {
    settings.general.autoSaveMemory = autoSaveMemory.checked();
  });
  autoSaveMemoryHint.setText("Helps safeguard game saves from being lost").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});

  fastBoot.setText("Fast Boot").setChecked(settings.general.fastBoot).onToggle([&] {
    settings.general.fastBoot = fastBoot.checked();
  });
  fastBootHint.setText("Skips BIOS boot screens for the PlayStation and Neo Geo Pocket").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});

  autoDebug.setText("Auto Debug").setChecked(settings.general.autoDebug).onToggle([&] {
    settings.general.autoDebug = autoDebug.checked();
  });
  autoDebugHint.setText("Pauses the emulator and activates the debugger on game load").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});

  nativeFileDialogs.setText("Use Native File Dialogs").setChecked(settings.general.nativeFileDialogs).onToggle([&] {
    settings.general.nativeFileDialogs = nativeFileDialogs.checked();
  });
  nativeFileDialogsHint.setText("More familiar, but lacks advanced loading options").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
}
