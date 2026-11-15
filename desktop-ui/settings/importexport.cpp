auto ImportExportSettings::construct() -> void {
  setCollapsible();
  useImported.setText("将导入的文件设为当前设置文件");
  useImported.setChecked(false).onToggle([&] { imported = useImported.checked(); });
  refresh();

  importButton.setText("导入…").onActivate([&] {
    Program::Guard guard;
    BrowserDialog dialog;
    dialog.setTitle("导入设置");
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
      coreSettings.construct();
      developerSettings.construct();
      importExportSettings.construct();
    }
  });
  
  exportButton.setText("导出…").onActivate([&] {
    Program::Guard guard;
    BrowserDialog dialog;
    dialog.setTitle("导出设置");
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
  settingsFileLabel.setText({"当前文件： ", settings.filePath}).setFont(Font().setBold());
  settingsView.setEditable(false).setFont(Font().setFamily(Font::Mono));
  settingsView.setText(BML::serialize(settings, " "));
}

auto ImportExportSettings::setVisible(bool visible) -> ImportExportSettings& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
