#if defined(PLATFORM_MACOS)
#define ELLIPSIS "\u2026"
#else
#define ELLIPSIS " ..."
#endif

auto PathSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  homeLabel.setText("Home").setFont(Font().setBold());
  homeLayout.setPadding(12_sx, 0);
  homePath.setEditable(false);
  homeAssign.setText("Assign" ELLIPSIS).onActivate([&] {
    BrowserDialog dialog;
    dialog.setTitle("Select Home Path");
    dialog.setPath(Path::desktop());
    dialog.setAlignment(settingsWindow);
    if(auto location = program.selectFolder(dialog)) {
      settings.paths.home = location;
      refresh();
    }
  });
  homeReset.setText("Reset").onActivate([&] {
    settings.paths.home = "";
    refresh();
  });

  savesLabel.setText("Saves").setFont(Font().setBold());
  savesLayout.setPadding(12_sx, 0);
  savesPath.setEditable(false);
  savesAssign.setText("Assign" ELLIPSIS).onActivate([&] {
    BrowserDialog dialog;
    dialog.setTitle("Select Saves Path");
    dialog.setPath(Path::desktop());
    dialog.setAlignment(settingsWindow);
    if(auto location = program.selectFolder(dialog)) {
      settings.paths.saves = location;
      refresh();
    }
  });
  savesReset.setText("Reset").onActivate([&] {
    settings.paths.saves = "";
    refresh();
  });

  screenshotsLabel.setText("Screenshots").setFont(Font().setBold());
  screenshotsLayout.setPadding(12_sx, 0);
  screenshotsPath.setEditable(false);
  screenshotsAssign.setText("Assign" ELLIPSIS).onActivate([&] {
    BrowserDialog dialog;
    dialog.setTitle("Select Screenshots Path");
    dialog.setPath(Path::desktop());
    dialog.setAlignment(settingsWindow);
    if(auto location = program.selectFolder(dialog)) {
      settings.paths.screenshots = location;
      refresh();
    }
  });
  screenshotsReset.setText("Reset").onActivate([&] {
    settings.paths.screenshots = "";
    refresh();
  });

  debuggingLabel.setText("Debugging Files").setFont(Font().setBold());
  debuggingLayout.setPadding(12_sx, 0);
  debuggingPath.setEditable(false);
  debuggingAssign.setText("Assign" ELLIPSIS).onActivate([&] {
    BrowserDialog dialog;
    dialog.setTitle("Select Debugging Path");
    dialog.setPath(Path::desktop());
    dialog.setAlignment(settingsWindow);
    if(auto location = program.selectFolder(dialog)) {
      settings.paths.debugging = location;
      refresh();
    }
  });
  debuggingReset.setText("Reset").onActivate([&] {
    settings.paths.debugging = "";
    refresh();
  });

  refresh();
}

auto PathSettings::refresh() -> void {
  //simplifies pathnames by abbreviating the home folder and trailing slash
  auto pathname = [](string name) -> string {
    if(name.beginsWith(Path::user())) {
      name.trimLeft(Path::user(), 1L);
      name.prepend("~/");
    }
    if(name != "/") name.trimRight("/", 1L);
    return name;
  };

  if(settings.paths.home) {
    homePath.setText(pathname(settings.paths.home)).setForegroundColor();
  } else {
    homePath.setText(pathname(mia::homeLocation())).setForegroundColor(SystemColor::PlaceholderText);
  }

  if(settings.paths.saves) {
    savesPath.setText(pathname(settings.paths.saves)).setForegroundColor();
  } else {
    savesPath.setText("(same as game path)").setForegroundColor(SystemColor::PlaceholderText);
  }

  if(settings.paths.screenshots) {
    screenshotsPath.setText(pathname(settings.paths.screenshots)).setForegroundColor();
  } else {
    screenshotsPath.setText("(same as game path)").setForegroundColor(SystemColor::PlaceholderText);
  }

  if(settings.paths.debugging) {
    debuggingPath.setText(pathname(settings.paths.debugging)).setForegroundColor();
  } else {
    debuggingPath.setText("(same as game path)").setForegroundColor(SystemColor::PlaceholderText);
  }
}
