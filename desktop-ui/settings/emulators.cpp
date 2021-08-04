auto EmulatorSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  emulatorLabel.setText("Load Menu Emulators").setFont(Font().setBold());
  emulatorList.onToggle([&](auto cell) { eventToggle(cell); });
  emulatorList.append(TableViewColumn());
  emulatorList.append(TableViewColumn().setText("Name").setExpandable());
  emulatorList.append(TableViewColumn().setText("Manufacturer").setAlignment(1.0));
  emulatorList.setHeadered();
  for(auto& emulator : emulators) {
    TableViewItem item{&emulatorList};
    TableViewCell visible{&item};
    visible.setAttribute<shared_pointer<Emulator>>("emulator", emulator);
    visible.setCheckable();
    visible.setChecked(emulator->configuration.visible);
    TableViewCell name{&item};
    name.setText(emulator->name);
    TableViewCell manufacturer{&item};
    manufacturer.setText(emulator->manufacturer);
  }

  groupEmulators.setText("Group Emulators").setChecked(settings.general.groupEmulators).onToggle([&] {
    settings.general.groupEmulators = groupEmulators.checked();
    presentation.loadEmulators();
  });
  groupEmulatorsHint.setText("Groups emulators by manufacturer in the load menu").setFont(Font().setSize(7.0)).setForegroundColor({80, 80, 80});
}

auto EmulatorSettings::eventToggle(TableViewCell cell) -> void {
  if(auto emulator = cell.attribute<shared_pointer<Emulator>>("emulator")) {
    emulator->configuration.visible = cell.checked();
    presentation.loadEmulators();
  }
}
