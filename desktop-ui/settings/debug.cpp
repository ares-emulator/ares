auto DebugSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  debugLabel.setText("GDB-Server").setFont(Font().setBold());
  portLayout.setAlignment(1);
    portLabel.setText("Port");

    port.setText(integer(settings.debugServer.port));
    port.setEditable(true);
    port.onChange([&](){
      settings.debugServer.port = port.text().integer();
      string portStr = integer(settings.debugServer.port);

      if(portStr != port.text()) {
        port.setText(settings.debugServer.port == 0 ? string{""} : portStr);
      }

      infoRefresh();
    });

    portHint.setText("Safe range: 1024 - 32767").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  ipv4Layout.setAlignment(1);
    ipv4Label.setText("Use IPv4");

    ipv4.setEnabled(true);
    ipv4.setChecked(settings.debugServer.useIPv4);
    ipv4.onToggle([&](){
      settings.debugServer.useIPv4 = ipv4.checked();
      serverRefresh();
      infoRefresh();
    });

  enabledLayout.setAlignment(1);
    enabledLabel.setText("Enabled");

    enabled.setEnabled(true);
    enabled.setChecked(settings.debugServer.enabled);
    enabled.onToggle([&](){
      settings.debugServer.enabled = enabled.checked();
      serverRefresh();
      infoRefresh();
    });

  infoRefresh();
}

auto DebugSettings::infoRefresh() -> void {
  if(settings.debugServer.enabled) {
    connectInfo.setText(settings.debugServer.useIPv4 
      ? "Note: IPv4 mode binds to any device, enabling anyone in your network to access this server"
      : "Note: localhost only (for Windows/WSL: please use IPv4 instead)"
    );
    presentation.statusDebug.setText(
      nall::GDB::server.getStatusText(settings.debugServer.port, settings.debugServer.useIPv4)
    );
  } else {
    connectInfo.setText("");
    presentation.statusDebug.setText("");
  }
}

auto DebugSettings::serverRefresh() -> void {
  nall::GDB::server.close();

  if(settings.debugServer.enabled) {
    nall::GDB::server.open(settings.debugServer.port, settings.debugServer.useIPv4);
  }
}
