auto ImportExportSettings::construct() -> void {
  setCollapsible();
  useImported.setText("Set Imported File As Current Settings File");
  useImported.setChecked(settings.video.colorBleed).onToggle([&] { imported = useImported.checked(); });
  refresh();

  importButton.setText("Import" ELLIPSIS).onActivate([&] {
    Program::Guard guard;
    BrowserDialog dialog;
    dialog.setTitle("Import Settings");
    dialog.setPath(settings.paths.home);
    dialog.setFilters({"bml|*.bml"});
    dialog.setAlignment(settingsWindow);
    if(auto location = program.openFile(dialog)) {
      string currentSavePath = settings.filePath;
      settings.filePath = location;
      settings.load();
      if(!imported) settings.filePath = currentSavePath;
      videoSettings.construct();
      audioSettings.construct();
      inputSettings.construct();
      hotkeySettings.construct();
      emulatorSettings.construct();
      optionSettings.construct();
      firmwareSettings.construct();
      pathSettings.construct();
      driverSettings.construct();
      debugSettings.construct();
      importExportSettings.construct();
    }
  });
  
  exportButton.setText("Export" ELLIPSIS).onActivate([&] {
    Program::Guard guard;
    BrowserDialog dialog;
    dialog.setTitle("Export Settings");
    dialog.setPath(settings.paths.home);  
    dialog.setFilters({"bml|*.bml"});
    dialog.setAlignment(settingsWindow);
    if(auto location = program.saveFile(dialog)) {
      string currentSavePath = settings.filePath;
      settings.filePath = location;
      settings.save();
      settings.filePath = currentSavePath;
    }
  });
}

auto ImportExportSettings::refresh() -> void {
  settings.save();
  settingsFileLabel.setText({"Current File: ", settings.filePath}).setFont(Font().setBold());
  settingsView.setEditable(false).setFont(Font().setFamily(Font::Mono));
  settingsView.setText(BML::serialize(settings, " "));
}

auto ImportExportSettings::setVisible(bool visible) -> ImportExportSettings& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
