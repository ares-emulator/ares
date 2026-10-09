auto ImportExportSettings::construct() -> void {
  setCollapsible();

  profileManagerLabel.setText("Settings Profiles (*.bml)").setFont(Font().setBold());
  if(!profileDirectory) profileDirectory = Location::path(settings.filePath);
  profileFolderPath.setEditable(false);
  profileFolderButton.setText("Folder" ELLIPSIS).onActivate([&] {
    Program::Guard guard;
    BrowserDialog dialog;
    dialog.setTitle("Select Settings Profile Folder");
    dialog.setPath(profileDirectory);
    dialog.setAlignment(settingsWindow);
    if(auto location = program.selectFolder(dialog)) {
      profileDirectory = location;
      refreshProfiles();
    }
  });
  profileList.onChange([&] {
    auto item = profileList.selected();
    auto offset = item ? item.offset() : profilePaths.size();
    activateProfileButton.setEnabled(offset < profilePaths.size() && Path::real(profilePaths[offset]) != Path::real(settings.filePath));
  });
  activateProfileButton.setText("Activate").onActivate([&] { activateProfile(); });
  refreshProfilesButton.setText("Refresh").onActivate([&] { refreshProfiles(); });

  useImported.setText("Set Imported File As Current Settings File");
  useImported.setChecked(false).onToggle([&] { imported = useImported.checked(); });
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
      if(imported) profileDirectory = Location::path(location);
      rebuildSettingsPanels();
      refresh();
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
      profileDirectory = Location::path(location);
      refreshProfiles();
    }
  });
}

auto ImportExportSettings::refresh() -> void {
  settings.save();
  refreshProfiles();
  settingsFileLabel.setText({"Current File: ", settings.filePath}).setFont(Font().setBold());
  settingsView.setEditable(false).setFont(Font().setFamily(Font::Mono));
  settingsView.setText(string::read(settings.filePath));
}

auto ImportExportSettings::refreshProfiles() -> void {
  if(!profileDirectory) profileDirectory = Location::path(settings.filePath);
  if(!profileDirectory.endsWith("/")) profileDirectory.append("/");
  profileFolderPath.setText(profileDirectory);

  profilePaths.clear();
  profileList.reset();
  if(directory::exists(profileDirectory)) {
    auto files = directory::files(profileDirectory, "*.bml");
    std::ranges::sort(files);
    for(auto& filename : files) {
      string location{profileDirectory, filename};
      profilePaths.push_back(location);
      ComboButtonItem item{&profileList};
      auto current = Path::real(location) == Path::real(settings.filePath);
      item.setText({filename, current ? " (current)" : ""});
      if(current) item.setSelected();
    }
  }

  auto item = profileList.selected();
  auto offset = item ? item.offset() : profilePaths.size();
  activateProfileButton.setEnabled(offset < profilePaths.size() && Path::real(profilePaths[offset]) != Path::real(settings.filePath));
}

auto ImportExportSettings::activateProfile() -> void {
  auto item = profileList.selected();
  if(!item || item.offset() >= profilePaths.size()) return;
  auto location = profilePaths[item.offset()];
  if(!file::exists(location)) {
    MessageDialog("The selected settings profile no longer exists.").setAlignment(settingsWindow).error();
    refreshProfiles();
    return;
  }
  if(Path::real(location) == Path::real(settings.filePath)) return;

  Program::Guard guard;
  settings.save();
  settings.filePath = location;
  settings.load();
  profileDirectory = Location::path(location);
  rebuildSettingsPanels();
  refresh();
}

auto ImportExportSettings::rebuildSettingsPanels() -> void {
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
}

auto ImportExportSettings::setVisible(bool visible) -> ImportExportSettings& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
