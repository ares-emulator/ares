auto DeveloperSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  gdbLabel.setText("Debug Server Options").setFont(Font().setBold());
  portLayout.setAlignment(1);
    portLabel.setText("Port");

    port.setText(integer(settings.developer.debugServerPort));
    port.setEditable(true);
    port.onChange([&](){
      settings.developer.debugServerPort = port.text().integer();
      string portStr = integer(settings.developer.debugServerPort);

      if(portStr != port.text()) {
        port.setText(settings.developer.debugServerPort == 0 ? string{""} : portStr);
      }

      infoRefresh();
    });

    portHint.setText("Safe range: 1024 - 32767").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

    ipv4.setEnabled(true);
    ipv4.setText("Use IPv4");
    ipv4.setChecked(settings.developer.debugServerUseIPv4);
    ipv4.onToggle([&](){
      settings.developer.debugServerUseIPv4 = ipv4.checked();
      serverRefresh();
      infoRefresh();
    });

    enabled.setEnabled(true);
    enabled.setText("Enabled");
    enabled.setChecked(settings.developer.debugServerEnabled);
    enabled.onToggle([&](){
      settings.developer.debugServerEnabled = enabled.checked();
      serverRefresh();
      infoRefresh();
    });

  infoRefresh();

  debugOptionsLabel.setText("Debug Options").setFont(Font().setBold());

  homebrewMode.setText("Homebrew Development Mode").setChecked(settings.developer.homebrewMode).onToggle([&] {
    settings.developer.homebrewMode = homebrewMode.checked();
  });
  homebrewModeLayout.setAlignment(1).setPadding(12_sx, 0);
    homebrewModeHint.setText("Activate system-specific features to help homebrew developers").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  forceInterpreter.setText("Force Interpreter").setChecked(settings.developer.forceInterpreter).onToggle([&] {
    settings.developer.forceInterpreter = forceInterpreter.checked();
  });
  forceInterpreterLayout.setAlignment(1).setPadding(12_sx, 0);
    forceInterpreterHint.setText("(Slow) Enable interpreter for systems that default to a recompiler").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
}

auto DeveloperSettings::infoRefresh() -> void {
  if(settings.developer.debugServerEnabled) {
    connectInfo.setText(settings.developer.debugServerUseIPv4 
      ? "Note: IPv4 mode binds to any device, enabling anyone in your network to access this server"
      : "Note: localhost only (for Windows/WSL: please use IPv4 instead)"
    );
  } else {
    connectInfo.setText("");
  }
}

auto DeveloperSettings::serverRefresh() -> void {
  nall::GDB::server.close();

  if(settings.developer.debugServerEnabled) {
    nall::GDB::server.open(settings.developer.debugServerPort, settings.developer.debugServerUseIPv4);
  }
}
