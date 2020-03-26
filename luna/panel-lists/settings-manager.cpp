SettingsManager::SettingsManager(View* view) : PanelList(view, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  listView.onChange([&] { onChange(); });
}

auto SettingsManager::show() -> void {
  listView.selectNone().doChange();
  refresh();
  setVisible(true);
}

auto SettingsManager::hide() -> void {
  setVisible(false);
}

auto SettingsManager::refresh() -> void {
  listView.reset();
  listView.append(ListViewItem().setText("Video").setIcon(Icon::Device::Display));
  listView.append(ListViewItem().setText("Audio").setIcon(Icon::Device::Speaker));
  listView.append(ListViewItem().setText("Input").setIcon(Icon::Device::Joypad));
  listView.append(ListViewItem().setText("Hotkeys").setIcon(Icon::Device::Keyboard));
}

auto SettingsManager::onChange() -> void {
  if(auto item = listView.selected()) {
    if(item.text() == "Video") return program.setPanelItem(videoSettings);
    if(item.text() == "Audio") return program.setPanelItem(audioSettings);
    if(item.text() == "Input") return program.setPanelItem(inputSettings);
    if(item.text() == "Hotkeys") return program.setPanelItem(hotkeySettings);
  }
  return program.setPanelItem(home);
}
