auto NCISettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  nciLabel.setText("Network Control Interface").setFont(Font().setBold());
  portLayout.setAlignment(1);
    portLabel.setText("Port");

    port.setText(integer(settings.nci.port));
    port.setEditable(true);
    port.onChange([&](){
      settings.nci.port = port.text().integer();
      string portStr = integer(settings.nci.port);

      if(portStr != port.text()) {
        port.setText(settings.nci.port == 0 ? string{""} : portStr);
      }

      infoRefresh();
    });

    portHint.setText("Default: 55355").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

    ipv4.setEnabled(true);
    ipv4.setText("Use IPv4");
    ipv4.setChecked(settings.nci.useIPv4);
    ipv4.onToggle([&](){
      settings.nci.useIPv4 = ipv4.checked();
      serverRefresh();
      infoRefresh();
    });

    enabled.setEnabled(true);
    enabled.setText("Enabled");
    enabled.setChecked(settings.nci.enabled);
    enabled.onToggle([&](){
      settings.nci.enabled = enabled.checked();
      serverRefresh();
      infoRefresh();
    });

  infoRefresh();
}

auto NCISettings::infoRefresh() -> void {
  if(settings.nci.enabled) {
    connectInfo.setText(settings.nci.useIPv4
      ? "Note: IPv4 mode binds to any device, enabling anyone in your network to access this server"
      : "Note: localhost only (for Windows/WSL: please use IPv4 instead)"
    );
  } else {
    connectInfo.setText("");
  }
}

auto NCISettings::serverRefresh() -> void {
  nci.close();

  if(settings.nci.enabled) {
    nci.open();
  }
}
