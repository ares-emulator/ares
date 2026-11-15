auto EmulatorSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  emulatorLabel.setText("加载菜单模拟器").setFont(Font().setBold());
  emulatorList.onToggle([&](auto cell) { eventToggle(cell); });
  emulatorList.append(TableViewColumn());
  emulatorList.append(TableViewColumn().setText("名称").setExpandable());
  emulatorList.append(TableViewColumn().setText("制造商").setAlignment(1.0));
  emulatorList.setHeadered();
  for(auto& emulator : emulators) {
    TableViewItem item{&emulatorList};
    TableViewCell visible{&item};
    visible.setAttribute<std::shared_ptr<Emulator>>("emulator", emulator);
    visible.setCheckable();
    visible.setChecked(emulator->configuration.visible);
    TableViewCell name{&item};
    name.setText(emulator->name);
    TableViewCell manufacturer{&item};
    manufacturer.setText(emulator->manufacturer);
  }
  emulatorList.resizeColumns();
  emulatorList.column(0).setWidth(32);
}

auto EmulatorSettings::eventToggle(TableViewCell cell) -> void {
  if(auto emulator = cell.attribute<std::shared_ptr<Emulator>>("emulator")) {
    emulator->configuration.visible = cell.checked();
    presentation.loadEmulators();
  }
}
