InputSettings::InputSettings(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);

  driverHeader.setText("Input Settings").setFont(Font().setBold());
  driverLabel.setText("Driver:");
  for(auto& driver : Input::hasDrivers()) {
    ComboButtonItem item{&driverOption};
    item.setText(driver);
    if(driver == settings.input.driver) item.setSelected();
  }
  driverOption.onChange([&] {
    settings.input.driver = driverOption.selected().text();
  });
  changeButton.setText("Change").onActivate([&] { eventChange(); });

  focusLossHeader.setText("Focus Loss").setFont(Font().setBold());
  focusPause.setText("Pause emulation").onActivate([&] {
    settings.input.unfocused = "Pause";
  });
  focusBlock.setText("Block input").onActivate([&] {
    settings.input.unfocused = "Block";
  });
  focusAllow.setText("Allow input").onActivate([&] {
    settings.input.unfocused = "Allow";
  });
  if(settings.input.unfocused == "Pause") focusPause.setChecked();
  if(settings.input.unfocused == "Block") focusBlock.setChecked();
  if(settings.input.unfocused == "Allow") focusAllow.setChecked();
}

auto InputSettings::show() -> void {
  setVisible(true);
}

auto InputSettings::hide() -> void {
  setVisible(false);
}

auto InputSettings::refresh() -> void {
  emulator.inputUpdate();
}

auto InputSettings::eventChange() -> void {
  settings.input.driver = driverOption.selected().text();
  refresh();
  driverHeader.setText({"Input Driver (", settings.input.driver, ")"});
}
